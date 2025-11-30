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

// Definimos una dirección lógica fija para la Pila Auxiliar (ej. después del código/datos)
#define PAG_LOG_INIT_AUX_STACK  (PAG_LOG_INIT_DATA + NUM_PAG_DATA + 50) // Lejos de la pila normal
#define AUX_STACK_SIZE 1 // 1 página

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
  unsigned long saved_eip;          // Para guardar dónde estaba el thread
  unsigned long saved_esp;          // Para guardar la pila original del thread
  int in_keyboard_handler;          // Flag: 1 si está ejecutando el evento, 0 si no
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
