#include <stdio.h>

typedef unsigned int uint_t;

typedef struct thpool thpool;

//任务结构体
typedef struct job {
	void(*task)(void*);			//线程ID
	void *arg;					//线程参数
}job;

//创建线程
//@capacity 		要创建的线程数量
//@thpool			返回线程池指针
thpool * thpool_init(uint_t capacity);

//添加任务
//@thpool  			创建的线程池指针
//@job				要添加的任务
void thpool_add(thpool *pool, job *job);

//销毁线程池
//@pool				创建要销毁的线程池指针
void thpool_destroy(thpool *pool);
