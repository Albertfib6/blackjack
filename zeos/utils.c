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
    unsigned long addr_i = (((unsigned long)addr) >> 12);
    unsigned long addr_f = (((unsigned long)addr + size) >> 12);

    if (addr_f < addr_i) return 0; 

    switch(type)
    {
        case VERIFY_WRITE:
            if ((addr_i >= USER_FIRST_PAGE) && (addr_f < TOTAL_PAGES)) return 1;
            break;
        default:
            if ((addr_i >= USER_FIRST_PAGE) && (addr_f < TOTAL_PAGES)) return 1;
            break;
    }

    // Si no es ninguno de los anteriores, acceso denegado.
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