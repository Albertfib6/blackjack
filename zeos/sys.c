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
// Definimos el tamaño máximo que podrá crecer la pila y el gap de seguridad
#define MAX_USER_STACK_PAGES 20 

void * get_ebp();

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -EBADF; 
  if (permissions!=ESCRIPTURA) return -EACCES; 
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
	return -ENOSYS; 
}

int sys_getpid()
{
	return current()->PID;
}

int global_PID=1000;

int ret_from_fork()
{
  return 0;
}

int sys_fork(void)
{
  struct list_head *lhcurrent = NULL;
  union task_union *uchild;
  
  /* Any free task_struct? */
  if (list_empty(&freequeue)) return -ENOMEM;

  lhcurrent=list_first(&freequeue);
  
  list_del(lhcurrent);
  
  uchild=(union task_union*)list_head_to_task_struct(lhcurrent);
  
  /* Copy the parent's task struct to child's */
  copy_data(current(), uchild, sizeof(union task_union));
  
  /* new pages dir */
  allocate_DIR((struct task_struct*)uchild);
  
  /* Allocate pages for DATA+STACK */
  int new_ph_pag, pag, i;
  page_table_entry *process_PT = get_PT(&uchild->task);
  for (pag=0; pag<NUM_PAG_DATA; pag++)
  {
    new_ph_pag=alloc_frame();
    if (new_ph_pag!=-1) /* One page allocated */
    {
      set_ss_pag(process_PT, PAG_LOG_INIT_DATA+pag, new_ph_pag);
    }
    else /* No more free pages left. Deallocate everything */
    {
      /* Deallocate allocated pages. Up to pag. */
      for (i=0; i<pag; i++)
      {
        free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
        del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
      }
      /* Deallocate task_struct */
      list_add_tail(lhcurrent, &freequeue);
      
      /* Return error */
      return -EAGAIN; 
    }
  }

  /* Copy parent's SYSTEM and CODE to child. */
  page_table_entry *parent_PT = get_PT(current());
  for (pag=0; pag<NUM_PAG_KERNEL; pag++)
  {
    set_ss_pag(process_PT, pag, get_frame(parent_PT, pag));
  }
  for (pag=0; pag<NUM_PAG_CODE; pag++)
  {
    set_ss_pag(process_PT, PAG_LOG_INIT_CODE+pag, get_frame(parent_PT, PAG_LOG_INIT_CODE+pag));
  }
  /* Copy parent's DATA to child. We will use TOTAL_PAGES-1 as a temp logical page to map to */
  for (pag=NUM_PAG_KERNEL+NUM_PAG_CODE; pag<NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA; pag++)
  {
    /* Map one child page to parent's address space. */
    set_ss_pag(parent_PT, pag+NUM_PAG_DATA, get_frame(process_PT, pag));
    copy_data((void*)(pag<<12), (void*)((pag+NUM_PAG_DATA)<<12), PAGE_SIZE);
    del_ss_pag(parent_PT, pag+NUM_PAG_DATA);
  }
  /* Deny access to the child's memory space */
  set_cr3(get_DIR(current()));

  uchild->task.PID=++global_PID;
  uchild->task.state=ST_READY;

  int register_ebp;		/* frame pointer */
  /* Map Parent's ebp to child's stack */
  register_ebp = (int) get_ebp();
  register_ebp=(register_ebp - (int)current()) + (int)(uchild);

  uchild->task.register_esp=register_ebp + sizeof(DWord);

  DWord temp_ebp=*(DWord*)register_ebp;
  /* Prepare child stack for context switch */
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=(DWord)&ret_from_fork;
  uchild->task.register_esp-=sizeof(DWord);
  *(DWord*)(uchild->task.register_esp)=temp_ebp;

  /* Set stats to 0 */
  init_stats(&(uchild->task.p_stats));

  /* Queue child process into readyqueue */
  uchild->task.state=ST_READY;
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
	if (nbytes < 0)
		return -EINVAL;
	if (!access_ok(VERIFY_READ, buffer, nbytes))
		return -EFAULT;
	
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

void sys_exit()
{  
  int i;

  page_table_entry *process_PT = get_PT(current());

  // Deallocate all the propietary physical pages
  for (i=0; i<NUM_PAG_DATA; i++)
  {
    free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
    del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
  }
  
  /* Free task_struct */
  list_add_tail(&(current()->list), &freequeue);
  
  current()->PID=-1;
  
  /* Restarts execution of the next process */
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
  
  if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats))) return -EFAULT; 
  
  if (pid<0) return -EINVAL;
  for (i=0; i<NR_TASKS; i++)
  {
    if (task[i].task.PID==pid)
    {
      task[i].task.p_stats.remaining_ticks=remaining_quantum;
      copy_to_user(&(task[i].task.p_stats), st, sizeof(struct stats));
      return 0;
    }
  }
  return -ESRCH; /*ESRCH */
}


