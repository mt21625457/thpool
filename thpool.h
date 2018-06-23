#include <stdio.h>

//添加任务模式
typedef enum ADD_MODEL {
    ADD_POLL = 0,       //轮询模式
    ADD_LB,             //负载均衡
}ADD_MODEL;

typedef unsigned int uint_t;

typedef struct thpool thpool;

//任务结构体
typedef struct job {
	void(*task)(void*);			//线程ID
	void *arg;					//线程参数
}job;

//创建线程
//@capacity 		线程池的容量
//@thsize 			初始线程数量 必须小于等于线程池容量
//@qlen             任务队列大小,传 0 默认为1024
//@thpool			返回线程池指针
thpool * thpool_init(uint_t capacity,uint_t thsize,uint_t qlen);

//添加任务
//@thpool  			创建的线程池指针
//@job				要添加的任务
//@model            添加任务的模式(轮询 或者 负载均衡)
void thpool_add_task(thpool *pool, job *job,ADD_MODEL model);

//增加线程数量
//@pool  			线程池指针
//@thsize			要增加的线程数量  总数量不能大于线程容量
//@return			返回成功增加的线程数量
uint_t thpool_add_thread(thpool * pool, uint_t thsize);

//减少线程数量
//@pool 			线程池指针
//@thsize 			要减少的线程池数量
//@return 			实际减少的线程数量
uint_t thpool_sub_thread(thpool * pool, uint_t thsize);

//返回在工作的线程数量
uint_t thpool_get_thread();

//销毁线程池
//@pool				创建要销毁的线程池指针
void thpool_destroy(thpool *pool);
