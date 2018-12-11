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
	
	thpool * pool =  thpool_init(10,2,1024);
	if( pool ==NULL) {
		return;
	}
	
	uint_t n = thpool_add_thread(pool,1);
	if(n != 1) {
		fprintf(stderr,"增加线程失败!");
		return ;
	} else {
		printf("添加一条线程成功\n");
	}


	for(int i = 0; i< 3; i++){
		thpool_add_task(pool,job,ADD_LB);
	}
	
	sleep(2);

	uint_t nu = thpool_sub_thread(pool,2);
	if(nu != 2) {
		fprintf(stderr,"减少线程失败");
	} else {
		printf("减少2条线程成功\n");
	}
	
	for(int i = 0; i< 3; i++){
		thpool_add_task(pool,job,ADD_LB);
	}
	
	uint_t th = thpool_get_thread();
	printf("还有子线程:%d\n",th);

	
	sleep(2);
	

	thpool_destroy(pool);
	free(job);

}


int main()
{
	pthread_mutex_init(&mutex,NULL);
	test01();
	return 0;
}