int sys_ThreadCreate(void (*function)(void* arg), void* parameter) {
 
  struct task_struct *uchild_struct;
  struct list_head *lhcurrent = NULL;
  union task_union *uchild;
  
  /* 1. Any free task_struct? */
  if (list_empty(&freequeue)) return -ENOMEM;

  lhcurrent = list_first(&freequeue);
  list_del(lhcurrent);
  
  uchild = (union task_union*)list_head_to_task_struct(lhcurrent);
  uchild_struct = &uchild->task;
  
  /* 2. Copy the parent's task struct to child's */
  copy_data(current(), uchild, sizeof(union task_union));
  
  // Inicializar identidad del thread
  uchild_struct->TID = ++global_TID;
  uchild_struct->PID = current()->PID; // Comparten PID
  uchild_struct->state = ST_READY;

  /* 3. Buscar hueco en la memoria lógica para la pila */
  page_table_entry *current_PT = get_PT(current());
  int stack_ini = -1;
  int pages_needed = MAX_USER_STACK_PAGES;
  
  // Empezamos a buscar después de la zona de datos
  unsigned long search_start = PAG_LOG_INIT_DATA + NUM_PAG_DATA + 10; 

  for (int start = search_start; start < TOTAL_PAGES - pages_needed; start++) {
      int free = 1;
      for (int offset = 0; offset < pages_needed; offset++) {
          if (current_PT[start + offset].bits.present || get_frame(current_PT, start + offset) != -1) {
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

  if (stack_ini == -1) {
      list_add_tail(lhcurrent, &freequeue);
      return -ENOMEM; 
  }

  /* Guardamos información para el Crecimiento Dinámico */
  uchild_struct->PAG_INICI = stack_ini; 
  uchild_struct->STACK_PAGES = pages_needed; 

  /* 4. Asignar SOLO 1 página física inicial (la cima lógica) */
  int logical_page_top = stack_ini + pages_needed - 1;
  int new_ph_pag = alloc_frame();
  
  if (new_ph_pag == -1) {
      list_add_tail(lhcurrent, &freequeue);
      return -EAGAIN;
  }
  
  set_ss_pag(current_PT, logical_page_top, new_ph_pag);

  /* 5. Preparar Pila de Usuario (Parámetro y Retorno) */
  // Dirección lineal base (techo de la pila)
  unsigned int user_stack_base = (logical_page_top + 1) << 12;
  unsigned int *stack_ptr = (unsigned int *)user_stack_base;
  
  stack_ptr -= 1; 
  *stack_ptr = (unsigned int)parameter; // Push Parámetro
  
  stack_ptr -= 1; 
  *stack_ptr = sys_ThreadExit; // Push Fake Return Address (o wrapper_exit)

  /* 6. Preparar Pila de Sistema (Kernel Stack) para task_switch */
  
  // --- CONTEXTO HARDWARE (IRET) ---
  // Reutilizamos SS, CS, EFLAGS del padre que ya están en el stack por copy_data
  
  // ESP: Apunta a la pila de usuario modificada (stack_ptr ya tiene el valor correcto)
  uchild->stack[KERNEL_STACK_SIZE - 2] = (unsigned long)stack_ptr; 
  
  // EIP: Apunta a la función del thread
  uchild->stack[KERNEL_STACK_SIZE - 5] = (unsigned long)function; 

  // --- CONTEXTO SOFTWARE (task_switch) ---
  // Tomamos la dirección justo debajo del contexto HW (EIP está en -5)
  unsigned long *ksp = &uchild->stack[KERNEL_STACK_SIZE - 5];
  
  ksp -= 10; // Espacio para registros generales (EAX, EBX...)
  ksp -= 1; 
  *ksp = 0; // Fake EBP para el POP EBP
  ksp -= 1; 
  *ksp = (unsigned long) ret_from_fork; // Dirección de retorno CLAVE

  // Guardamos el puntero final en el PCB
  uchild->task.register_esp = (unsigned long) ksp;

  /* 7. Encolar y Retornar */
  init_stats(&uchild_struct->p_stats);
  list_add_tail(&uchild_struct->list, &readyqueue);

  return uchild_struct->TID;
}
  


void sys_ThreadExit() {
   // Obtener el thread actual; se asume que current() devuelve un puntero a struct task_struct.
    struct task_struct *current_task = current();
    page_table_entry *current_PT = get_PT(current_task);

    // Recorremos los STACK_PAGES asignados a la pila. La información sobre el inicio
    // y la cantidad de páginas debe haberse guardado al crearlo.
    for (int i = 0; i < current_task->STACK_PAGES; i++) {
        int page_num = current_task->PAG_INICI + i;
        int frame = get_frame(current_PT, page_num);
        if (frame != -1) { // Verificamos que realmente haya un frame asignado.
            free_frame(frame);
            del_ss_pag(current_PT, page_num);
        }
    }
    
    current_task->TID = -1;

    //list_del(&(current_task->list));

    list_add_tail(&(current_task->list), &freequeue);  
    // 4. Llamar al scheduler para cambiar de contexto. Esta llamada no debería retornar.
    sched_next_rr();
}

