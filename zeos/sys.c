/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <p_stats.h>

#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1
// Definimos el tamaño máximo que podrá crecer la pila 
#define MAX_USER_STACK_PAGES 5 

void * get_ebp();
extern void ret_from_fork();
void thread_wrapper();


int check_fd(int fd, int permissions)
{
  if (fd!=1){
    current()->errno = EBADF;
    return -1;
  } 
  if (permissions!=ESCRIPTURA){
    current()->errno = EACCES;
    return -1;
  } 
  return 0;
}

void user_to_system(void)
{
  update_stats(&(current()->p_stats.user_ticks), &(current()->p_stats.elapsed_total_ticks));
}

void system_to_user(void)
{
  update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
}

int sys_ni_syscall()
{
	  current()->errno = ENOSYS;
    return -1;  
}

int sys_getpid()
{
	return current()->PID;
}

int next_free_id = 1000;


// int sys_fork(void)
// {
//   struct list_head *lhcurrent = NULL;
//   union task_union *uchild;
  
//   /* Any free task_struct? */
//   if (list_empty(&freequeue)) return -ENOMEM;

//   lhcurrent=list_first(&freequeue);
  
//   list_del(lhcurrent);
  
//   uchild=(union task_union*)list_head_to_task_struct(lhcurrent);
  
//   /* Copy the parent's task struct to child's */
//   copy_data(current(), uchild, sizeof(union task_union));
  
//   /* new pages dir */
//   allocate_DIR((struct task_struct*)uchild);
  
//   /* Allocate pages for DATA+STACK */
//   int new_ph_pag, pag, i;
//   page_table_entry *process_PT = get_PT(&uchild->task);
//   for (pag=0; pag<NUM_PAG_DATA; pag++)
//   {
//     new_ph_pag=alloc_frame();
//     if (new_ph_pag!=-1) /* One page allocated */
//     {
//       set_ss_pag(process_PT, PAG_LOG_INIT_DATA+pag, new_ph_pag);
//     }
//     else /* No more free pages left. Deallocate everything */
//     {
//       /* Deallocate allocated pages. Up to pag. */
//       for (i=0; i<pag; i++)
//       {
//         free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
//         del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
//       }
//       /* Deallocate task_struct */
//       list_add_tail(lhcurrent, &freequeue);
      
//       /* Return error */
//       return -EAGAIN; 
//     }
//   }

//   /* Copy parent's SYSTEM and CODE to child. */
//   page_table_entry *parent_PT = get_PT(current());
//   for (pag=0; pag<NUM_PAG_KERNEL; pag++)
//   {
//     set_ss_pag(process_PT, pag, get_frame(parent_PT, pag));
//   }
//   for (pag=0; pag<NUM_PAG_CODE; pag++)
//   {
//     set_ss_pag(process_PT, PAG_LOG_INIT_CODE+pag, get_frame(parent_PT, PAG_LOG_INIT_CODE+pag));
//   }
//   /* Copy parent's DATA to child. We will use TOTAL_PAGES-1 as a temp logical page to map to */
//   for (pag=NUM_PAG_KERNEL+NUM_PAG_CODE; pag<NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA; pag++)
//   {
//     /* Map one child page to parent's address space. */
//     set_ss_pag(parent_PT, pag+NUM_PAG_DATA, get_frame(process_PT, pag));
//     copy_data((void*)(pag<<12), (void*)((pag+NUM_PAG_DATA)<<12), PAGE_SIZE);
//     del_ss_pag(parent_PT, pag+NUM_PAG_DATA);
//   }
//   /* Deny access to the child's memory space */
//   set_cr3(get_DIR(current()));

//   uchild->task.PID=++global_PID;
//   uchild->task.state=ST_READY;

//   int register_ebp;		/* frame pointer */
//   /* Map Parent's ebp to child's stack */
//   register_ebp = (int) get_ebp();
//   register_ebp=(register_ebp - (int)current()) + (int)(uchild);

//   uchild->task.register_esp=register_ebp + sizeof(DWord);

