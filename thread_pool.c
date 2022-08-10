/* a job queue for threads */

/*
  I'm planning to just use these for async/await style threads.
  
  Should work fine.
*/

local s32          global_thread_count                    = 0;
local SDL_Thread** global_thread_pool                     = NULL;
local struct memory_arena  global_thread_pool_arenas[128] = {};

typedef s32 (*job_queue_function)(void*);
enum {
    THREAD_JOB_STATUS_READY,
    THREAD_JOB_STATUS_WORKING,
    THREAD_JOB_STATUS_FINISHED,
};
struct thread_job_queue {
    u8                 status;
    void*              data;
    job_queue_function job;
};

static int _thread_job_executor(void* context) {
    struct memory_arena* arena = (struct memory_arena*)context;
    _debugprintf("greetings \"%s\" thread", SDL_GetThreadName(0));

    return 0;
}

void initialize_thread_pool(void) {
    s32 cpu_count = SDL_GetCPUCount();
    _debugprintf("%d cpus reported.", cpu_count);


    for (s32 index = 0; index < cpu_count; ++index) {
        global_thread_pool_arenas[index] = memory_arena_create_from_heap("threadarena", Kilobyte(128));
        global_thread_pool[index] = SDL_CreateThread(_thread_job_executor, format_temp("slave%d", index), &global_thread_pool_arenas[index]);
    }
}

void synchronize_and_finish_thread_pool(void) {
    for (s32 thread_index = 0; thread_index < global_thread_count; ++thread_index) {
        SDL_WaitThread(global_thread_pool[thread_index], NULL);
        memory_arena_finish(&global_thread_pool_arenas[thread_index]);
    }
}
