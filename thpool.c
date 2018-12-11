#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include "thpool.h"


static uint_t th_num = 0;			//线程数量
static uint_t th_capa = 0;			//线程池容量

//线程循环变量
static int isExit = 1;


//线程结构体
typedef struct thread {
    uint_t id;						//线程编号
    pthread_t thread;				//线程ID
}thread;


//将队列和线程绑定
typedef struct thpool{
    uint_t flags;					//线程标志位
    struct annulus * queue;			//队列
    struct thread * pthread;		//线程
}thpool;


//队列结构体
typedef struct annulus {
    uint_t flags;					//队列标识
    uint_t in;						//输入指针
    uint_t out;						//输出指针
    uint_t qsize;					//队列元素个数
    uint_t capacity;				//队列容量
    void ** data;					//队列数据指针
}annulus;



/*============================================================================*/

//创建队列
static annulus * annulus_queue_init(uint_t capacity);

//入队列g
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

//负载均衡
static uint_t load_balancing(thpool *queue);

/*============================================================================*/

//初始化
thpool * thpool_init(uint_t capacity, uint_t thsize,uint_t qlen)
{
    if(thsize > capacity){
        fprintf(stderr,"thpool_init  输入的参数有误");
        return NULL;
    }

    if(qlen == 0 ){
        qlen = 1024;
    }


    thpool * pool = (thpool *)calloc(sizeof(thpool), capacity);
    if(pool == NULL){
        fprintf(stderr,"thpool_init error");
        return NULL;
    }

    for(uint_t i =0 ; i < thsize ; ++i){    	//每条线程对应一个队列		
        annulus * queue = annulus_queue_init(qlen);
        if(queue == NULL) {
            fprintf(stderr,"annulus_queue_init create error");
            return NULL;
        }

        thread * pthread = thread_init(i,queue); //将队列指针传入线程回调函数
        if(pthread == NULL) {
            fprintf(stderr,"thread_init error");
            return NULL;
        }

        pool[i].flags = 1;						//赋值
        pool[i].queue = queue;
        pool[i].pthread = pthread;
    }

    th_num = thsize;
    th_capa = capacity;

    return pool;
}

//返回返回线程池的线程数量
    uint_t 
thpool_num()
{
    return th_num;
}

//轮询添加任务
    void
thpool_add_task(thpool* pool,job *job,ADD_MODEL model)
{
    if(pool == NULL){
        fprintf(stderr,"thpool is NULL");
        return;
    }
    //轮询添加任务
    static uint_t task_num = 0;			//向哪个线程添加任务
    annulus * queue = NULL;

    //轮询模式添加任务
    if(model == ADD_POLL) {
        while(1) {
            queue = pool[task_num].queue;
            if(queue == NULL) {
                task_num = (++task_num) % th_num;
                continue;
            }

            if(queue->qsize == queue->capacity || queue->flags == 0 ){
                task_num = (++task_num) % th_num;
                continue;
            } else {
                annulus_queue_push(queue,(void*)job);
                task_num = (++task_num) % th_num;
                break;
            }
        }

    }

    //负载均衡模式
    else if(model == ADD_LB) {
        
        while(1) {
            uint_t num =  load_balancing(pool);
            queue = pool[num].queue;
            if(queue == NULL) {
                continue;
            }

            if(queue->qsize == queue->capacity || queue->flags == 0 ){
                continue;
            } else {
                annulus_queue_push(queue,(void*)job);
                break;
            }
        }
    }
}

//增加线程数量
    uint_t 
thpool_add_thread(thpool *pool,uint_t thsize)
{
    if(pool == NULL || thsize == 0) {
        fprintf(stderr,"thpool_add_thread 输入的参数有误");
        return 0;
    }

    uint_t temp = th_capa - th_num;
    if(thsize > temp) {
        thsize = temp;
    }

    uint_t num = thsize;
    for(uint_t i = 0; i < th_capa && num >0; ++i)
    {
        if(pool[i].flags == 0) {
            annulus * queue = annulus_queue_init(1024);
            if(queue == NULL) {
                fprintf(stderr,"annulus_queue_init create error");
                return 0;
            }

            thread * pthread = thread_init(i,queue); //将队列指针传入线程回调函数
            if(pthread == NULL) {
                fprintf(stderr,"thread_init error");
                return 0;
            }

            pool[i].flags = 1;						//赋值
            pool[i].queue = queue;
            pool[i].pthread = pthread;
            --num;
            ++th_num;
        }
    }

    return thsize;
}

//减少线程
    uint_t 
thpool_sub_thread(thpool * pool, uint_t thsize)
{
    uint num = 0;
    for(uint i = th_num -1; num < thsize && i > 0; --i)
    {
        if(pool[i].queue->flags == 1 && pool[i].flags == 1 ){
            pool[i].queue->flags = 0;
            free(pool[i].pthread);
            pool[i].pthread = NULL;
            pool[i].flags=0;
            ++num;
            --th_num;
        }
    }
    return num;
}


//返回还在工作的线程数量
    uint_t
thpool_get_thread()
{
    return th_num;
}


//销毁线程池
    void 
thpool_destroy(thpool *pool)
{
    if(pool == NULL){
        return;
    }

    for(uint_t i = 0; i < th_capa; ++i){
        if(pool[i].flags == 1 ) {
            pool[i].flags = 0;
            free(pool[i].pthread);
            annulus_queue_destroy(pool[i].queue );	//释放队列内存
        }
    }
    sleep(1);
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
    struct job * pjob = NULL;

    while(isExit) {
        if(queue->flags == 0 && annulus_queue_size(queue) == 0)
        {
            free(queue->data);
            free(queue);
            break;
        }

        if(annulus_queue_size(queue) > 0) 
        {
            pjob =(struct job*)annulus_queue_pull(queue);
            if(pjob == NULL){
                continue;
            }
            pjob->task(pjob->arg);
            continue;
        }
        usleep(5);
    }
    //printf("线程[%ld]退出\n",pthread_self());
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

    queue->flags = 1;
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
    annulus * pQueue = (annulus *)queue;
    if(pQueue->flags == 1) {
        pQueue->flags =0;
    }

}

//负载均衡
static uint_t 
load_balancing(thpool* pool)
{    
    uint_t num = pool[0].queue->qsize;
    uint_t index = 0;
    for(uint_t i = 1; i <th_num ; ++i) 
    {
        if(num > pool[i].queue->qsize){
            num = pool[i].queue->qsize;
            index = i;
        }
    }
    return index;
}





