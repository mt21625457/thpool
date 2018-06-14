#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include"thpool.h"

pthread_mutex_t mutex;

static int n =0;

void pp(void *arg)
{
	pthread_mutex_lock(&mutex);
	printf("[%ld]---%d\n",pthread_self(),n++);
	pthread_mutex_unlock(&mutex);
	
}	


void test01()
{
	job * job = (struct job *) malloc (sizeof(struct job));
	
	job->task = pp;
	job->arg =NULL;
	
	thpool * pool =  thpool_init(10);
	if( pool ==NULL) {
		return;
	}

	for(int i = 0; i< 1000000; i++){
		thpool_add(pool,job);
	}
	
	while(1){
		sleep(1);
	}

	thpool_destroy(pool);
	free(job);

}


int main()
{
	pthread_mutex_init(&mutex,NULL);
	test01();
	return 0;
}
