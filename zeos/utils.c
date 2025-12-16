#include <utils.h>
#include <types.h>
#include <sched.h>
#include <mm_address.h>

void copy_data(void *start, void *dest, int size)
{
  DWord *p = start, *q = dest;
  Byte *p1, *q1;
  while(size > 4) {
    *q++ = *p++;
    size -= 4;
  }
  p1=(Byte*)p;
  q1=(Byte*)q;
  while(size > 0) {
    *q1++ = *p1++;
    size --;
  }
}

int copy_from_user(void *start, void *dest, int size)
{
  DWord *p = start, *q = dest;
  Byte *p1, *q1;
  while(size > 4) {
    *q++ = *p++;
    size -= 4;
  }
  p1=(Byte*)p;
  q1=(Byte*)q;
  while(size > 0) {
    *q1++ = *p1++;
    size --;
  }
  return 0;
}

int copy_to_user(void *start, void *dest, int size)
{
  DWord *p = start, *q = dest;
  Byte *p1, *q1;
  while(size > 4) {
    *q++ = *p++;
    size -= 4;
  }
  p1=(Byte*)p;
  q1=(Byte*)q;
  while(size > 0) {
    *q1++ = *p1++;
    size --;
  }
  return 0;
}

/* access_ok: Checks if a user space pointer is valid */
int access_ok(int type, const void * addr, unsigned long size)
{
    unsigned long start = (unsigned long)addr;
    unsigned long end = start + size;

    /* Comprovacion de overflow */
    if (end < start) return 0;

    unsigned long user_start = L_USER_START;
    unsigned long global_end = (PAG_LOG_INIT_DATA + NUM_PAG_DATA) << 12;
    int is_global = (start >= user_start && end <= global_end);

    /* Proteccion del area de codigo contra escritura */
    if (type == VERIFY_WRITE) {
      unsigned long code_end = (USER_FIRST_PAGE + NUM_PAG_CODE) << 12;
      if (is_global && (start < code_end)) return 0;
    }
    
    if (is_global) return 1;

    /* permite acceso a pila auxiliar (Milestone 2)*/
    unsigned long aux_start = PAG_LOG_INIT_AUX_STACK << 12;
    unsigned long aux_end = (PAG_LOG_INIT_AUX_STACK + 1) << 12;
    if (start >= aux_start && end <= aux_end) return 1;

    /* Comprobar si la dirección pertenece a alguna pila de usuario activa */
    struct task_struct *t;
    for (int i = 0; i < NR_TASKS; i++) {
      t = &(task[i].task);
      
      if (t->PID != -1 && t->PAG_INICI > 0) {
        unsigned long stack_start = t->PAG_INICI << 12;
        unsigned long stack_end = (t->PAG_INICI + t->STACK_PAGES) << 12;

        if (start >= stack_start && end <= stack_end) {
            return 1;
        }
      }
  }

    return 0;
}

#define CYCLESPERTICK 109000

#define do_div(n,base) ({ \
        unsigned long __upper, __low, __high, __mod, __base; \
        __base = (base); \
        asm("":"=a" (__low), "=d" (__high):"A" (n)); \
        __upper = __high; \
        if (__high) { \
                __upper = __high % (__base); \
                __high = __high / (__base); \
        } \
        asm("divl %2":"=a" (__low), "=d" (__mod):"rm" (__base), "0" (__low), "1" (__upper)); \
        asm("":"=A" (n):"a" (__low),"d" (__high)); \
        __mod; \
})


#define rdtsc(low,high) \
        __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high))

unsigned long get_ticks(void) {
        unsigned long eax;
        unsigned long edx;
        unsigned long long ticks;

        rdtsc(eax,edx);

        ticks=((unsigned long long) edx << 32) + eax;
        do_div(ticks,CYCLESPERTICK);

        return ticks;
}

void memset(void *s, unsigned char c, int size)
{
  unsigned char *m=(unsigned char *)s;
  int i;
  for (i=0; i<size; i++)
  {
    m[i]=c;
  }
}