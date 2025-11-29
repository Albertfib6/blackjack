/*
 * interrupt.c -
 */
#include <types.h>
#include <interrupt.h>
#include <segment.h>
#include <hardware.h>
#include <io.h>

#include <sched.h>

#include <zeos_interrupt.h>

Gate idt[IDT_ENTRIES];
Register    idtR;

char char_map[] =
{
  '\0','\0','1','2','3','4','5','6',
  '7','8','9','0','\'','�','\0','\0',
  'q','w','e','r','t','y','u','i',
  'o','p','`','+','\0','\0','a','s',
  'd','f','g','h','j','k','l','�',
  '\0','�','\0','�','z','x','c','v',
  'b','n','m',',','.','-','\0','*',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','7',
  '8','9','-','4','5','6','+','1',
  '2','3','0','\0','\0','\0','<','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0'
};

int zeos_ticks = 0;
extern struct list_head blockedqueue;

void clock_routine()
{
  zeos_show_clock();
  zeos_ticks ++;

  struct list_head *pos, *n;
  struct task_struct *task;

  list_for_each_safe(pos, n, &blockedqueue) 
  {
    task = list_entry(pos, struct task_struct, list);
    if (task->wake_up_time == zeos_ticks) 
    { 
      list_del(&task->list);
      task->state = ST_READY;
      list_add_tail(&(task->list), &readyqueue);
    }
  }
  
  schedule();
}

#define NUM_KEYS 128
extern char key_state[NUM_KEYS];  // Array global que guarda el estado de las teclas

// void keyboard_routine()
// {
//   unsigned char c = inb(0x60);
//   unsigned char keycode = c & 0x7F;  // Ignoramos el bit de "liberación"

//   if (c&0x80) {
//     key_state[keycode] = 0;
//     //printc_xy(0, 0, char_map[c&0x7f]);
//   }
//   else key_state[keycode] = 1;

// }

void keyboard_routine()
{
  unsigned char c = inb(0x60);
  unsigned char keycode = c & 0x7F; // Ignoramos el bit de "liberación"
  
  // 1. Determinar si es Press (1) o Release (0)
  // En tu código: (c & 0x80) es release, else es press.
  int pressed = (c & 0x80) ? 0 : 1; 

  // Actualizamos el array de estado global (opcional, pero útil para el sistema)
  if (pressed) key_state[keycode] = 1;
  else key_state[keycode] = 0;

  struct task_struct *t = current();

  // 2. Verificar si hay que inyectar el evento
  // Condición: Hay función registrada Y NO estamos ya procesando una (evitar recursión)
  if (t->keyboard_func != NULL && t->in_keyboard_handler == 0) {
      
      char key = char_map[keycode]; // Convertir scancode a char

      // --- PASO A: ACCEDER A LA PILA DE SISTEMA DEL HILO ---
      // Aquí el hardware guardó SS, ESP, EFLAGS, CS, EIP al producirse la interrupción.
      // KERNEL_STACK_SIZE suele ser 1024 dwords (o bytes según tu define).
      // Accedemos como array de unsigned long.
      unsigned long *kernel_stack = (unsigned long *)&t->stack[KERNEL_STACK_SIZE];

      // Índices desde el final (según push de hardware x86):
      // [-1] SS
      // [-2] ESP (User Stack Original)
      // [-3] EFLAGS
      // [-4] CS
      // [-5] EIP (Instrucción donde se interrumpió el usuario)

      // --- PASO B: GUARDAR CONTEXTO ORIGINAL ---
      // Guardamos dónde estaba el usuario para que 'int 0x2b' sepa volver.
      t->saved_eip = kernel_stack[-5]; 
      t->saved_esp = kernel_stack[-2]; 
      t->in_keyboard_handler = 1;      // Marcamos que estamos en evento

      // --- PASO C: PREPARAR PILA AUXILIAR (User Space) ---
      // Usamos la página reservada en sys_KeyboardEvent
      // Calculamos la dirección lineal del techo de esa página
      unsigned int aux_stack_base = (PAG_LOG_INIT_AUX_STACK + 1) << 12;
      unsigned int *user_stack_ptr = (unsigned int *)aux_stack_base;

      // "Push" de los argumentos para libc_keyboard_wrapper
      // Prototipo: void wrapper(void (*func)(...), char key, int pressed)
      // En C (x86), los argumentos van a la pila en orden inverso.

      user_stack_ptr -= 1; *user_stack_ptr = pressed;               // Arg3: pressed
      user_stack_ptr -= 1; *user_stack_ptr = (unsigned int)key;     // Arg2: key
      user_stack_ptr -= 1; *user_stack_ptr = (unsigned int)t->keyboard_func; // Arg1: puntero función
      
      // Fake Return Address (si el wrapper hace RET, crashea, pero el wrapper debería llamar a int 0x2b)
      user_stack_ptr -= 1; *user_stack_ptr = 0;                     

      // --- PASO D: MODIFICAR CONTEXTO DE RETORNO (HIJACKING) ---
      // Engañamos al IRET. Le decimos: "No vuelvas donde estabas.
      // Vuelve a 'libc_keyboard_wrapper' usando la pila auxiliar".
      
      kernel_stack[-5] = (unsigned long)libc_keyboard_wrapper; // Nuevo EIP
      kernel_stack[-2] = (unsigned long)user_stack_ptr;        // Nuevo ESP

      // FIN: Al terminar esta función, se ejecuta el epílogo de la interrupción 
      // (RESTORE_ALL + IRET) y la CPU salta al wrapper.
  } 
  else {
      // 3. Comportamiento Legacy (si no hay función registrada)
      // Si quieres mantener el print por pantalla cuando no hay handler:
      if (pressed) {
          printc_xy(0, 0, char_map[keycode]);
      }
  }
}

void setInterruptHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE INTERRUPTION GATE FLAGS:                          R1: pg. 5-11  */
  /* ***************************                                         */
  /* flags = x xx 0x110 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}

void setTrapHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE TRAP GATE FLAGS:                                  R1: pg. 5-11  */
  /* ********************                                                */
  /* flags = x xx 0x111 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);

  //flags |= 0x8F00;    /* P = 1, D = 1, Type = 1111 (Trap Gate) */
  /* Changed to 0x8e00 to convert it to an 'interrupt gate' and so
     the system calls will be thread-safe. */
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}

void clock_handler();
void keyboard_handler();
void system_call_handler();

void setMSR(unsigned long msr_number, unsigned long high, unsigned long low);

void setSysenter()
{
  setMSR(0x174, 0, __KERNEL_CS);
  setMSR(0x175, 0, INITIAL_ESP);
  setMSR(0x176, 0, (unsigned long)system_call_handler);
}

void setIdt()
{
  /* Program interrups/exception service routines */
  idtR.base  = (DWord)idt;
  idtR.limit = IDT_ENTRIES * sizeof(Gate) - 1;
  
  set_handlers();

  /* ADD INITIALIZATION CODE FOR INTERRUPT VECTOR */
  setInterruptHandler(32, clock_handler, 0);
  setInterruptHandler(33, keyboard_handler, 0);

  setSysenter();

  set_idt_reg(&idtR);
}

