# thpool

test.c :  这是一个简单的测试程序

Threads are only for Linux and Unix,(该线程池只适用于linux和unix)

This is a lightweight, completely unlocked thread pool,(这是一个轻量级完全无锁的线程池)

The default task queue size is 1024(默认任务队列的大小是102)

The task queue size API will be switched out later(后续会将任务队列大小的API开换出来)


创建线程

@capacity         线程池的容量

@thsize           初始线程数量 必须小于等于线程池容量

@thpool           返回线程池指针

thpool * thpool_init(uint_t capacity,uint_t thsize);



添加任务

创建的线程池指针

@job              要添加的任务

void thpool_add_task(thpool *pool, job *job);



增加线程数量

@pool             线程池指针

@thsize           要增加的线程数量  总数量不能大于线程容量

@return           返回成功增加的线程数量

uint_t thpool_add_thread(thpool * pool, uint_t thsize);



减少线程数量

@pool             线程池指针

@thsize           要减少的线程池数量

@return           实际减少的线程数量

uint_t thpool_sub_thread(thpool * pool, uint_t thsize);



返回在工作的线程数量

uint_t thpool_get_thread();



销毁线程池

@pool             创建要销毁的线程池指针

void thpool_destroy(thpool *pool);
                                        
