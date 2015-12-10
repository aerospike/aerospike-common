#include <pthread.h>

extern void cf_mutex_dump(void);

#if defined ENHANCED_THREADS

extern int cf_thread_create(pthread_t *thread, const pthread_attr_t *attr,
		void *(*start_routine)(void *), void *arg);

extern int cf_mutex_lock_do(pthread_mutex_t *mutex, const char *file_name, int line_no);
extern int cf_mutex_trylock_do(pthread_mutex_t *mutex, const char *file_name, int line_no);
extern int cf_mutex_unlock_do(pthread_mutex_t *mutex);

#define cf_mutex_lock(mutex) cf_mutex_lock_do(mutex, __FILE__, __LINE__)
#define cf_mutex_trylock(mutex) cf_mutex_trylock_do(mutex, __FILE__, __LINE__)
#define cf_mutex_unlock(mutex) cf_mutex_unlock_do(mutex)

#else

#define cf_thread_create(thread, attr, start_routine, arg) \
		pthread_create(thread, attr, start_routine, arg)

#define cf_mutex_lock(mutex) pthread_mutex_lock(mutex)
#define cf_mutex_trylock(mutex) pthread_mutex_trylock(mutex)
#define cf_mutex_unlock(mutex) pthread_mutex_unlock(mutex)

#endif