//   DWord temp_ebp=*(DWord*)register_ebp;
//   /* Prepare child stack for context switch */
//   uchild->task.register_esp-=sizeof(DWord);
//   *(DWord*)(uchild->task.register_esp)=(DWord)&ret_from_fork;
//   uchild->task.register_esp-=sizeof(DWord);
//   *(DWord*)(uchild->task.register_esp)=temp_ebp;

//   /* Set stats to 0 */
//   init_stats(&(uchild->task.p_stats));

//   /* Queue child process into readyqueue */
//   uchild->task.state=ST_READY;
//   list_add_tail(&(uchild->task.list), &readyqueue);
  
//   return uchild->task.PID;
// }

int sys_fork(void)
{
  struct list_head *lhcurrent = NULL;
  union task_union *uchild;
  
  /* Any free task_struct? */
  if (list_empty(&freequeue)) {
    current()->errno = ENOMEM;
    return -1;
  } 

  lhcurrent=list_first(&freequeue);
  
  list_del(lhcurrent);
  
  uchild=(union task_union*)list_head_to_task_struct(lhcurrent);
  
  /* Copy the parent's task struct to child's */
  copy_data(current(), uchild, sizeof(union task_union));
  
  /* El hijo es un proceso nuevo, el hilo actual se convierte en el principal */
  uchild->task.PID = ++next_free_id;
  uchild->task.TID = uchild->task.PID;
  uchild->task.state=ST_READY;
  uchild->task.errno = 0;
  
  /* new pages dir */
  allocate_DIR((struct task_struct*)uchild);
  
  /* Allocate pages for DATA + STACK */
  int new_ph_pag, pag, i;
  page_table_entry *process_PT = get_PT(&uchild->task);
  page_table_entry *parent_PT = get_PT(current());

  for (pag=0; pag<NUM_PAG_DATA; pag++)
  {
    new_ph_pag=alloc_frame();
    if (new_ph_pag!=-1) 
    {
      set_ss_pag(process_PT, PAG_LOG_INIT_DATA+pag, new_ph_pag);
    }
    else 
    {
      /* Deallocate allocated pages. Up to pag. */
      for (i=0; i<pag; i++)
      {
        free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
        del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
      }
      list_add_tail(lhcurrent, &freequeue);
      current()->errno = EAGAIN;
      return -1;
    }
  }

  /* COPIA DE LA PILA DEL HILO ACTUAL (Solo esta pila) */
  int start_stack = current()->PAG_INICI;
  int num_stack = current()->STACK_PAGES;
  
  for (i = 0; i < num_stack; i++) {
      int log_page = start_stack + i;

      /* Evitar doble copia si la pila cae dentro de NUM_PAG_DATA */
      if (log_page >= PAG_LOG_INIT_DATA && log_page < PAG_LOG_INIT_DATA + NUM_PAG_DATA) {
          continue; 
      }

      /* Verificar si el padre tiene página física asignada aquí (Growth dinámico) */
      if (get_frame(parent_PT, log_page) != -1) {
          new_ph_pag = alloc_frame();
          if (new_ph_pag != -1) {
              set_ss_pag(process_PT, log_page, new_ph_pag);
         
              /* Copia de memoria: Mapeamos temp en el padre para copiar */
              int temp_page = NUM_PAG_KERNEL + NUM_PAG_CODE + NUM_PAG_DATA; // Una pág libre
              set_ss_pag(parent_PT, temp_page, get_frame(process_PT, log_page));
              copy_data((void*)(log_page<<12), (void*)(temp_page<<12), PAGE_SIZE);
              del_ss_pag(parent_PT, temp_page);
              set_cr3(get_DIR(current())); // Flush TLB
          } else {
              /* error: No hay memoria para copiar la pila completa. */
              
              /* Liberar las páginas de PILA que ya habíamos asignado en este bucle */
              for (int k = 0; k < i; k++) {
                  int undo_log_page = start_stack + k;
                  /* Solo liberamos si realmente asignamos un frame (verificamos en el hijo) */
                  if (get_frame(process_PT, undo_log_page) != -1) {
                      free_frame(get_frame(process_PT, undo_log_page));
                      del_ss_pag(process_PT, undo_log_page);
                  }
              }

              /* Liberar las páginas de DATOS GLOBALES asignadas en el paso anterior */
              for (int k = 0; k < NUM_PAG_DATA; k++) {
                  free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA + k));
                  del_ss_pag(process_PT, PAG_LOG_INIT_DATA + k);
              }

              /* Liberar el task_struct (devolver a la cola de libres) */
              list_add_tail(lhcurrent, &freequeue);

              /* Retornar código de error */
              current()->errno = EAGAIN;
              return -1;
          }
      }
  }

  /* Copy parent's SYSTEM and CODE to child. */
  for (pag=0; pag<NUM_PAG_KERNEL; pag++)
  {
    set_ss_pag(process_PT, pag, get_frame(parent_PT, pag));
  }
  for (pag=0; pag<NUM_PAG_CODE; pag++)
  {
    set_ss_pag(process_PT, PAG_LOG_INIT_CODE+pag, get_frame(parent_PT, PAG_LOG_INIT_CODE+pag));
  }
  
  /* Copy parent's DATA content to child. */
  for (pag=NUM_PAG_KERNEL+NUM_PAG_CODE; pag<NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA; pag++)
  {
    set_ss_pag(parent_PT, pag+NUM_PAG_DATA, get_frame(process_PT, pag));
    copy_data((void*)(pag<<12), (void*)((pag+NUM_PAG_DATA)<<12), PAGE_SIZE);
    del_ss_pag(parent_PT, pag+NUM_PAG_DATA);
  }
  
  /* Deny access to the child's memory space */
  set_cr3(get_DIR(current()));

  /* Configuración del Kernel Stack para el Context Switch (ret_from_fork) */
  int register_ebp;  
  register_ebp = (int) get_ebp();
  register_ebp=(register_ebp - (int)current()) + (int)(uchild);

  uchild->task.register_esp=register_ebp + sizeof(DWord);

  DWord temp_ebp=*(DWord*)register_ebp;
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=(DWord)&ret_from_fork;
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=temp_ebp;

  /* Set stats to 0 */
  init_stats(&(uchild->task.p_stats));

  /* Queue child process into readyqueue */
  list_add_tail(&(uchild->task.list), &readyqueue);
  
  return uchild->task.PID;
}

