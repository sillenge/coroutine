/*
作者：sillenge

请勿使用多个线程控制一个协程块，这将产生未定义行为
可以单个线程使用多个协程块，但我不建议这么做
*/

#pragma once

#include <iostream>
#include <cstdlib>
#include <memory.h>
#include <list>
#include <functional>
#include <chrono>
#include <unordered_map>

#include <unistd.h>
#include <sys/mman.h>

#define COROUTINE_CREATE    0x1
#define COROUTINE_READY     0x2
#define COROUTINE_RUNNING   0x4
#define COROUTINE_SLEEP     0x8
#define COROUTINE_SUSPEND   0x10
#define COROUTINE_EXIT      0x100


namespace crt {
//-------------------------namesapce crt----------------------------

typedef unsigned int UINT;
typedef unsigned char UCHAR;
typedef unsigned long ULNOG;

typedef void (*Callback)(void*);

//注释偏移按64位计算
typedef struct Coroutine {
    void* cur_stack = nullptr;      // +0x0  堆栈当前位置 (EPS) 放到第一个不用区分64和32位 
    void* inital_stack = nullptr;   // +x08  堆栈起始位置 
    void* stack_limit = nullptr;    // +0x10 堆栈界限 

    int CID = 0;                    // +0x18 协程ID，0保留给当前协程 
    UINT flags = 0;                 // +0x1c 状态    
    ULNOG sleep_millisecond_dot = 0;// +0x20 休眠时间, 这里不够其实可以增加到longlong

    
    Callback func = nullptr;        // +0x28 线程函数
    void* arg = nullptr;            // +0x30 线程函数的参数
    void* arg2 = nullptr;            // +0x38 线程函数的参数
}CRT;


typedef class CoroutineController {
public:
    enum Flags {
        unknow = 0x0,
        Create = 0x1,
        Ready = 0x2,
        Running = 0x4,
        Sleep = 0x8,
        Suspend = 0x10,
        Exit = 0x100
    };
private:
    size_t stack_size = 0x80000;        //将会申请的堆栈大小
    void* stack_limit = 0;              //保留

    std::list<Coroutine*> liCrt;
    std::list<Coroutine*>::iterator cur;
    
protected:
    std::unordered_map<UINT, Coroutine*> mapCid;
    //子类可以通过cid索引到Coroutine结构体
    Coroutine* getCoroutine(UINT cid);

private:
    //push void* (32位下sizeof(void*) = 4 --- 64位下sizeof(void*) = 8)
    void pushStack(void**  stack, void* data);
    //调用汇编函数前的传参，需要确保传参和使用参数方式一致，减少if的使用
    void (*switchContext)(Coroutine* newCoroutine, Coroutine* oldCoroutine) = nullptr;
    
    inline void setFlag(Coroutine* c, UINT flag);
    inline void unsetFlag(Coroutine* c, UINT flag);
    inline unsigned long getTimestamp();
    //从链表中删除一个协程
    inline void removeCoroutine(std::list<Coroutine*>::iterator it);
    //方便后序拓展
    inline void* getStack();

public:
    //初始化后，保留第一个留给当前协程使用
    CoroutineController();
    ~CoroutineController();
    //休息20ms后继续
    void idleCoroutine(void* arg);
    //向CoCtrl中注册一个协程，返回一个CID (协程ID)
    int  registerCoroutine(Callback func, void* arg, void* arg2 = nullptr);
    //调度协程
    void scheduling();
    //睡眠指定时间，单位：毫秒
    void sleep(UINT Milliseconds);
    //挂起自己
    int suspend();
    //挂起指定协程
    int suspend(UINT cid);
    //恢复指定协程
    int resume(UINT cid);
    //结束指定协程
    int terminate(UINT cid);
    //通过CID获取Coroutine状态
    UINT getCoroutineStatus(UINT cid);
}CoCtrl;

//如果这个能用，那么startCoroutine可以写成私有函数
template<typename dst_type, typename src_type>
dst_type pointer_cast(src_type src) {
    return *static_cast<dst_type*>(static_cast<void*>(&src));
}
//这个是非成员函数, 成员函数取不出函数地址（或者是我太菜了）
void startCoroutine(Coroutine* c, CoroutineController* ccb);


extern "C" void asm_switch_context_32(Coroutine * oldCo, Coroutine * newCo);
extern "C" void asm_switch_context_64(Coroutine * oldCo, Coroutine * newCo);

}

