#ifndef __ANDRO_THREADPOOL_H__
#define __ANDRO_THREADPOOL_H__

#include <pthread.h>

#include <atomic>
#include <list>
#include <vector>

class CThreadPool {
   public:
    CThreadPool();
    ~CThreadPool();

   public:
    bool Create(int in_thread_num);
    void StopAll();
    void PushToMsgQueueAndAwake(char* buffer);
    void Call();

   private:
    static void* thread_func(void* thread_data);
    void clear_msg_queue();

   private:
    typedef struct andro_thread_s {
        pthread_t handle;
        CThreadPool* This;
        bool is_running;

        andro_thread_s(CThreadPool* pool) : This(pool), is_running(false) {}

        ~andro_thread_s() {}

    } thread_t, *lp_thread_t;

   private:
    static pthread_mutex_t pool_pthread_mutex;
    static pthread_cond_t pool_pthread_cond;
    static bool shutdown;

    int thread_num;

    std::atomic<int> running_thread_num;
    time_t last_emg_time;

    std::vector<lp_thread_t> thread_vector;

    std::list<char*> msg_queue;
    int msg_queue_size;
};
#endif