#define TAM_BUFFER 512

int sys_write(int fd, char *buffer, int nbytes) {
char localbuffer [TAM_BUFFER];
int bytes_left;
int ret;

	if ((ret = check_fd(fd, ESCRIPTURA)))
		return ret;
	if (nbytes < 0) {
    current()->errno = EINVAL;
    return -1;
  }  
	if (!access_ok(VERIFY_READ, buffer, nbytes)){
    current()->errno = EFAULT;
    return -1;
  }
	bytes_left = nbytes;
	while (bytes_left > TAM_BUFFER) {
		copy_from_user(buffer, localbuffer, TAM_BUFFER);
		ret = sys_write_console(localbuffer, TAM_BUFFER);
		bytes_left-=ret;
		buffer+=ret;
	}
	if (bytes_left > 0) {
		copy_from_user(buffer, localbuffer,bytes_left);
		ret = sys_write_console(localbuffer, bytes_left);
		bytes_left-=ret;
	}
	return (nbytes-bytes_left);
}


extern int zeos_ticks;

int sys_gettime()
{
  return zeos_ticks;
}

// void sys_exit()
// {  
//   int i;

//   page_table_entry *process_PT = get_PT(current());

//   // Deallocate all the propietary physical pages
//   for (i=0; i<NUM_PAG_DATA; i++)
//   {
//     free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
//     del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
//   }
  
//   /* Free task_struct */
//   list_add_tail(&(current()->list), &freequeue);
  
//   current()->PID=-1;
  
//   /* Restarts execution of the next process */
//   sched_next_rr();
// }

