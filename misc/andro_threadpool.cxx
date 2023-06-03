
#include "andro_threadpool.h"

#include <stdarg.h>
#include <unistd.h>  //usleep

#include "andro_func.h"
#include "andro_global.h"
#include "andro_macro.h"
#include "andro_memory.h"

pthread_mutex_t CThreadPool::pthread_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t CThreadPool::pthread_cond = PTHREAD_COND_INITIALIZER;
bool CThreadPool::shutdown = false;

CThreadPool::CThreadPool() {
    running_thread_num = 0;
    last_emg_time = 0;
}

CThreadPool::~CThreadPool() {
}

bool CThreadPool::Create(int in_thread_num) {
    lp_thread_t thread;
    int err;

    thread_num = in_thread_num;
    for (int i = 0; i < thread_num; ++i) {
        thread = new thread_t(this);
        thread_vector.push_back(thread);
        err = pthread_create(&thread->handle, NULL, ThreadFunc, thread);
        if (err != 0) {
            log_stderr(err, "to create therad[%d] failed, err=%d", i, err);
            return false;
        }
    }

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

void* CThreadPool::ThreadFunc(void* threa_data) {
    lp_thread_t therad = static_cast<lp_thread_t>(threa_data);
    CThreadPool* thread_pool = therad->This;

    char* job_buffer = nullptr;
    CMemory* memory = CMemory::GetInstance();
    int err;

    pthread_t tid = pthread_self();
    while (true) {
        err = pthread_mutex_lock(&pthread_mutex);
        if (err != 0) {
            log_stderr(err, "pthread_mutex_lock() failed in CThreadPool::ThreadFunc(), err=%d", err);
        }

        while ((job_buffer = G_SOCKET.PopFromMsgQueue()) == nullptr && shutdown == false) {
            if (therad->is_running == false) {
                therad->is_running = true;
            }

            pthread_cond_wait(&pthread_cond, &pthread_mutex);
        }

        err = pthread_mutex_unlock(&pthread_mutex);
        if (err != 0) {
            log_stderr(err, "pthread_cond_wait() failed in CThreadPool::ThreadFunc(), err=%d", err);
        }

        if (shutdown) {
            if (job_buffer != nullptr) {
                memory->FreeMemeory(job_buffer);
            }
            break;
        }

        ++thread_pool->running_thread_num;

        log_stderr(0, "excute begin, tid=%u", tid);
        sleep(5);
        log_stderr(0, "excute end, tid=%u", tid);

        memory->FreeMemeory(job_buffer);
        --thread_pool->running_thread_num;
    }

    return (void*)0;
}

void CThreadPool::StopAll() {
    if (shutdown == true) {
        return;
    }
    shutdown = true;

    int err = pthread_cond_broadcast(&pthread_cond);
    if (err != 0) {
        log_stderr(err, "pthread_cond_broadcast() failed in CThreadPool::StopAll(), err=%d", err);
        return;
    }

    std::vector<lp_thread_t>::iterator iter;
    for (iter = thread_vector.begin(); iter != thread_vector.end(); iter++) {
        pthread_join((*iter)->handle, NULL);  // Waiting for a thread end.
    }

    pthread_mutex_destroy(&pthread_mutex);
    pthread_cond_destroy(&pthread_cond);

    for (iter = thread_vector.begin(); iter != thread_vector.end(); iter++) {
        if (*iter) {
            delete *iter;
        }
    }
    thread_vector.clear();

    log_stderr(0, "CThreadPool::StopAll() returns successfully, all threads in the thread pool end normally");
    return;
}

void CThreadPool::Call(int msg_count) {
    int err = pthread_cond_signal(&pthread_cond);
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