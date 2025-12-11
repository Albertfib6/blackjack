/*
 * sched.h - Estructures i macros pel tractament de processos
 */

#ifndef __SCHED_H__
#define __SCHED_H__

#include <list.h>
#include <types.h>
#include <mm_address.h>
#include <stats.h>


#define NR_TASKS      10
#define KERNEL_STACK_SIZE	1024

// Definimos una dirección lógica fija para la Pila Auxiliar
#define PAG_LOG_INIT_AUX_STACK 500
#define AUX_STACK_SIZE 1 // 1 página
#define  SW_HW_CONTEXT 16
#define SCREEN_MEM 0xb8000


#define STACK_EBX (KERNEL_STACK_SIZE - 16)     
#define STACK_ECX (KERNEL_STACK_SIZE - 15)     
#define STACK_EDX (KERNEL_STACK_SIZE - 14)     
#define STACK_ESI (KERNEL_STACK_SIZE - 13)     
#define STACK_EDI (KERNEL_STACK_SIZE - 12)     
#define STACK_EBP (KERNEL_STACK_SIZE - 11)     
#define STACK_EAX (KERNEL_STACK_SIZE - 10)      
#define STACK_DS (KERNEL_STACK_SIZE - 9)        
#define STACK_ES (KERNEL_STACK_SIZE - 8)        
#define STACK_FS (KERNEL_STACK_SIZE - 7)        
#define STACK_GS (KERNEL_STACK_SIZE - 6)       
#define STACK_USER_EIP (KERNEL_STACK_SIZE - 5)  
#define STACK_USER_CS (KERNEL_STACK_SIZE - 4)   
#define STACK_EFLAGS (KERNEL_STACK_SIZE - 3)    
#define STACK_USER_ESP (KERNEL_STACK_SIZE - 2)  
#define STACK_USER_SS (KERNEL_STACK_SIZE - 1)  
#define STACK_RET_ADDR (KERNEL_STACK_SIZE - 18)
#define STACK_FAKE_EBP (KERNEL_STACK_SIZE - 19) 

enum state_t { ST_RUN, ST_READY, ST_BLOCKED };

struct task_struct {
  int PID;			/* Process ID. This MUST be the first field of the struct. */
  int TID;		
  page_table_entry * dir_pages_baseAddr;
  struct list_head list;	/* Task struct enqueuing */
  int register_esp;		/* position in the stack */
  enum state_t state;		/* State of the process */
  int total_quantum;		/* Total quantum of the process */
  struct stats p_stats;		/* Process stats */ 
  int PAG_INICI; /* Cima de la pila*/
  int STACK_PAGES;
  int errno;  /*errno por thread*/

/* Soporte KeyboardEvent */
  void (*keyboard_func)(char, int); // Puntero a la función del usuario         
  int in_keyboard_handler;          // Flag: 1 si está ejecutando el evento, 0 si no
  void *keyboard_wrapper; //wrapper que ejecutara func y resume execution
  unsigned long aux_stack;  // Pila auxiliar para ejecutar func
  unsigned long ctx_guardat[SW_HW_CONTEXT];  // Para guardar todo el contexto hw+sw


};

union task_union {
  struct task_struct task;
  unsigned long stack[KERNEL_STACK_SIZE];    /* pila de sistema, per procés */
};

extern union task_union protected_tasks[NR_TASKS+2];
extern union task_union *task; /* Vector de tasques */
extern struct task_struct *idle_task;


#define KERNEL_ESP(t)       	(DWord) &(t)->stack[KERNEL_STACK_SIZE]

#define INITIAL_ESP       	KERNEL_ESP(&task[1])

extern struct list_head freequeue;
extern struct list_head readyqueue;
extern struct list_head tick_queue;

/* Inicialitza les dades del proces inicial */
void init_task1(void);

void init_idle(void);

void init_sched(void);

void schedule(void);

struct task_struct * current();

void task_switch(union task_union*t);
void switch_stack(int * save_sp, int new_sp);

void sched_next_rr(void);

void force_task_switch(void);

struct task_struct *list_head_to_task_struct(struct list_head *l);

int allocate_DIR(struct task_struct *t);

page_table_entry * get_PT (struct task_struct *t) ;

page_table_entry * get_DIR (struct task_struct *t) ;

/* Headers for the scheduling policy */
void sched_next_rr();
void update_process_state_rr(struct task_struct *t, struct list_head *dest);
int needs_sched_rr();
void update_sched_data_rr();

void init_stats(struct stats *s);

#endif  /* __SCHED_H__ */
