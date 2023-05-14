# Legends RPG: Job Queue System

Multithreading is a difficult problem however since Legends is a pretty constrained case where 
only the graphics engine requires the performance of multithreading.

This allows me to write an extremely simple and clean job system. It has a very simple API as a result
and made parallelizing the graphics engine very painless.

## Main Interface

The interface exposes a few functions and notably a simple function pointer. The API is extremely
easy to use and kind of hard to mess up using.

**NOTE: Because I use a unity build in my engine, this is actually just one file since the entire file is quite small.**
**NOTE: This is not exactly designed for asynchronous calculations that you want results of (but you can just add a output pointer to your jobs.)**

```c
#define MAX_POSSIBLE_THREADS (16)
local volatile s32 global_thread_count                                     = 0;
local SDL_Thread* global_thread_pool[MAX_POSSIBLE_THREADS]                 = {};
local struct memory_arena  global_thread_pool_arenas[MAX_POSSIBLE_THREADS] = {};

typedef s32 (*job_queue_function)(void*);
#define MAX_JOBS (512)

enum {
    THREAD_JOB_STATUS_FINISHED,
    THREAD_JOB_STATUS_READY,
    THREAD_JOB_STATUS_WORKING,
};
struct thread_job {
    u8                 status;
    void*              data;
    job_queue_function job;
};

struct thread_job_queue {
    struct thread_job jobs[MAX_JOBS];
    SDL_sem*          notification;
    SDL_mutex*        mutex;
};

void thread_pool_add_job(job_queue_function job, void* data);
void thread_pool_synchronize_tasks();
void initialize_thread_pool(void); 
void synchronize_and_finish_thread_pool(void);
```

## Implementation

Here is the full implementation of the job system in legends. It's a very simple but effective job system.

```
#define MAX_POSSIBLE_THREADS (16)
local volatile s32 global_thread_count                                     = 0;
local SDL_Thread* global_thread_pool[MAX_POSSIBLE_THREADS]                 = {};
local struct memory_arena  global_thread_pool_arenas[MAX_POSSIBLE_THREADS] = {};

typedef s32 (*job_queue_function)(void*);
enum {
    THREAD_JOB_STATUS_FINISHED,
    THREAD_JOB_STATUS_READY,
    THREAD_JOB_STATUS_WORKING,
};
struct thread_job {
    u8                 status;
    void*              data;
    job_queue_function job;
};

#define MAX_JOBS (512)

struct thread_job_queue {
    struct thread_job jobs[MAX_JOBS];
    SDL_sem*          notification;
    SDL_mutex*        mutex;
};

struct thread_job_queue global_job_queue = {};

void thread_pool_add_job(job_queue_function job, void* data) {
    for (s32 index = 0; index < MAX_JOBS; ++index) {
        struct thread_job* current_job = &global_job_queue.jobs[index];
        
        if (current_job->status == THREAD_JOB_STATUS_FINISHED) {
            current_job->status = THREAD_JOB_STATUS_READY;
            current_job->data   = data;
            current_job->job    = job;
            _debugprintf("posted new job (%p dataptr)", current_job->data);

            SDL_SemPost(global_job_queue.notification);
            return;
        }
    }
}

void thread_pool_synchronize_tasks() {
    bool done = false;
    while (!done && global_game_running) {
        done = true;

        for (s32 index = 0; index < MAX_JOBS; ++index) {
            struct thread_job* current_job = global_job_queue.jobs + index;
            if (current_job->status != THREAD_JOB_STATUS_FINISHED) {
                done = false;
            }
        }
    }
}

static int _thread_job_executor(void* context) {
    struct memory_arena* arena   = (struct memory_arena*)context;
    struct memory_arena  scratch = memory_arena_push_sub_arena(arena, Kilobyte(64));
    _debugprintf("thread start : \"%d\"", (s32)SDL_ThreadID());

    struct thread_job* working_job = 0;

    for (;;) {
        if (!SDL_SemWait(global_job_queue.notification)) {
            if (!global_game_running) {
                break;
            }

            SDL_LockMutex(global_job_queue.mutex);
            {
                while (!working_job && global_game_running) {
                    for (s32 index = 0; index < MAX_JOBS && global_game_running; ++index) {
                        struct thread_job* current_job = &global_job_queue.jobs[index];

                        if (current_job->status == THREAD_JOB_STATUS_READY) {
                            current_job->status = THREAD_JOB_STATUS_WORKING;
                            working_job = current_job;
                            break;
                        }
                    }
                }
            }
            SDL_UnlockMutex(global_job_queue.mutex);
        }

        if (working_job && working_job->job) {
            working_job->job(working_job->data);
            working_job->status = THREAD_JOB_STATUS_FINISHED;
            working_job         = NULL;
        }
    }

    _debugprintf("Thread (%d) quits", (s32)SDL_ThreadID());
    return 0;
}

void initialize_thread_pool(void) {
    s32 cpu_count = SDL_GetCPUCount();
    global_thread_count = cpu_count * 2;
    _debugprintf("%d cpus reported.", cpu_count);

    global_job_queue.notification = SDL_CreateSemaphore(0);
    global_job_queue.mutex        = SDL_CreateMutex();

    if (global_thread_count > MAX_POSSIBLE_THREADS) global_thread_count = MAX_POSSIBLE_THREADS;
    for (s32 index = 0; index < global_thread_count; ++index) {
        _debugprintf("Trying to make thread %d", index);
        global_thread_pool_arenas[index] = memory_arena_create_from_heap("thread pool", Kilobyte(256));
        global_thread_pool[index]        = SDL_CreateThread(_thread_job_executor, format_temp("slave%d", index), &global_thread_pool_arenas[index]);
    }
}

void synchronize_and_finish_thread_pool(void) {
    _debugprintf("Trying to synchronize and finish the threads... Please");
    /* signal to quit the threads since we do a blocking wait */

    _debugprintf("semaphore value: %d (have to kill %d threads)", SDL_SemValue(global_job_queue.notification), global_thread_count);
    thread_pool_synchronize_tasks();
    for (s32 thread_index = 0; thread_index < global_thread_count; ++thread_index) {
        _debugprintf("posting semaphore, and hoping a thread dies");
        SDL_SemPost(global_job_queue.notification);
    }

    for (s32 thread_index = 0; thread_index < global_thread_count; ++thread_index) {
        _debugprintf("waiting on thread");
        SDL_WaitThread(global_thread_pool[thread_index], NULL);
    }

    for (s32 thread_index = 0; thread_index < global_thread_count; ++thread_index) {
        memory_arena_finish(&global_thread_pool_arenas[thread_index]);
    }

    SDL_DestroySemaphore(global_job_queue.notification);
    SDL_DestroyMutex(global_job_queue.mutex);
}
```
