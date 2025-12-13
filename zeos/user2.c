#include <libc.h>

char buff[24];
char buff2[24];

int pid;

static volatile int syscalls_bloqueadas = 0;
static volatile int ultima_tecla = 0;
static volatile int teclas_pulsadas = 0;



// Comptadors globals per testing
int thread1_counter = 0;
int thread2_counter = 0;
int thread3_counter = 0;
int recursion_counter = 0;
int dynamic_counter_1 = 0;
int dynamic_counter_2 = 0;
int dynamic_counter_3 = 0;
int exit_counter = 0;

#define EINPROGRESS 115

/* Función para probar la pila profunda */
/* En user.c */
/* ======================================================== */
/* TEST INTEGRAL 1: CONCURRENCIA CON STACK GROWTH           */
/* ======================================================== */

volatile int race_finished = 0;

void recursive_racer(int depth, int id) {
    char buffer[400]; // Variable grande para forzar uso de pila
    int i;
    
    // 1. "Ensuciar" la pila con nuestro ID
    for(i=0; i<400; i++) buffer[i] = (char)id;
    
    // 2. Ceder CPU (Yield) en mitad de la recursión para estresar al scheduler
    if (depth % 10 == 0) {
        write(1, "  [Racer] Yielding...\n", 22);
        yield();
    }
    
    // 3. Profundizar (Si depth > 0)
    if (depth > 0) {
        recursive_racer(depth - 1, id);
    }
    
    // 4. VERIFICACIÓN: Al volver, ¿sigue mi pila intacta?
    for(i=0; i<400; i++) {
        if (buffer[i] != (char)id) {
            write(1, "ERROR CRITICO: Pila corrupta por otro thread!\n", 46);
            while(1); // Bloquear para notar el error
        }
    }
    
    if (depth == 0) write(1, "  [Racer] Fondo alcanzado OK\n", 29);
}

void thread_racer_wrapper(void *arg) {
    int id = *(int*)arg;
    
    // Profundidad 20 * 400 bytes = ~8KB (fuerza 2 o 3 page faults dinámicos)
    recursive_racer(20, id);
    
    race_finished++;
    ThreadExit();
}

void test_comprehensive_1_stack_race(void) {
    int id1=1, id2=2;
    int tid1, tid2;
    int i;

    write(1, "\n=== TEST FINAL 1: Carrera de Pilas (Stress) ===\n", 49);
    
    race_finished = 0;
    
    // Creamos dos hilos que competirán por crecer su pila
    tid1 = ThreadCreate(thread_racer_wrapper, &id1);
    tid2 = ThreadCreate(thread_racer_wrapper, &id2);

    if (tid1 < 0 || tid2 < 0) {
        write(1, "Error creando threads\n", 22);
        return;
    }

    // El main espera (polling simple)
    while(race_finished < 2) {
        yield();
    }

    write(1, "[TEST 1] PASSAT: Pilas crecieron y no se corrompieron.\n", 55);
}

/* ======================================================== */
/* TEST INTEGRAL 2: GESTIÓN DE MEMORIA (CREATE / EXIT)      */
/* ======================================================== */

volatile int worker_done = 0;

void quick_worker(void *arg) {
    int id = *(int*)arg;
    char b[20];
    
    // Usamos algo de memoria
    write(1, "  [Worker] Hola soy el thread ", 30);
    itoa(id, b);
    write(1, b, strlen(b));
    write(1, "\n", 1);
    
    worker_done = 1;
    ThreadExit();
}

void test_comprehensive_2_memory(void) {
    int i, tid;
    
    write(1, "\n=== TEST FINAL 2: Reciclaje de Threads ===\n", 44);
    write(1, "Creando y destruyendo threads secuencialmente...\n", 49);

    // Intentamos crear 20 threads (Si no liberas memoria, fallará al 10º aprox)
    for (i = 0; i < 20; i++) {
        worker_done = 0;
        
        tid = ThreadCreate(quick_worker, &i);
        
        if (tid < 0) {
            write(1, "ERROR: Fallo al crear thread (Leak de memoria?)\n", 48);
            return;
        }
        
        // Esperamos a que termine antes de lanzar el siguiente
        while(worker_done == 0) yield();
    }

    write(1, "[TEST 2] PASSAT: Memoria y TIDs liberados correctamente.\n", 57);
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
            int offset = (i * 80 + j) * 2;
            int es_parell = (i + j) % 2; 
            if (es_parell) {
                buffer[offset] = 'X';      
                buffer[offset + 1] = 0x2F; // Fons Verd (2), Text Blanc (F)
            } else {
                buffer[offset] = 'Y';
                buffer[offset + 1] = 0x4F; // Fons Vermell (4), Text Blanc (F)
	        }
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
    


      write(1, "\n=== INICIANDO TESTS GLOBALES MILESTONE 1 ===\n", 46);

      // Ejecuta estos dos para validar todo el Milestone 1
      test_comprehensive_1_stack_race();
      test_comprehensive_2_memory();

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
 








