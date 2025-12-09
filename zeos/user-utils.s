# 0 "user-utils.S"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "user-utils.S"
# 1 "include/asm.h" 1
# 2 "user-utils.S" 2

.globl syscall_sysenter; .type syscall_sysenter, @function; .align 0; syscall_sysenter:
 push %ecx
 push %edx
 push $SYSENTER_RETURN
 push %ebp
 mov %esp, %ebp
 sysenter
.globl SYSENTER_RETURN; .type SYSENTER_RETURN, @function; .align 0; SYSENTER_RETURN:
 pop %ebp
 pop %edx
 pop %edx
 pop %ecx
 ret


.globl write; .type write, @function; .align 0; write:
 pushl %ebp
 movl %esp, %ebp
 pushl %ebx;
 movl $4, %eax
 movl 0x8(%ebp), %ebx;
 movl 0xC(%ebp), %ecx;
 movl 0x10(%ebp), %edx;
 call syscall_sysenter
 popl %ebx
 test %eax, %eax
 js nok
 popl %ebp
 ret


nok:
 mov $-1, %eax
 popl %ebp
 ret


.globl gettime; .type gettime, @function; .align 0; gettime:
 pushl %ebp
 movl %esp, %ebp
 movl $10, %eax
 call syscall_sysenter
 popl %ebp
 ret


.globl getpid; .type getpid, @function; .align 0; getpid:
 pushl %ebp
 movl %esp, %ebp
 movl $20, %eax
 call syscall_sysenter
 popl %ebp
 ret


.globl fork; .type fork, @function; .align 0; fork:
 pushl %ebp
 movl %esp, %ebp
 movl $2, %eax
 call syscall_sysenter
 test %eax, %eax
 js nok
 popl %ebp
 ret


.globl exit; .type exit, @function; .align 0; exit:
 pushl %ebp
 movl %esp, %ebp
 movl $1, %eax
 call syscall_sysenter
 popl %ebp
 ret


.globl yield; .type yield, @function; .align 0; yield:
 pushl %ebp
 movl %esp, %ebp
 movl $13, %eax
 call syscall_sysenter
 popl %ebp
 ret


.globl get_stats; .type get_stats, @function; .align 0; get_stats:
 pushl %ebp
 movl %esp, %ebp
 pushl %ebx;
 movl $35, %eax
 movl 0x8(%ebp), %ebx;
 movl 0xC(%ebp), %ecx;
 call syscall_sysenter
 popl %ebx
 test %eax, %eax
 js nok
 popl %ebp
 ret

.globl ThreadCreate; .type ThreadCreate, @function; .align 0; ThreadCreate:
 pushl %ebp
 movl %esp, %ebp
 pushl %ebx
 movl $27, %eax
 movl 0x8(%ebp), %ebx
 movl 0xC(%ebp), %ecx
 movl $thread_wrapper, %edx
 call syscall_sysenter
 popl %ebx
 test %eax, %eax
 js nok
 popl %ebp
 ret

 .globl ThreadExit; .type ThreadExit, @function; .align 0; ThreadExit:
  pushl %ebp
  movl %esp, %ebp
  movl $28, %eax
  call syscall_sysenter
  test %eax, %eax
  js nok
  popl %ebp
  ret

.globl KeyboardEvent; .type KeyboardEvent, @function; .align 0; KeyboardEvent:
 pushl %ebp
 movl %esp, %ebp
 pushl %ebx;
 movl $29, %eax
 movl 0x8(%ebp), %ebx;
 movl $keyboard_wrapper, %ecx
 call syscall_sysenter
 popl %ebx
 test %eax, %eax
 js nok
 popl %ebp
 ret
# 152 "user-utils.S"
.globl keyboard_wrapper; .type keyboard_wrapper, @function; .align 0; keyboard_wrapper:

    pushl %ebp
    movl %esp, %ebp






    pushl 0x10(%ebp)


    pushl 0xC(%ebp)


    movl 0x8(%ebp), %eax
    call *%eax



    addl $8, %esp



    int $0x2b


    popl %ebp
    ret
# 190 "user-utils.S"
.globl thread_wrapper; .type thread_wrapper, @function; .align 0; thread_wrapper:

    pushl %ebp
    movl %esp, %ebp



    pushl 0xC(%ebp)



    movl 0x8(%ebp), %eax
    call *%eax


    addl $4, %esp



    call ThreadExit


    popl %ebp
    ret



.globl get_errno; .type get_errno, @function; .align 0; get_errno:
    pushl %ebp
    movl %esp, %ebp
    movl $30, %eax
    call syscall_sysenter
    popl %ebp
    ret
