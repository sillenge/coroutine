/*
这个协程池是仿造线程池的结构写的

协程池要设计好感觉很难，关键的运行协程的cid需要传出
因为协程不受操作系统调度，所以只能由用户自己控制
而cid是控制协程调度的关键，但调度时才清楚执行任务的cid
所以使用线程池的结构写协程池，易用性就被削弱了很多
*/
#pragma once
#include "coroutine.h"
#include <queue>
#include <functional>


namespace crt {

struct CoArgs {
    UINT cid;    //当协程开始执行此任务时填充
    void* other; //其他自定义参数
};
typedef void (*_callback)(CoArgs* args);
struct CoTask {
    _callback func;   //需要执行的任务
    CoArgs args;
};

//协程增长倍率，但增长和减少在 [min_size, max_size] 间
//当任务超过协程很多时 alive += alive*0.2 (alive*0.2为0时，置为1，下同)
//当协程空闲过多时     alive -= alive*0.2 
const float add_rate = 0.2;


template<typename ResultFunction, typename Function>
ResultFunction function_cast(Function fun) {
    ResultFunction result = ResultFunction(*(reinterpret_cast<int*>(&fun)));
    return result;
}



class CoroutinePool : public crt::CoroutineController {
private:
    size_t max_size;
    size_t min_size;

    size_t num_alive;
    size_t num_busy;

    bool shutdown;

    std::queue<CoTask> tasks;
private:
    //协程池主循环
    //参数 mainArgs ：需要符合void*的传参规则，只能创建一个结构体往里面放参数再传递
    static void worker(void*);
    
    //<cid , busy_flag>
    //UINT cid : 协程id
    //bool busy_flag : 此协程是否正在执行任务
    std::vector<std::pair<UINT, bool>> arr_cid;
    inline CoTask popTask();
    void manager(void*);
public:
    CoroutinePool(size_t max_size, size_t min_size);
    ~CoroutinePool();
    size_t getBusyNum() { return num_busy; }
    size_t getAliveNum() { return num_alive; }
    size_t getTasksSize();
    UINT getIdleCoroutine();
    
    void close();
    void addTask(CoTask& task);
    
    void scheduling();
};

};