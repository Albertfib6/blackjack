#include <libc.h>

char buff[24];
char buff2[24];

int pid;

static volatile int syscalls_bloqueadas = 0;
static volatile int ultima_tecla = 0;
static volatile int teclas_pulsadas = 0;

#define EINPROGRESS 115

/* Función para probar la pila profunda */
/* En user.c */

void funcion_thread_pesado(void *parametro) {
    /* 1. Imprimimos mensaje INICIAL (Ahora sí saldrá) */
    write(1, "\n[HILO] Thread iniciado. ID: ", 29);
    char buff[10];
    itoa((int)parametro, buff);
    write(1, buff, strlen(buff));
    write(1, "\n", 1);

    write(1, "[HILO] Intentando romper la pila manualment...\n", 47);

    /* 2. TRUCO DE MAGIA: No declaramos array gigante.
       Usamos punteros para calcular una dirección "lejos" en la pila. */
       
    int variable_local;
    /* Obtenemos la dirección actual de la pila (donde está esta variable) */
    char *puntero_pila = (char*)&variable_local; 
    
    /* Nos movemos 5000 bytes hacia abajo (hacia direcciones menores)
       Esto nos saca de la página actual y nos mete en la siguiente (que no existe aún) */
    puntero_pila -= 5000; 

    /* 3. EL MOMENTO DE LA VERDAD */
    /* Al escribir aquí, saltará el Page Fault.
       - Tu 'page_fault_routine' saltará.
       - Detectará que es pila dinámica.
       - Asignará la memoria.
       - Volverá aquí y escribirá la 'Z'. */
       
    *puntero_pila = 'Z'; 

    /* 4. COMPROBACIÓN */
    if (*puntero_pila == 'Z') {
        write(1, "[HILO] EXITO: La pila ha crecido y recuperado el valor!\n", 56);
    } else {
        write(1, "[HILO] ERROR: Memoria corrupta\n", 31);
    }

    /* Terminamos el hilo */
    ThreadExit();
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

void generate_checkerboard_pattern(char *buffer) {
    for (int y = 0; y < 25; y++) {
        for (int x = 0; x < 80; x++) {
            int pos = (y * 80 + x) * 2;

            /* Row 0 is black for time/FPS display */
            if (y == 0) {
                buffer[pos] = ' ';
                buffer[pos + 1] = 0x00;
                continue;
            }

            /* Checkerboard pattern: alternate A/B both horizontally and vertically */
            int is_alternate = (x + y) % 2;
            buffer[pos] = is_alternate ? 'B' : 'A';       /* Character */
            buffer[pos + 1] = is_alternate ? 0x4F : 0x1F; /* Red on white : Blue on white */
        }
    }
}

void test_screen() {
    char screen_buffer[80*25*2];
    generate_checkerboard_pattern(screen_buffer);
    write(10, screen_buffer, sizeof(screen_buffer));
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
    int i;
    for (i = 0; i < 20000000; i++); 

    write(1, "Test finalizado.\n", 17);

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
 





