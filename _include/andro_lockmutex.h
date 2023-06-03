#ifndef __ANDRO_LOCKMUTEX_H__
#define __ANDRO_LOCKMUTEX_H__

#include <pthread.h>

class CLock {
   public:
    CLock(pthread_mutex_t *in_mutex) {
        mutex = in_mutex;
        pthread_mutex_lock(mutex);
    }

    ~CLock() {
        pthread_mutex_unlock(mutex);
    }

   private:
    pthread_mutex_t *mutex;
};
#endif