#include <libc.h>
#include <errno.h>

char buff[24];

int pid;

// Comptadors globals per testing
int thread1_counter = 0;
int thread2_counter = 0;
int thread3_counter = 0;
int recursion_counter = 0;
int dynamic_counter_1 = 0;
int dynamic_counter_2 = 0;
int dynamic_counter_3 = 0;
int exit_counter = 0;

// === Test 1: Simple Math Thread ===
void math_worker(void *arg)
{
  int *val = (int *)arg;
  int i;
  int acc = 0;
  
  write(1, "  [Math] Starting calculations...\n", 34);
  
  for (i = 0; i < 5; i++) {
    acc += (*val) + i;
    yield();
  }
  
  *val = acc;
  write(1, "  [Math] Finished.\n", 19);
  ThreadExit();
}

// === Test 2: Visual Concurrency ===
void print_x(void *arg)
{
  int i;
  for (i = 0; i < 10; i++) {
    write(1, "X", 1);
    yield();
  }
  ThreadExit();
}

void print_o(void *arg)
{
  int i;
  for (i = 0; i < 10; i++) {
    write(1, "O", 1);
    yield();
  }
  //ThreadExit();
}

// === Test 3: Stack Integrity ===
void stack_diver(int current_depth, int *max_depth)
{
  char frame_data[80]; 
  int k;
  
  // Mark stack memory
  for(k=0; k<80; k++) frame_data[k] = (char)(current_depth + 65); // 'A', 'B'...
  
  if (current_depth > *max_depth) *max_depth = current_depth;
  
  if (current_depth > 0) {
    stack_diver(current_depth - 1, max_depth);
  }
  
  // Check memory
  if (frame_data[0] != (char)(current_depth + 65)) {
      write(1, "  [!] Stack Corruption\n", 23);
  }
  
  if (current_depth % 4 == 0) yield();
}

void stack_tester(void *arg)
{
  write(1, "  [Stack] Allocating frames...\n", 31);
  
  recursion_counter = 0;
  stack_diver(18, &recursion_counter);
  
  write(1, "  [Stack] Frames released.\n", 27);
  //ThreadExit();
}

// === Test 5: Stack Growth ===
void stack_grower(void *arg)
{
    int depth = (int)arg;
    volatile char block[1024]; // 1KB per frame, volatile to force memory access
    int i;
    char buff[64];
    
    // Print current stack address
    write(1, "  [Grower] Depth: ", 18);
    itoa(depth, buff);
    write(1, buff, strlen(buff));
    write(1, " &block: 0x", 11);
    itoa((int)block, buff); // Assuming itoa handles hex or just int
    write(1, buff, strlen(buff));
    write(1, "\n", 1);

    // Force page allocation
    for(i=0; i<1024; i++) block[i] = (char)i;
    
    if (depth > 0) {
        stack_grower((void*)(depth - 1));
        // Prevent Tail Call Optimization by doing something after return
        block[0] = 'X'; 
    } else {
        write(1, "  [Grower] Limit reached. Returning safely.\n", 44);
    }
}



void run_test_1_creation(void)
{
  int tid;
  int number = 10;
  int i;
  
  write(1, "=== Test 1: Single Thread Execution ===\n", 40);

  // Errno check
  if (write(-1, 0, 0) < 0) {
     // Just to ensure errno works
  }
  
  tid = ThreadCreate(math_worker, (void *)&number);
  
  if (tid < 0) {
    write(1, "FAIL: ThreadCreate returned error\n", 34);
    return;
  }
  
  write(1, "Thread spawned. Waiting...\n", 27);
  
  for (i = 0; i < 10; i++) yield();
  
  write(1, "Result: ", 8);
  itoa(number, buff);
  write(1, buff, strlen(buff));
  write(1, "\n", 1);
}

void run_test_2_concurrency(void)
{
  int t1, t2;
  int i;
  
  write(1, "\n=== Test 2: X/O Interleaving ===\n", 34);
  
  t1 = ThreadCreate(print_x, 0);
  t2 = ThreadCreate(print_o, 0);
  
  if (t1 < 0 || t2 < 0) {
    write(1, "FAIL: Could not create threads\n", 31);
    return;
  }
  
  for (i = 0; i < 25; i++) yield();
  
  write(1, "\n[Test 2] Done\n", 15);
}

void run_test_3_stack(void)
{
  int tid;
  int i;
  
  write(1, "\n=== Test 3: Stack Depth Check ===\n", 35);
  
  tid = ThreadCreate(stack_tester, 0);
  
  if (tid < 0) {
    write(1, "FAIL: Stack thread creation error\n", 34);
    return;
  }
  
  for (i = 0; i < 60; i++) yield();
  
  write(1, "Max Depth: ", 11);
  itoa(recursion_counter, buff);
  write(1, buff, strlen(buff));
  write(1, "\n", 1);
}

void run_test_4_fork(void)
{
  int pid;
  int dummy = 0;
  
  write(1, "\n=== Test 4: Fork & Thread Mix ===\n", 35);
  
  write(1, "Spawning background thread...\n", 30);
  ThreadCreate(math_worker, (void *)&dummy);
  yield();
  
  write(1, "Forking process...\n", 19);
  pid = fork();
  
  if (pid == 0) {
    write(1, "  [Child] Alive. Checking threads...\n", 37);
    
    write(1, "  [Child] Spawning 'O' printer...\n", 34);
    ThreadCreate(print_o, 0);
    
    for (pid = 0; pid < 20; pid++) yield();
    
    write(1, "  [Child] Exiting.\n", 19);
    exit();
  } else {
    write(1, "  [Parent] Child PID: ", 22);
    itoa(pid, buff);
    write(1, buff, strlen(buff));
    write(1, "\n", 1);
    
    for (pid = 0; pid < 50; pid++) yield();
    
    write(1, "\n[Test 4] Passed\n", 17);
  }
}

void run_test_5_growth(void)
{
  int tid;
  
  write(1, "\n=== Test 5: Dynamic Stack Growth ===\n", 38);
  write(1, "Growing stack beyond initial page...\n", 37);
  
  // Depth 10 * 1KB = 10KB. Initial stack is usually 4KB (1 page).
  // This guarantees page faults that must be recovered.
  tid = ThreadCreate(stack_grower, (void *)10);
  
  if (tid < 0) {
    write(1, "FAIL: Could not create thread\n", 30);
    return;
  }
  
  // Wait for completion
  int i;
  for(i=0; i<100; i++) yield();
  
  write(1, "\n[Test 5] Passed\n", 17);
}


int __attribute__ ((__section__(".text.main")))
  main(void)
{
  
  
  // Executem els tests en ordre
  run_test_1_creation();
  run_test_2_concurrency();
  run_test_3_stack();
  run_test_4_fork();
  run_test_5_growth();
  
  write(1, "   Tests completat\n", 22);
  
  while (1) {
    yield();
  }
}

