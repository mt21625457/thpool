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

    //创建一个线程池
    thpool * pool =  thpool_init(10,10,1024);
	if( pool ==NULL) {
		return;
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
