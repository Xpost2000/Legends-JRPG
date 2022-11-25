/* a job queue for threads */

/*
  I'm planning to just use these for async/await style threads.
  
  Should work fine.
*/

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

/* this might be not a great implementation */
struct thread_job_queue {
    struct thread_job jobs[MAX_JOBS];
    SDL_sem*          notification;
    SDL_mutex*        mutex;
};

struct thread_job_queue global_job_queue = {};
/* local bool global_thread_pool_use_threads = true; */

void thread_pool_add_job(job_queue_function job, void* data) {
    for (s32 index = 0; index < MAX_JOBS; ++index) {
        struct thread_job* current_job = &global_job_queue.jobs[index];
        
        if (current_job->status == THREAD_JOB_STATUS_FINISHED) {
            current_job->status = THREAD_JOB_STATUS_READY;
            current_job->data   = data;
            current_job->job    = job;
#if 0
            _debugprintf("posted new job (%p dataptr)", current_job->data);
#endif

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
#if 1
    _debugprintf("thread start : \"%d\"", (s32)SDL_ThreadID());
#endif

    struct thread_job* working_job = 0;

    for (;;) {
        if (!SDL_SemWait(global_job_queue.notification)) {
            if (!global_game_running) {
                break;
            }

            /* job hunting! */
            /* let's hunt */
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

void thread_pool_simulate_single_threaded(bool v) {
    /* TODO */
}

void synchronize_and_finish_thread_pool(void) {
    _debugprintf("Trying to synchronize and finish the threads... Please");
    /* signal to quit the threads since we do a blocking wait */

    _debugprintf("semaphore value: %d (have to kill %d threads)", SDL_SemValue(global_job_queue.notification), global_thread_count);
    /* 
       notify the semaphore up to the amount of threads,
       
       semaphore is accessed, and should atomically decrement and the thread should quit.

       I have no idea why I have to separate the waiting honestly. Seems to deadlock in that
       case.
       
       Considering I only have one mutex and one semaphore I'm surprised I can cause a deadlock with
       a situation that simple...
    */
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
