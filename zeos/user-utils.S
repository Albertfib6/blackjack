#include <asm.h>

ENTRY(syscall_sysenter)
	push %ecx
	push %edx
	push $SYSENTER_RETURN
	push %ebp
	mov %esp, %ebp
	sysenter
ENTRY(SYSENTER_RETURN)
	pop %ebp
	pop %edx
	pop %edx
	pop %ecx
	ret

/* int write(int fd, char *buffer, int size) */
ENTRY(write)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx;  // Save EBX, ESI and EDI if modified
	movl $4, %eax
	movl 0x8(%ebp), %ebx;	//fd
	movl 0xC(%ebp), %ecx;	//buffer
	movl 0x10(%ebp), %edx;	//size
	call syscall_sysenter
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) -> 
	popl %ebp
	ret

/* Common code for negative return */
nok:
	neg %eax
	mov %eax, errno
	mov $-1, %eax
	popl %ebp
	ret

/* int gettime() */
ENTRY(gettime)
	pushl %ebp
	movl %esp, %ebp
	movl $10, %eax
	call syscall_sysenter
	popl %ebp
	ret

/* int getpid() */
ENTRY(getpid)
	pushl %ebp
	movl %esp, %ebp
	movl $20, %eax
	call syscall_sysenter
	popl %ebp
	ret

/* int fork() */
ENTRY(fork)
	pushl %ebp
	movl %esp, %ebp
	movl $2, %eax
	call syscall_sysenter
	test %eax, %eax
	js nok	// if (eax < 0) -->
	popl %ebp
	ret

/* void exit() */
ENTRY(exit)
	pushl %ebp
	movl %esp, %ebp
	movl $1, %eax
	call syscall_sysenter
	popl %ebp
	ret

/* int yield() */
ENTRY(yield)
	pushl %ebp
	movl %esp, %ebp
	movl $13, %eax
	call syscall_sysenter
	popl %ebp
	ret

/* int get_stats(int pid, struct stats *st) */
ENTRY(get_stats)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx;  // Save EBX, ESI and EDI if modified
	movl $35, %eax
	movl 0x8(%ebp), %ebx;	//pid
	movl 0xC(%ebp), %ecx;	//st
	call syscall_sysenter
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) -->
	popl %ebp
	ret

ENTRY(ThreadCreate)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx;  // Save EBX, ESI and EDI if modified
	movl $27, %eax
	movl 0x8(%ebp), %ebx;	//function
	movl 0xC(%ebp), %ecx;	//parameter
	call syscall_sysenter
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) -->
	popl %ebp
	ret

 ENTRY(ThreadExit)
 	pushl %ebp
 	movl %esp, %ebp
 	movl $28, %eax
 	call syscall_sysenter
 	test %eax, %eax
 	js nok	// if (eax < 0) -->
 	popl %ebp
 	ret

ENTRY(KeyboardEvent)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx;  // Save EBX, ESI and EDI if modified
	movl $29, %eax
	movl 0x8(%ebp), %ebx;	//func
	call syscall_sysenter
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) -> 
	popl %ebp
	ret



/* * void keyboard_wrapper(void (*func)(char, int), char key, int pressed);
 *
 * Stack Layout al entrar (construido por el Kernel):
 * [EBP]      = EBP Guardado
 * [EBP + 4]  = Retorno (Dummy 0)
 * [EBP + 8]  = func (puntero a función)
 * [EBP + 12] = key (char/int)
 * [EBP + 16] = pressed (int)
 */

ENTRY(keyboard_wrapper)
    /* 1. Prólogo estándar */
    pushl %ebp
    movl %esp, %ebp

    /* 2. Preparar los argumentos para llamar a la función del usuario 'func' */
    /* Prototipo: func(char key, int pressed) */
    /* Convención C (cdecl): Empujar argumentos de derecha a izquierda */

    /* Push 'pressed' (2º argumento) */
    pushl 0x10(%ebp)

    /* Push 'key' (1º argumento) */
    pushl 0xC(%ebp)

    /* 3. Obtener el puntero a la función y llamar */
    movl 0x8(%ebp), %eax   // Cargar la dirección de 'func' en EAX
    call *%eax             // Llamada indirecta a la función

    /* 4. Limpiar la pila después de la llamada */
    /* Hemos empujado 2 argumentos de 4 bytes = 8 bytes */
    addl $8, %esp

    /* 5. Restaurar la ejecución del hilo original */
    /* Esta interrupción hace que el Kernel restaure el EIP/ESP guardados */
    int $0x2b

    /* 6. Epílogo (Código inalcanzable, pero por corrección) */
    popl %ebp
    ret