void sys_exit()
{  
  int pid_dead = current()->PID;
  struct task_struct *t;
  page_table_entry *PT = get_PT(current());

  // Deallocate all the propietary physical pages
  for (int i=0; i<NUM_PAG_DATA; i++)
  {
   free_frame(get_frame(PT, PAG_LOG_INIT_DATA+i));
   del_ss_pag(PT, PAG_LOG_INIT_DATA+i);
  }

  /* 2. Recorrer TODAS las tareas para encontrar hilos del mismo proceso */
  for (int i = 0; i < NR_TASKS; i++) {
      t = &(task[i].task);
      if (t->STACK_PAGES > 0 && t->PAG_INICI > 0) {
      /* Si la tarea pertenece al proceso que muere (mismo PID) y está en uso */
      if (t->PID == pid_dead && t->PID != -1) {
          /* Liberar su pila específica */
          page_table_entry *thread_PT = get_PT(t); // Comparten DIR, es el mismo
          for (int j = 0; j < t->STACK_PAGES; j++) {
              int page = t->PAG_INICI + j;
              int frame = get_frame(thread_PT, page);
              if (frame != -1) {
                  free_frame(frame);
                  del_ss_pag(thread_PT, page);
              }
          }
        t->STACK_PAGES = 0; 
        t->PAG_INICI = 0;
        }
          t->PID = -1;
          t->TID = -1;
          list_del(&(t->list));
          list_add_tail(&(t->list), &freequeue);
      }
  }
  
  /* Cambio de contexto */
  sched_next_rr();
}

/* System call to force a task switch */
int sys_yield()
{
  force_task_switch();
  return 0;
}

extern int remaining_quantum;

int sys_get_stats(int pid, struct stats *st)
{
  int i;
  
  if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats))){
    current()->errno = EFAULT;
    return -1;
  }
  
  if (pid<0) { 
    current()->errno = EINVAL;
    return -1;
  }
  for (i=0; i<NR_TASKS; i++)
  {
    if (task[i].task.PID==pid)
    {
      task[i].task.p_stats.remaining_ticks=remaining_quantum;
      copy_to_user(&(task[i].task.p_stats), st, sizeof(struct stats));
      return 0;
    }
  }
  
  current()->errno = ESRCH;
  return -1;
}

int sys_ThreadCreate(void (*function)(void* arg), void* parameter, void* thread_wrapper) {
  struct task_struct *uchild_struct;
  struct list_head *lhcurrent = NULL;
  union task_union *uchild;
  
  if (list_empty(&freequeue)) {
    current()->errno = ENOMEM;
    return -1;
  } 

  lhcurrent = list_first(&freequeue);
  list_del(lhcurrent);
  
  uchild = (union task_union*)list_head_to_task_struct(lhcurrent);
  uchild_struct = &uchild->task;
  
  copy_data(current(), uchild, sizeof(union task_union));
  
  uchild_struct->TID = ++next_free_id;
  uchild_struct->PID = current()->PID; 
  uchild_struct->state = ST_READY;
  uchild_struct->errno = 0; 

  page_table_entry *current_PT = get_PT(current());
  int stack_ini = -1;
  int pages_needed = MAX_USER_STACK_PAGES;

  /* Buscamos hueco en memoria para almacenar la pila del thread nuevo */
  for (int start = PAG_LOG_INIT_DATA + NUM_PAG_DATA*2; start < TOTAL_PAGES; start++) {
      int free = 1;
      for (int offset = 0; offset < pages_needed; offset++) {
          /* Comprobamos que la página no este mapeada a una física */
          if (current_PT[start+offset].bits.present) { 
              free = 0; 
              start += offset; 
              break;
          }
      }
      if (free) { 
          stack_ini = start; 
          break; 
      }
  }

  /* Si no encontramos hueco devolvemos error de memoria */
  if (stack_ini == -1) {
      list_add_tail(lhcurrent, &freequeue);
      current()->errno = ENOMEM;
      return -1;
  } 

  uchild_struct->PAG_INICI = stack_ini; 
  uchild_struct->STACK_PAGES = pages_needed; 

  /* Asignación de memoria física */
  int logical_page_top = stack_ini + pages_needed - 1;
  int new_ph_pag = alloc_frame();
  
  if (new_ph_pag == -1) { list_add_tail(lhcurrent, &freequeue); 
    current()->errno = EAGAIN;
    return -1;
  }
  
  set_ss_pag(current_PT, logical_page_top, new_ph_pag);
  set_cr3(get_DIR(current())); 

  /* Pila de usuario */
  unsigned int user_stack_base = (logical_page_top + 1) << 12;
  unsigned int *stack_ptr = (unsigned int *)user_stack_base;
  
  stack_ptr -= 1; *stack_ptr = (unsigned int)parameter; 
  stack_ptr -= 1; *stack_ptr = (unsigned int)function;   
  stack_ptr -= 1; *stack_ptr = 0; 

  /* Pila de sistema */
  unsigned long *kstack = (unsigned long *)&uchild->stack[KERNEL_STACK_SIZE];
  
  /* Contexto HW */
  kstack -= 1;              // SS 
  kstack -= 1; *kstack = (unsigned long)stack_ptr; // ESP, puntero a la cima de usuario mencionada antes
  kstack -= 1;            // EFLAGS
  kstack -= 1;              // CS 
  kstack -= 1; *kstack = (unsigned long)thread_wrapper;  // EIP, ponemos la direccion del wrapper que llamara a la funcion y a exit

  uchild->task.register_esp = (unsigned long)&uchild->stack[KERNEL_STACK_SIZE-18];


  init_stats(&uchild_struct->p_stats);
  list_add_tail(&uchild_struct->list, &readyqueue);

  return uchild_struct->TID; /* Devolvemos el id del nuevo thread */
}



