/* a job queue for threads */

/*
  I'm planning to just use these for async/await style threads.
  
  Should work fine.
*/

#define MAX_POSSIBLE_THREADS (16)
local s32 global_thread_count                                              = 0;
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
    while (!done) {
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
#if 0
    _debugprintf("thread start : \"%d\"", SDL_ThreadID());
#endif

    struct thread_job* working_job = 0;

    while (global_game_running) {
#if 0
        _debugprintf("thread(%d) waits for a job", SDL_ThreadID());
#endif
        if (!SDL_SemWait(global_job_queue.notification)) {
            /* job hunting! */
            /* let's hunt */
            (SDL_LockMutex(global_job_queue.mutex)); {
#if 0
                _debugprintf("thread(%d) begins locking", SDL_ThreadID());
#endif
                while (!working_job && global_game_running) {
                    for (s32 index = 0; index < MAX_JOBS; ++index) {
                        struct thread_job* current_job = &global_job_queue.jobs[index];

                        if (current_job->status == THREAD_JOB_STATUS_READY) {
#if 0
                            _debugprintf("job #%d should be taken", index);
#endif
                            current_job->status = THREAD_JOB_STATUS_WORKING;
                            working_job = current_job;
                            break;
                        }
                    }
                }
#if 0
                _debugprintf("thread (%d) found a job!", SDL_ThreadID());
                _debugprintf("thread(%d) stops locking", SDL_ThreadID());
#endif
            } 
            SDL_UnlockMutex(global_job_queue.mutex);

            if (working_job && working_job->job) {
#if 0
                _debugprintf("thread (%d)(%p) doing job", SDL_ThreadID(), working_job->data);
#endif
                working_job->job(working_job->data);
                working_job->status = THREAD_JOB_STATUS_FINISHED;
            }

            working_job         = NULL;
        }
    }

    if (!SDL_TryLockMutex(global_job_queue.mutex)) {
        SDL_UnlockMutex(global_job_queue.mutex);
    }

    return 0;
}

void initialize_thread_pool(void) {
    s32 cpu_count = SDL_GetCPUCount();
    _debugprintf("%d cpus reported.", cpu_count);

    global_job_queue.notification = SDL_CreateSemaphore(0);
    global_job_queue.mutex        = SDL_CreateMutex();

    if (cpu_count > MAX_POSSIBLE_THREADS) cpu_count = MAX_POSSIBLE_THREADS;
    for (s32 index = 0; index < cpu_count; ++index) {
        global_thread_pool_arenas[index] = memory_arena_create_from_heap("thread pool", Kilobyte(256));
        global_thread_pool[index] = SDL_CreateThread(_thread_job_executor, format_temp("slave%d", index), &global_thread_pool_arenas[index]);
    }
}

void thread_pool_simulate_single_threaded(bool v) {
    /* TODO */
}

void synchronize_and_finish_thread_pool(void) {
    /* signal to quit the threads since we do a blocking wait */
    SDL_SemPost(global_job_queue.notification);
    thread_pool_synchronize_tasks();

    for (s32 thread_index = 0; thread_index < global_thread_count; ++thread_index) {
        SDL_WaitThread(global_thread_pool[thread_index], NULL);
    }

    for (s32 thread_index = 0; thread_index < global_thread_count; ++thread_index) {
        memory_arena_finish(&global_thread_pool_arenas[thread_index]);
    }

    SDL_DestroySemaphore(global_job_queue.notification);
    SDL_DestroyMutex(global_job_queue.mutex);
}
