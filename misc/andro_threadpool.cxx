
#include "andro_threadpool.h"

#include <stdarg.h>
#include <unistd.h>  //usleep

#include "andro_func.h"
#include "andro_global.h"
#include "andro_logic.h"
#include "andro_macro.h"
#include "andro_memory.h"

pthread_mutex_t CThreadPool::pool_pthread_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t CThreadPool::pool_pthread_cond = PTHREAD_COND_INITIALIZER;
bool CThreadPool::shutdown = false;

CThreadPool::CThreadPool() {
    running_thread_num = 0;
    last_emg_time = 0;

    msg_queue_size = 0;
}

CThreadPool::~CThreadPool() {
    clear_msg_queue();
}

void CThreadPool::clear_msg_queue() {
    char* tmp_mem_ptr;
    CMemory* memory = CMemory::GetInstance();
    while (!msg_queue.empty()) {
        tmp_mem_ptr = msg_queue.front();
        msg_queue.pop_front();
        memory->FreeMemory(tmp_mem_ptr);
    }
}

bool CThreadPool::Create(int in_thread_num) {
    lp_thread_t thread;
    int err;

    thread_num = in_thread_num;
    for (int i = 0; i < thread_num; ++i) {
        thread_vector.push_back(thread = new thread_t(this));
        err = pthread_create(&thread->handle, NULL, thread_func, thread);
        if (err != 0) {
            log_stderr(err, "to create therad[%d] failed, err=%d", i, err);
            return false;
        } else {
            // log_stderr(0, "to create therad[%d] succesed,tid=%ui", i, thread->handle);
        }
    }

    // We must ensure that each thread is started and runs to pthread_cond_wait() before this function returns.
    // Only in this way can these threads perform their subsequent normal operations.
    std::vector<lp_thread_t>::iterator iter;
lblfor:
    for (iter = thread_vector.begin(); iter != thread_vector.end(); iter++) {
        if ((*iter)->is_running == false) {
            usleep(100 * 1000);  // Sleep 100ms
            goto lblfor;
        }
    }
    return true;
}

void* CThreadPool::thread_func(void* threa_data) {
    lp_thread_t therad = static_cast<lp_thread_t>(threa_data);
    CThreadPool* thread_pool = therad->This;

    CMemory* memory = CMemory::GetInstance();
    int err;

    pthread_t tid = pthread_self();
    while (true) {
        err = pthread_mutex_lock(&pool_pthread_mutex);
        if (err != 0) {
            log_stderr(err, "pthread_mutex_lock() failed in CThreadPool::thread_func(), err=%d", err);
        }

        // During the initialization phase, put the threads into sleep mode.
        while (thread_pool->msg_queue.size() == 0 && shutdown == false) {
            if (therad->is_running == false) {
                therad->is_running = true;
            }

            pthread_cond_wait(&pool_pthread_cond, &pool_pthread_mutex);
        }

        // First, check the condition for thread exit.
        if (shutdown) {
            pthread_mutex_unlock(&pool_pthread_mutex);
            break;
        }

        // Processing the message now.  Please note, it's still under mutual exclusion.
        char* job_buffer = thread_pool->msg_queue.front();
        thread_pool->msg_queue.pop_front();
        --thread_pool->msg_queue_size;

        err = pthread_mutex_unlock(&pool_pthread_mutex);
        if (err != 0) {
            log_stderr(err, "pthread_cond_wait() failed in CThreadPool::thread_func(), err=%d", err);
        }

        ++thread_pool->running_thread_num;

        G_SOCKET.ThreadRecvProcFunc(job_buffer);

        memory->FreeMemory(job_buffer);
        --thread_pool->running_thread_num;
    }

    return (void*)0;
}

void CThreadPool::StopAll() {
    if (shutdown == true) {
        return;
    }
    shutdown = true;

    int err = pthread_cond_broadcast(&pool_pthread_cond);
    if (err != 0) {
        log_stderr(err, "pthread_cond_broadcast() failed in CThreadPool::StopAll(), err=%d", err);
        return;
    }

    std::vector<lp_thread_t>::iterator iter;
    for (iter = thread_vector.begin(); iter != thread_vector.end(); iter++) {
        pthread_join((*iter)->handle, NULL);  // Waiting for a thread end.
    }

    pthread_mutex_destroy(&pool_pthread_mutex);
    pthread_cond_destroy(&pool_pthread_cond);

    for (iter = thread_vector.begin(); iter != thread_vector.end(); iter++) {
        if (*iter) {
            delete *iter;
        }
    }
    thread_vector.clear();

    log_stderr(0, "CThreadPool::StopAll() returns successfully, all threads in the thread pool end normally");
    return;
}

void CThreadPool::PushToMsgQueueAndAwake(char* buffer) {
    int err = pthread_mutex_lock(&pool_pthread_mutex);
    if (err != 0) {
        log_stderr(err, "pthread_mutex_lock() failed in CThreadPool::PushToMsgQueueAndAwake(),err=%d", err);
    }

    msg_queue.push_back(buffer);
    ++msg_queue_size;

    err = pthread_mutex_unlock(&pool_pthread_mutex);
    if (err != 0) {
        log_stderr(err, "pthread_mutex_unlock() failed in CThreadPool::PushToMsgQueueAndAwake(),err=%d", err);
    }

    Call();
    return;
}

void CThreadPool::Call() {
    int err = pthread_cond_signal(&pool_pthread_cond);
    if (err != 0) {
        log_stderr(err, "pthread_cond_signal() failed in CThreadPool::Call(), err=%d", err);
    }

    if (thread_num == running_thread_num) {
        time_t curr_time = time(NULL);
        if (curr_time - last_emg_time > 10) {
            last_emg_time = curr_time;
            log_stderr(0, "the current number of idle threads in the thread pool is 0, which means it's time to consider expanding the thread pool");
        }
    }
    return;
}