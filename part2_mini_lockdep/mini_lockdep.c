#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");


#define MAX_LOCKS       8
#define MAX_THREADS     8
#define MAX_EDGES       50

typedef struct {
    int lock_id;
    char lock_name[32];
    int owner_thread_id;
} Lock;

typedef struct {
    int thread_id;
    char thread_name[32];
    int held_locks[MAX_LOCKS];
    int lock_count;
} Thread;

typedef struct {
    int from_lock_id;
    int to_lock_id;
    int thread_id;
} Edge;

static Lock locks[MAX_LOCKS];
static Thread threads[MAX_THREADS];
static Edge edges[MAX_EDGES];

static int lock_count = 0;
static int thread_count = 0;
static int edge_count = 0;

/* PART 2a: HELPER FUNCTIONS */

static int get_or_create_lock(int lock_id, const char *lock_name)
{
    int i;
    for (i = 0; i < lock_count; i++) {
        if (locks[i].lock_id == lock_id)
            return i;
    }

    if (lock_count >= MAX_LOCKS)
        return -1;

    locks[lock_count].lock_id = lock_id;
    strncpy(locks[lock_count].lock_name, lock_name, sizeof(locks[lock_count].lock_name) - 1);
    locks[lock_count].owner_thread_id = -1;

    return lock_count++;
}

static int get_or_create_thread(int thread_id, const char *thread_name)
{
    int i;
    for (i = 0; i < thread_count; i++) {
        if (threads[i].thread_id == thread_id)
            return i;
    }

    if (thread_count >= MAX_THREADS)
        return -1;

    threads[thread_count].thread_id = thread_id;
    strncpy(threads[thread_count].thread_name, thread_name, sizeof(threads[thread_count].thread_name) - 1);
    threads[thread_count].lock_count = 0;

    return thread_count++;
}

static int add_edge(int from_lock_id, int to_lock_id, int thread_id)
{
    int i;

    for (i = 0; i < edge_count; i++) {
        if (edges[i].from_lock_id == from_lock_id && edges[i].to_lock_id == to_lock_id)
            return 0;
    }

    if (edge_count >= MAX_EDGES)
        return 0;

    edges[edge_count].from_lock_id = from_lock_id;
    edges[edge_count].to_lock_id = to_lock_id;
    edges[edge_count].thread_id = thread_id;

    edge_count++;
    return 1;
}

/* PART 2a: LOCK ACQUISITION AND RELEASE */

void mini_lockdep_acquire(int thread_id, int lock_id, const char *lock_name)
{
    int lock_idx, thread_idx, i;

    lock_idx = get_or_create_lock(lock_id, lock_name);
    if (lock_idx < 0) return;

    thread_idx = get_or_create_thread(thread_id, "thread");
    if (thread_idx < 0) return;

    locks[lock_idx].owner_thread_id = thread_id;

    /* PART 2b: CREATE EDGES */
    for (i = 0; i < threads[thread_idx].lock_count; i++) {
        int from_lock = threads[thread_idx].held_locks[i];
        add_edge(from_lock, lock_id, thread_id);
    }

    if (threads[thread_idx].lock_count >= MAX_LOCKS)
        return;

    threads[thread_idx].held_locks[threads[thread_idx].lock_count] = lock_id;
    threads[thread_idx].lock_count++;
}

void mini_lockdep_release(int thread_id, int lock_id)
{
    int thread_idx, i, j, found = 0;

    for (thread_idx = 0; thread_idx < thread_count; thread_idx++) {
        if (threads[thread_idx].thread_id == thread_id)
            break;
    }

    if (thread_idx >= thread_count)
        return;

    for (i = 0; i < threads[thread_idx].lock_count; i++) {
        if (threads[thread_idx].held_locks[i] == lock_id) {
            for (j = i; j < threads[thread_idx].lock_count - 1; j++)
                threads[thread_idx].held_locks[j] = threads[thread_idx].held_locks[j + 1];

            threads[thread_idx].lock_count--;
            found = 1;
            break;
        }
    }
}

/* PART 2b: PRINT DEPENDENCY GRAPH */

void mini_lockdep_print_graph(void)
{
    int i;

    printk(KERN_INFO "\nGraph Edges:\n");
    for (i = 0; i < edge_count; i++) {
        printk(KERN_INFO "  Lock_%d -> Lock_%d\n",
               edges[i].from_lock_id, edges[i].to_lock_id);
    }
}

