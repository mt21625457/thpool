#include <llstdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include "thpool.h"

//线程数量
static uint_t thread_num = 0;

//线程循环变量
static int isExit = 1;


//线程结构体
typedef struct thread {
	uint_t id;						//线程编号
	pthread_t thread;				//线程ID
}thread;


//将队列和线程绑定
typedef struct thpool{
	uint_t id;						//id
	struct annulus * queue;			//队列
	struct thread * pthread;		//线程
}thpool;


//队列结构体
typedef struct annulus {
	uint_t id;						//队列ID
	uint_t in;						//输入指针
	uint_t out;						//输出指针
	uint_t qsize;					//队列元素个数
	uint_t capacity;				//队列容量
	void ** data;					//队列数据指针
}annulus;



/*============================================================================*/

//创建队列
static annulus * annulus_queue_init(uint_t capacity);

//入队列
static int annulus_queue_push(void * queue,void *arg);

//出队列
static void * annulus_queue_pull(void *queue);

//返回队列任务数量
static uint_t annulus_queue_size(void * queue);

//销毁队列
static void annulus_queue_destroy(void *queue);

//线程相关
static thread * thread_init(uint_t id,void *arg);

//线程工作回调函数
static void * dowork(void *arg);

/*============================================G================================*/

//初始化
thpool * thpool_init(uint_t thsize)
{
	if(thsize == 0) {
		thsize = 8;
	}

	thpool * pool = (thpool *)calloc(sizeof(thpool), thsize);
	if(pool == NULL){
		fprintf(stderr,"thpool_init error");
		return NULL;
	}

	for(uint_t i =0 ; i < thsize ; ++i){    	//每条线程对应一个队列		
		annulus * queue = annulus_queue_init(1024);
		if(queue == NULL) {
			fprintf(stderr,"annulus_queue_init create error");
			return NULL;
		}

		thread * pthread = thread_init(i,queue); //将队列指针传入线程回调函数
		if(pthread == NULL) {
			fprintf(stderr,"thread_init error");
			return NULL;
		}

		pool[i].id = i;						//赋值
		pool[i].queue = queue;
		pool[i].pthread = pthread;
	}

	thread_num = thsize;

	return pool;
}

//返回返回线程池的线程数量
	uint_t 
thpool_num()
{
	return thread_num;
}

//轮询添加任务
	void
thpool_add(thpool* pool,job *job)
{
	if(pool == NULL){
		fprintf(stderr,"thpool is NULL");
		return;
	}
	//轮询添加任务
	static uint_t task_num = 0;			//向哪个线程添加任务

	annulus * queue = NULL;

	while(1) {
		queue = pool[task_num].queue;
		if(queue->qsize == queue->capacity ){
			task_num = (++task_num) % thread_num;
			continue;
		} else {
			annulus_queue_push(queue,(void*)job);
			task_num = (++task_num) % thread_num;
			break;
		}
	}
}

//销毁线程池
	void 
thpool_destroy(thpool *pool)
{
	if(pool == NULL){
		return;
	}

	thread_exit();
	for(uint_t i = 0; i < thread_num; ++i){
		annulus_queue_destroy(pool[i].queue );	//释放队列内存
		//让线程停止
	}

	free(pool);
}


//创建一个线程 成功返回线程指针  失败返回NULL
	static thread * 
thread_init(uint_t id,void *arg)
{
	thread * th = (thread *)malloc(sizeof(thread));
	if(th == NULL) {
		return NULL;
	}	

	th->id = id;
	//创建线程 设置分离
	int ret  = pthread_create(&th->thread,NULL,dowork,(void *)arg);
	if(ret < 0 ) {
		return NULL;
	}

	pthread_detach(th->thread);
	return th;
}


//线程回调函数
	static void *
dowork(void *arg)
{
	static int isExit = 1;
	annulus * queue = (annulus *)arg;

	while(isExit) {

		if(annulus_queue_size(queue) > 0) 
		{
			struct job * pjob =(struct job*)annulus_queue_pull(queue);
			if(pjob == NULL){
				continue;
			}
			void (*func)(void*);
			void*farg;

			func = pjob->task;
			farg = pjob->arg;
			func(farg);
		}
	}
	pthread_exit(NULL);
}

//创建一个队列 并初始化
//@capacity 	队列容量大小
//@return 		返回创建好的队列
	static annulus * 
annulus_queue_init(uint_t capacity)
{
	annulus * queue = (annulus *)malloc(sizeof(struct annulus));
	if (queue == NULL) {
		fprintf(stderr,"queue malloc error");
		return NULL;
	}

	queue->in = 0;
	queue->out = 0;
	queue->qsize = 0;
	queue->capacity = capacity;

	queue->data = (void **)calloc(sizeof(void *), (size_t)capacity);
	if (queue->data == NULL) {
		fprintf(stderr,"queue data error");
		return NULL;
	}

	for (uint_t i =0; i < queue->capacity; ++i){
		queue->data[i] = NULL;
	}

	return queue;
}

//入队列
	static int 
annulus_queue_push(void * queue,void *arg)
{
	if(queue == NULL || arg == NULL ) {
		return -1;
	}

	annulus * pQueue = (annulus*)queue;
	if (pQueue->qsize == pQueue->capacity ) {
		fprintf(stderr,"queue 队列任务已满");
		return -1;
	}

	//将任务指针存入循环队列
	while(1) {
		if(pQueue->data[pQueue->in] == NULL){
			pQueue->data[pQueue->in]=(void*)arg;
			pQueue->in = (++pQueue->in) % pQueue->capacity;
			++pQueue->qsize;
			break;
		}
		pQueue->in = (++pQueue->in) % pQueue->capacity;
	}
	return 0;
}

//出队列
	static void * 
annulus_queue_pull(void *queue)
{
	annulus * pQueue = (annulus*)queue;

	void *arg = NULL;
	//取出任务
	while(pQueue->qsize != 0) {
		if((arg = pQueue->data[pQueue->out]) == NULL) {	
			pQueue->out = (++pQueue->out) % pQueue->capacity;
			continue;
		}
		break;
	}

	pQueue->data[pQueue->out] = NULL;
	pQueue->out = (++pQueue->out) % pQueue->capacity;
	--pQueue->qsize;
	
	return arg;
}

//返回队列中元素个数
	static uint_t
annulus_queue_size(void *queue)
{
	annulus * pQueue =(annulus*)queue; 
	return pQueue->qsize;
}

//销毁队列
	static void 
annulus_queue_destroy(void *queue)
{
	if(queue == NULL) {
		return;
	}
	annulus * pQueue = (annulus*)queue;
	free(pQueue->data);
	free(pQueue);
}

static void thread_exit()
{
	isExit =0;
}