void sys_ThreadExit() {

    struct task_struct *current_task = current();
    page_table_entry *current_PT = get_PT(current_task);

    // Recorremos los STACK_PAGES asignados a la pila. 
    for (int i = 0; i < current_task->STACK_PAGES; i++) {
        int page_num = current_task->PAG_INICI + i;
        int frame = get_frame(current_PT, page_num);
        if (frame != -1) { // Verificamos que realmente haya un frame asignado.
            free_frame(frame);
            del_ss_pag(current_PT, page_num);
        }
    }
    
    current_task->TID = -1;

    list_add_tail(&(current_task->list), &freequeue);  
    sched_next_rr();
}

int sys_KeyboardEvent(void (*func), void *wrapper) {
    struct task_struct *t = current();

	if (t->aux_stack != NULL) {
        return 0; /* Already allocated */
    }
    // 1. Registrar la función
    t->keyboard_func = func;
    t->keyboard_wrapper = wrapper;
    t->in_keyboard_handler = 0;

	int frame = alloc_frame();
    if (frame < 0) {
        return -ENOMEM;
    }

    /* Map the frame to a fixed virtual page in user space */
    page_table_entry *PT = get_PT(t);
    set_ss_pag(PT, PAG_LOG_INIT_AUX_STACK, frame);

    /* Flush TLB */
    set_cr3(get_DIR(t));

    /* Stack top is at the end of the page (stacks grow downward) */
     t->aux_stack = (PAG_LOG_INIT_AUX_STACK+1) << 12;

    return 0;
}

int sys_resume_execution() {
    struct task_struct *t = current();
    union task_union *u = (union task_union *)t; 


    // Solo hacemos algo si venimos de un evento de teclado
    if (t->in_keyboard_handler == 1) {
        
        // --- RESTAURAR CONTEXTO ORIGINAL ---
        
        // Necesitamos acceder a la pila del kernel donde se guardó el estado 
        // al entrar en ESTA syscall (int 0x2b)
        unsigned long *kernel_stack = (unsigned long *)&u->stack[KERNEL_STACK_SIZE];
        
        // Restauramos el EIP y ESP que guardamos en keyboard_routine
        kernel_stack[-5] = t->saved_eip;
        kernel_stack[-2] = t->saved_esp;
        
        // Desactivamos el flag
        t->in_keyboard_handler = 0;
        
        // Al hacer IRET desde esta syscall, la CPU volverá 
        // exactamente donde estaba el thread antes de pulsar la tecla.
    }
    
    return 0;
}

int is_in_keyboard_handler() {
    return current()->in_keyboard_handler;
}

int sys_get_errno()
{
    return current()->errno;
}
