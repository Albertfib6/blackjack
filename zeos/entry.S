/*
 * entry.S - Entry point to system mode from user mode
 */

#include <asm.h>
#include <segment.h>
#include <errno.h>


/**************************************************/
/**** Save & Restore ******************************/
/**                                              **/
/** When we change to privilege level 0 (kernel) **/
/** (through an interrupt, a system call, an     **/
/** exception ...) we must save the state of the **/
/** currently running task (save).               **/
/**                                              **/
/** Stack layout in 'systemCall':                **/
/**                                              **/
/**   0(%esp) - %ebx    \                        **/
/**   4(%esp) - %ecx     |                       **/
/**   8(%esp) - %edx     |                       **/
/**   C(%esp) - %esi     | Register saved        **/
/**  10(%esp) - %edi     |  by 'save'            **/
/**  14(%esp) - %ebp     |                       **/
/**  18(%esp) - %eax     |                       **/
/**  1C(%esp) - %ds      |                       **/
/**  20(%esp) - %es      |                       **/
/**  24(%esp) - %fs      |                       **/
/**  28(%esp) - %gs     /                        **/
/**  2C(%esp) - %eip    \                        **/
/**  30(%esp) - %cs      |                       **/
/**  34(%esp) - %eflags  |  Return context saved **/
/**  38(%esp) - %oldesp  |   by the processor.   **/
/**  3C(%esp) - %oldss  /                        **/
/**                                              **/
/**************************************************/

#define SAVE_ALL \
      pushl %gs; \
      pushl %fs; \
      pushl %es; \
      pushl %ds; \
      pushl %eax; \
      pushl %ebp; \
      pushl %edi; \
      pushl %esi; \
      pushl %edx; \
      pushl %ecx; \
      pushl %ebx; \
      movl $__KERNEL_DS, %edx;    \
      movl %edx, %ds;           \
      movl %edx, %es

#define RESTORE_ALL \
      popl %ebx;   \
      popl %ecx;   \
      popl %edx;   \
      popl %esi;   \
      popl %edi;   \
      popl %ebp;   \
      popl %eax;   \
      popl %ds;    \
      popl %es;    \
      popl %fs;    \
      popl %gs;

#define EOI \
      movb $0x20, %al; \
      outb %al, $0x20;

ENTRY(clock_handler)
      SAVE_ALL
      pushl %eax;
      call user_to_system;
      popl %eax;
      EOI
      call clock_routine;
      pushl %eax;
      call system_to_user;
      popl %eax;
      RESTORE_ALL
      iret;

ENTRY(keyboard_handler)
      SAVE_ALL
      pushl %eax;
      call user_to_system;
      popl %eax;
      EOI
      call keyboard_routine;
      pushl %eax;
      call system_to_user;
      popl %eax;
      RESTORE_ALL
      iret;

ENTRY(system_call_handler)
    push $__USER_DS
    push %ebp
    pushfl
    push $__USER_CS
    push 4(%ebp)
    SAVE_ALL    // Save registers

    /* --- INICIO MODIFICACIÓN: Chequeo de Bloqueo --- */
    
    /* 1. Guardamos EAX (número de syscall) porque 'call' lo va a machacar */
    pushl %eax
    
    /* 2. Preguntamos si estamos dentro del handler de teclado */
    call is_in_keyboard_handler
    
    /* 3. Comprobamos el resultado (en EAX) */
    cmpl $0, %eax
    jne sysenter_einprogress /* Si no es 0, estamos bloqueados -> Salta */
    
    /* 4. Si es 0 (no bloqueado), restauramos el número de syscall y seguimos */
    popl %eax
    
    /* --- FIN MODIFICACIÓN --- */

    cmpl $0, %eax
    jl sysenter_err
    cmpl $MAX_SYSCALL, %eax
    jg sysenter_err
    call *sys_call_table(, %eax, 0x04)
    jmp sysenter_fin

sysenter_err:
    pushl $ENOSYS
    call update_errno_to_current
    addl $4, %esp
    
    /* 2. Devolver -1 al wrapper */
    movl $-1, %eax
    jmp sysenter_fin

/* Nueva etiqueta para el error de progreso */
sysenter_einprogress:

    addl $4, %esp 
    /* Llamamos a la función C para guardar el error en el TLS */
    pushl $EINPROGRESS          ; Argumento: El código de error positivo
    call update_errno_to_current
    addl $4, %esp               ; Limpiar argumento
    
    /* Preparamos el valor de retorno (-1) para el wrapper */
    movl $-1, %eax
    
    /* Saltamos al final para restaurar contexto y salir */
    jmp sysenter_fin

    /* Dejamos que caiga ("fallthrough") a sysenter_fin */

sysenter_fin:
    /* Sobrescribimos el EAX guardado en la pila con el valor de retorno */
    movl %eax, 0x18(%esp) 
    RESTORE_ALL
    movl (%esp), %edx   // Return address
    movl 12(%esp), %ecx // User stack address
    sti
    sysexit

ENTRY(page_fault_handler)

    SAVE_ALL

    /* 2. Obtener los parámetros para la función de C */
    
    movl %cr2, %eax      // Leer la dirección que causó el fallo desde CR2
    pushl %eax           // 2ndo Argumento: fault_address
    
    /* El Error Code fue empujado por la CPU automáticamente antes de saltar aquí.
       Está en la pila, pero "enterrado" bajo los registros de SAVE_ALL */

    pushl 0x30(%esp)     // 1r Argumento: error_code 

    call page_fault_routine

    /* 4. Limpiar argumentos de la llamada */
    addl $8, %esp 

    RESTORE_ALL

    
    /* La excepción 14 empuja un código de error en la pila. */
       
    addl $4, %esp 

    iret

  
ENTRY(ret_from_fork)
    RESTORE_ALL
    iret

ENTRY(handler_resume_execution)
    SAVE_ALL                    // Guardar registros (contexto software)
    
    call sys_resume_execution   // Llamar a la lógica de restauración en C
                                // (Esta función modificará el EIP/ESP guardados en la pila)
    RESTORE_ALL                 // Restaurar registros
    iret                        // Volver a usuario (usará el EIP/ESP modificados)
    






