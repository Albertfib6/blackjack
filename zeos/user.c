#include <libc.h>

char buff[24];
char buff2[24];

int pid;

static volatile int syscalls_bloqueadas = 0;
static volatile int ultima_tecla = 0;
static volatile int teclas_pulsadas = 0;

#define EINPROGRESS 115

/* Función para probar la pila profunda */
void funcion_thread_pesado(void *parametro) {
    int id = (int)parametro;
    
    itoa(id, buff2);
    write(1, "Valor de ID: ", 13);
    write(1, buff2, strlen(buff2));
    write(1, "\n", 1);
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
}

void handler_teclado(char key, int pressed) {
    
    /* Solo nos interesa cuando se pulsa (pressed=1), no cuando se suelta */
    if (pressed) {
        
        /* 1. PRUEBA DE BLOQUEO DE SYSCALLS 
           Intentamos escribir por pantalla. ESTO DEBE FALLAR. */
        int ret = write(1, "X", 1);
        
        /* Si falla con el error correcto, sumamos al contador */
        /* Nota: write devuelve -1 en error, y el error real está en errno (o eax negativo si es raw) */
        /* Asumimos wrapper estándar que pone errno. Si es raw syscall, ret será -115 */
        
        if (ret < 0 && get_errno() == EINPROGRESS) {
            syscalls_bloqueadas++;
        }
        
        /* 2. REGISTRO DEL EVENTO */
        ultima_tecla = (int)key;
        teclas_pulsadas++;
    }
}

void generar_pantalla(char *buffer) {
    for (int i = 0; i < 25; i++) {
        for (int j = 0; j < 80; j++) {
            int pos = (i * 80 + j) * 2;
            int par = pos%2;
            buffer[pos] = par ? 'X' : 'Y';      
            buffer[pos + 1] = par ? 0x2F : 0x4F; 
        }
    }
}

void test_screen() {
    char screen_buffer[80*25*2];
    generar_pantalla(screen_buffer);
    write(10, screen_buffer, sizeof(screen_buffer));
}


int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

	/* Test 1: Creación Básica */
    /*write(1, "Iniciando Test Milestone 1...\n", 30);
    
    int pid_hilo = ThreadCreate(funcion_thread_pesado, (void*)1);

    if (pid_hilo < 0) {
    	char buff[32];
    	write(1, "ERROR CODIGO: ", 14);
    	itoa(get_errno(), buff);
    	write(1, buff, strlen(buff));
    	write(1, "\n", 1);
        write(1, "ERROR: Fallo al crear hilo\n", 27);
    } 

    else {
        write(1, "Hilo creado correctamente. Esperando...\n", 40);
    }


   /* ESPERA SEGURA:
       Usamos un bucle largo en lugar de pocos yields para dar tiempo de sobra
       al hilo (creación + page fault + impresión) antes de seguir. */
    /*int i;
    for (i = 0; i < 20000000; i++); 

    write(1, "Test finalizado.\n", 17);*/

    write(1, "Test 2: Activando Teclado. PULSA TECLAS.\n", 41);

    /* Registramos el handler */
    if (KeyboardEvent(handler_teclado) < 0) {
        write(1, "Error syscall KeyboardEvent\n", 28);
        while(1);
    }

    int local_teclas = 0;
 
    while(1) { 
        test_screen();
        /* Detectamos si el handler ha modificado las variables */
        if (local_teclas != teclas_pulsadas) {
            
            /* Actualizamos estado local */
            local_teclas = teclas_pulsadas;
            
            /* Preparamos el mensaje */
            write(1, "Evento detectado! -> ", 21);
            
            /* Imprimimos tecla */
            char k = (char)ultima_tecla;
            if (k >= ' ' && k <= '~') { // Si es imprimible
                write(1, "Tecla: '", 8);
                write(1, &k, 1);
                write(1, "' ", 2);
            } else {
                write(1, "Tecla: [Special] ", 17);
            }

            /* Imprimimos estado del bloqueo */
            write(1, "| Bloqueos exitosos: ", 21);
            itoa(syscalls_bloqueadas, buff);
            write(1, buff, strlen(buff));
            
            write(1, "\n", 1);
        }

        /* Hacemos un poco de espera para no saturar la CPU imprimiendo (opcional) */
        // for(int i=0; i<100000; i++); 
    }
}
 