/* PART 2c: CYCLE DETECTION (DFS) */

static int dfs(int lock_id, int visited[], int in_stack[])
{
    int i;

    if (in_stack[lock_id])
        return 1;

    if (visited[lock_id])
        return 0;

    visited[lock_id] = 1;
    in_stack[lock_id] = 1;

    for (i = 0; i < edge_count; i++) {
        if (edges[i].from_lock_id == lock_id) {
            if (dfs(edges[i].to_lock_id, visited, in_stack))
                return 1;
        }
    }

    in_stack[lock_id] = 0;
    return 0;
}

int mini_lockdep_detect_cycles(void)
{
    int visited[256] = {0};
    int in_stack[256] = {0};
    int i;

    for (i = 0; i < lock_count; i++) {
        if (!visited[locks[i].lock_id]) {
            if (dfs(locks[i].lock_id, visited, in_stack))
                return 1;
        }
    }
    return 0;
}

/* CLEANUP */

void mini_lockdep_cleanup(void)
{
    lock_count = 0;
    thread_count = 0;
    edge_count = 0;
}

/* TEST SCENARIOS */

void scenario_no_deadlock(void)
{
    printk(KERN_INFO "\n--- Scenario 1: No Deadlock ---\n");
    mini_lockdep_cleanup();

    mini_lockdep_acquire(1, 1, "LockA");
    mini_lockdep_acquire(1, 2, "LockB");
    mini_lockdep_release(1, 2);
    mini_lockdep_release(1, 1);

    mini_lockdep_acquire(2, 1, "LockA");
    mini_lockdep_acquire(2, 2, "LockB");
    mini_lockdep_release(2, 2);
    mini_lockdep_release(2, 1);

    mini_lockdep_print_graph();
    printk(KERN_INFO "Result: %s\n", mini_lockdep_detect_cycles() ? "DEADLOCK" : "OK");
}

void scenario_two_lock_deadlock(void)
{
    printk(KERN_INFO "\n--- Scenario 2: AB/BA Deadlock ---\n");
    mini_lockdep_cleanup();

    mini_lockdep_acquire(1, 1, "LockA");
    mini_lockdep_acquire(1, 2, "LockB");
    mini_lockdep_release(1, 2);
    mini_lockdep_release(1, 1);

    mini_lockdep_acquire(2, 2, "LockB");
    mini_lockdep_acquire(2, 1, "LockA");
    mini_lockdep_release(2, 1);
    mini_lockdep_release(2, 2);

    mini_lockdep_print_graph();
    printk(KERN_INFO "Result: %s\n", mini_lockdep_detect_cycles() ? "DEADLOCK" : "OK");
}

void scenario_three_lock_cycle(void)
{
    printk(KERN_INFO "\n--- Scenario 3: Three-Lock Cycle ---\n");
    mini_lockdep_cleanup();

    mini_lockdep_acquire(1, 1, "LockA");
    mini_lockdep_acquire(1, 2, "LockB");
    mini_lockdep_release(1, 2);
    mini_lockdep_release(1, 1);

    mini_lockdep_acquire(2, 2, "LockB");
    mini_lockdep_acquire(2, 3, "LockC");
    mini_lockdep_release(2, 3);
    mini_lockdep_release(2, 2);

    mini_lockdep_acquire(3, 3, "LockC");
    mini_lockdep_acquire(3, 1, "LockA");
    mini_lockdep_release(3, 1);
    mini_lockdep_release(3, 3);

    mini_lockdep_print_graph();
    printk(KERN_INFO "Result: %s\n", mini_lockdep_detect_cycles() ? "DEADLOCK" : "OK");
}

/* MODULE INIT/EXIT */

static int __init mini_lockdep_init(void)
{
    printk(KERN_INFO "\n=== MINI-LOCKDEP (Part 2) ===\n");

    scenario_no_deadlock();
    scenario_two_lock_deadlock();
    scenario_three_lock_cycle();

    printk(KERN_INFO "\n=== Tests Complete ===\n\n");
    return 0;
}

static void __exit mini_lockdep_exit(void)
{
    printk(KERN_INFO "Mini-Lockdep unloading\n");
}

module_init(mini_lockdep_init);
module_exit(mini_lockdep_exit);
