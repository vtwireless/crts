// Simple pthread_*() wrappers.


#define BARRIER_WAIT(barrier)                                           \
    do {                                                                \
        DSPEW("Enter barrier");                                         \
        int ret = pthread_barrier_wait(barrier);                        \
        ASSERT(ret == 0 || ret == PTHREAD_BARRIER_SERIAL_THREAD,        \
                "pthread_barrier_wait()=%d failed", ret);               \
        DSPEW("Exit barrier");                                          \
    } while(0)

#if 0
#  define MSPEW(x) DSPEW(x)
#else
#  define MSPEW(x) /*empty macro*/
#endif

#define MUTEX_LOCK(mutex)                                       \
    do {                                                        \
        MSPEW("Locking Mutex");                                 \
        errno = pthread_mutex_lock(mutex);                      \
        ASSERT(errno == 0, "pthread_mutex_lock() failed");      \
        MSPEW("Mutex Locked");                                  \
    } while(0)

#define MUTEX_UNLOCK(mutex)                                     \
    do {                                                        \
        MSPEW("Unlocking Mutex");                               \
        errno = pthread_mutex_unlock(mutex);                    \
        ASSERT(errno == 0, "pthread_mutex_unlock() failed");    \
        MSPEW("Mutex Unlocked");                                \
    } while(0)


// We need to recall pthread_cond_wait() in the main thread each time a
// signal is caught.

#define COND_WAIT(cond, mutex)                                          \
    do {                                                                \
        int ret;                                                        \
        /*DSPEW("thread waiting on cond");*/                            \
        while((ret = pthread_cond_wait((cond), (mutex))) == EINTR);     \
        ASSERT(ret == 0, "pthread_cond_wait()=%d failed", ret);         \
        /*DSPEW("Exit cond"); */                                        \
    } while(0)
