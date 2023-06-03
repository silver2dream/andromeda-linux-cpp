#ifndef __ANDRO_THREADPOOL_H__
#define __ANDRO_THREADPOOL_H__

#include <pthread.h>

#include <atomic>
#include <vector>

class CThreadPool {
   public:
    CThreadPool();
    ~CThreadPool();

   public:
    bool Create(int in_thread_num);
    void StopAll();
    void Call(int msg_count);

   private:
    static void* ThreadFunc(void* thread_data);

   private:
    typedef struct andro_thread_s {
        pthread_t handle;
        CThreadPool* This;
        bool is_running;

        andro_thread_s(CThreadPool* pool) : This(pool), is_running(false) {}

        ~andro_thread_s() {}

    } thread_t, *lp_thread_t;

   private:
    static pthread_mutex_t pthread_mutex;
    static pthread_cond_t pthread_cond;
    static bool shutdown;

    int thread_num;

    std::atomic<int> running_thread_num;
    time_t last_emg_time;

    std::vector<lp_thread_t> thread_vector;
};
#endif