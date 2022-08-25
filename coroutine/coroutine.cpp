
#include "coroutine.h"


namespace crt {

CoroutineController::CoroutineController() {
    liCrt.push_back(new Coroutine());
    cur = liCrt.begin();
    if (sizeof(void*) == 8) {// 64 bit
        switchContext = asm_switch_context_64;
    }
    else {
        switchContext = asm_switch_context_32;
    }
}

CoroutineController::~CoroutineController() {
    for (auto it = liCrt.begin(); it != liCrt.end();) {
        auto delIt = it;
        it++;
        removeCoroutine(delIt);
    }
}

int CoroutineController::registerCoroutine(Callback func, void* arg) {
    Coroutine* c = new Coroutine();
    c->flags = Flags::Create;
    c->func = func;
    c->arg = arg;
    //c->CID = liCrt.back()->CID + 1; //应该不会爆表
    UINT cid = liCrt.back()->CID + 1;
    while (mapCid.count(cid) != 0 || cid == 0) {
        cid++;
    }
    c->CID = cid;
    UCHAR* stack_page = static_cast<UCHAR*>(getStack());
    memset(stack_page, 0, stack_size);
    c->inital_stack = stack_page + stack_size;
    c->stack_limit = stack_page;
    void* t_ESP = c->inital_stack;

    //下面的代码与汇编context有关，
    
    pushStack(&t_ESP, reinterpret_cast<void*>(crt::startCoroutine)); //函数入口 eip (ret指令弹出)
    pushStack(&t_ESP, reinterpret_cast<void*>(0x246));        //eflags，无意义
    pushStack(&t_ESP, reinterpret_cast<void*>(0));        //rbp，寄存器初始值(下同)，无意义
    pushStack(&t_ESP, reinterpret_cast<void*>(c));        //rdi，参数1，通过这个参数调用scheduling
    pushStack(&t_ESP, reinterpret_cast<void*>(this));     //rsi，参数2，通过这个值找到func和arg并调用
    pushStack(&t_ESP, reinterpret_cast<void*>(0));        //rbx，无意义
    pushStack(&t_ESP, reinterpret_cast<void*>(0));        //rcx，无意义
    pushStack(&t_ESP, reinterpret_cast<void*>(0));        //rdx，无意义  
    pushStack(&t_ESP, reinterpret_cast<void*>(0));        //rax，无意义

    c->cur_stack = t_ESP;
    c->flags = Flags::Ready;//create --> ready
    //加入链表
    liCrt.push_back(c);
    //加入索引表
    mapCid[c->CID] = c;

    return c->CID;
}

void CoroutineController::scheduling() {
    Coroutine* newCoroutine = *liCrt.begin();
    Coroutine* oldCoroutine = *cur;
    ULNOG tick = getTimestamp();

    ++cur; //当前的执行完了，指向下一个
    auto it = cur;

    cur = liCrt.begin();

    for (; it != liCrt.end(); it++) {
        Coroutine* c = *it;
        int tflags = c->flags;

        if (tflags & Flags::Suspend) continue;
        //if (c->flags & Flags::Exit) continue;
        if (tflags & Flags::Exit) {
            if (c != *cur) {
                //不是当前线程，且退出了，直接清理
                auto delIt = it;
                it--;
                removeCoroutine(delIt);
            }
            continue;
        }

        if (tflags & Flags::Sleep) {
            if (c->sleep_millisecond_dot < tick) {
                unsetFlag(c, Flags::Sleep);
                setFlag(c, Flags::Ready);
            }
        }

        if (tflags & Flags::Ready) {
            newCoroutine = c;
            cur = it; //指向新的协程
            break;
        }
    }
    setFlag  (newCoroutine, Flags::Running);  
    unsetFlag(newCoroutine, Flags::Ready);//ready --> running

    unsetFlag(oldCoroutine, Flags::Running);
    // Exit 和 Sleep 状态下不能 Ready
    if ((oldCoroutine->flags & (Flags::Exit | Flags::Sleep)) == 0)
        setFlag(oldCoroutine, Flags::Ready);
    switchContext(newCoroutine, oldCoroutine);
}


void CoroutineController::sleep(UINT Milliseconds) {
    //设置苏醒时间，并设置flags，然后将执行权交给其他协程
    (*cur)->sleep_millisecond_dot = getTimestamp() + Milliseconds;
    setFlag(*cur, Flags::Sleep);
    unsetFlag(*cur, Flags::Ready | Flags::Running);
    scheduling();//让出执行权
}


int CoroutineController::suspend(UINT cid) {
    if (mapCid.find(cid) == mapCid.end()) return -1;

    Coroutine* c = mapCid[cid];
    setFlag(c, Flags::Suspend);
    unsetFlag(c, Flags::Ready | Flags::Running);
    scheduling();//让出执行权
    return 0;
}

int CoroutineController::suspend() {
    Coroutine* c = *cur;
    setFlag(c, Flags::Suspend);
    unsetFlag(c, Flags::Ready | Flags::Running);
    scheduling();//让出执行权
}

int CoroutineController::resume(UINT cid) {
    if (mapCid.find(cid) == mapCid.end()) return -1;
    Coroutine* c = mapCid[cid];
    unsetFlag(c, Flags::Suspend);
    if ((c->flags & (Flags::Exit | Flags::Sleep)) == 0)
        setFlag(c, Flags::Ready);
    return 0;
}

int CoroutineController::terminate(UINT cid) {
    if (cid == 0) return -1; //不许退出协程0
    if (mapCid.find(cid) == mapCid.end()) return -2; //找不到这个协程
    Coroutine* c = mapCid[cid];
    c->flags = Flags::Exit; //标记退出
    if (cid == (*cur)->CID) { //退出当前协程让出执行权
        scheduling();
    }
    return 0;
}

crt::UINT CoroutineController::getCoroutineStatus(UINT cid) {
    return mapCid[cid]->flags;
}

void CoroutineController::idleCoroutine(void* arg) {
    usleep(20000);  //休息20ms
    scheduling();   //继续搜索当前可执行的协程
}

void CoroutineController::setFlag(Coroutine* c, UINT flag) {
    c->flags |= flag;
}

void CoroutineController::unsetFlag(Coroutine* c, UINT flag) {
    c->flags &= ~flag;
}

crt::Coroutine* CoroutineController::getCoroutine(UINT cid) {
    if (mapCid.find(cid) != mapCid.end())
        return mapCid[cid];
    return nullptr;
}

void CoroutineController::pushStack(void** stack, void* data) {
    void*** s = reinterpret_cast<void***>(stack);
    (*s)--;
    **s = data;
}


unsigned long CoroutineController::getTimestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void CoroutineController::removeCoroutine(std::list<Coroutine*>::iterator it) {
    munmap((*it)->stack_limit, stack_size);
    mapCid.erase((*it)->CID);
    delete *it;
    liCrt.erase(it);
}

void* CoroutineController::getStack() {
    return mmap(0, stack_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
}

void startCoroutine(Coroutine* c, CoroutineController* ccb) {
    c->func(c->arg);
    c->flags = CoCtrl::Flags::Exit; //标记退出
    //这里不能直接删除，因为删除后这块地址不能使用，switchContext里是会出错的
    ccb->scheduling();
}

};

