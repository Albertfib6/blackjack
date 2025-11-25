#include <libc.h>

char buff[24];

int pid;


/* Función para probar la pila profunda */
void funcion_thread_pesado(void *parametro) {
    int id = (int)parametro;
    
    write(1, "Thread iniciado. Intentando romper la pila...\n", 46);

    /* PRUEBA DE FUEGO: Crecimiento de Pila
       El enunciado dice que la pila inicial es 1 página (4096 bytes).
       Creamos un array de 5000 bytes. Al escribir en el final, 
       nos salimos de la página inicial.
       
       - Si tu page_fault_routine funciona: El sistema pausa, asigna memoria y sigue.
       - Si falla: Saldrá un error de sistema o el ordenador se reiniciará.
    */
    char array_gigante[6000]; 
    
    // Escribimos lejos para forzar el Page Fault
    array_gigante[0] = 'A';
    array_gigante[5000] = 'Z'; 

    if (array_gigante[0] == 'A' && array_gigante[5000] == 'Z') {
        write(1, "EXITO: La pila ha crecido dinamicamente!\n", 41);
    } else {
        write(1, "ERROR: Memoria corrupta\n", 24);
    }

    /* Probamos que ThreadExit funcione */
    ThreadExit();
}


int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

	/* Test 1: Creación Básica */
    write(1, "Iniciando Test Milestone 1...\n", 30);
    
    int pid_hilo = ThreadCreate(funcion_thread_pesado, (void*)1);

    if (pid_hilo < 0) {
    	char buff[32];
    	write(1, "ERROR CODIGO: ", 14);
    	itoa(errno, buff);
    	write(1, buff, strlen(buff));
    	write(1, "\n", 1);
        write(1, "ERROR: Fallo al crear hilo\n", 27);
    } 

    else {
        write(1, "Hilo creado correctamente. Esperando...\n", 40);
    }


    yield();
    /* Hacemos un bucle para esperar y no terminar el proceso padre inmediatamente.
       En un caso real usarías algún mecanismo de sincronización, 
       pero un while largo sirve para ver el resultado en pantalla.
    */
    long i;
    for (i = 0; i < 5; i++) yield(); 

    write(1, "Test finalizado.\n", 17);


  while(1) { }
}
