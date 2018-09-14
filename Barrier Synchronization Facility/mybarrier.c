#include <stdlib.h>
#include "mybarrier.h"

mybarrier_t * mybarrier_init(unsigned int count)
{
    mybarrier_t * barrier = malloc(sizeof(*barrier));
    
	if (NULL == barrier) {
        return NULL;
    }

	//Initialize count, reached_count, mutex and condition variable
	barrier->count = count;
	barrier->reached_count = 0;
	pthread_mutex_init(&barrier->mutex, NULL);
	pthread_cond_init(&barrier->cond, NULL);

    return barrier;
}

int mybarrier_destroy(mybarrier_t * barrier)
{
	int ret = 0;

	if(barrier == NULL)
		return -1;

	pthread_mutex_lock(&barrier->mutex);

	//check if still there are threads blocking at mybarrier_wait()
	while(barrier->reached_count < barrier->count) {
		pthread_cond_wait(&barrier->cond, &barrier->mutex);
	}

	pthread_mutex_unlock(&barrier->mutex);

	//destroy the mutex and condition variable, free the memory space allocated for the mybarrier structure
	if(ret = pthread_cond_destroy(&barrier->cond)) {
		return ret;
	}
	if(ret = pthread_mutex_destroy(&barrier->mutex)) {
		return ret;
	}
	free(barrier);
	return ret;
}

int mybarrier_wait(mybarrier_t * barrier)
{
	int ret = 0;

	if(barrier == NULL)
		return -1;

	pthread_mutex_lock(&barrier->mutex);
	
	//increment the count of threads calling mybarrier_wait()
	barrier->reached_count++;

	//awaken the threads after count-th call
	if(barrier->reached_count == barrier->count) {
		pthread_cond_broadcast(&barrier->cond);
		pthread_mutex_unlock(&barrier->mutex);
		return 0;
	}
	//return -1 for all calls after count
	else if(barrier->reached_count > barrier->count) {
		pthread_mutex_unlock(&barrier->mutex);
		return -1;
	}
	//wait for other threads
	else {
		while(barrier->reached_count < barrier->count) {
			pthread_cond_wait(&barrier->cond, &barrier->mutex);
		}
		pthread_mutex_unlock(&barrier->mutex);
		return 0;
	}

	return ret;
}
