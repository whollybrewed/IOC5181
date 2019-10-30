#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef unsigned long long BIGINT;

BIGINT total_num_hits;
BIGINT toss_per_thread;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *dart_toss ()
{
	BIGINT num_hits = 0;
	unsigned int state = rand();
	for (BIGINT toss = 0; toss < toss_per_thread; toss++){
		double x = rand_r(&state) / ((double)RAND_MAX + 1);
		double y = rand_r(&state) / ((double)RAND_MAX + 1);
		if ( x * x + y * y <= 1){
			num_hits++;
		}
	}
	pthread_mutex_lock(&mutex);
	total_num_hits += num_hits;
	pthread_mutex_unlock(&mutex);
	pthread_exit(NULL);
}

int main(int argc, char **argv)
{	
	srand(time(NULL));
	double pi_estimate;
	unsigned long long  num_cpu, num_toss, num_thread;
	if ( argc < 2) {
		exit(-1);
	}
	num_cpu = atoi(argv[1]);
	num_toss = atoi(argv[2]);
	if (( num_cpu < 1) || ( num_toss < 0)) {
		exit(-1);
	}
	num_thread = num_cpu;
	toss_per_thread = num_toss / num_thread;
	pthread_t tid[num_thread];
	for(int i = 0; i < num_thread; i++){
		pthread_create(&tid[i], NULL, dart_toss,(void*)NULL);
	}
	for (int i = 0; i < num_thread; i++) {
		pthread_join(tid[i], NULL);
	}
	pthread_mutex_destroy(&mutex);
	pi_estimate = 4 * total_num_hits/((double)num_toss);
	printf("%f\n", pi_estimate);
	return 0;
}
