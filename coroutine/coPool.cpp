#include "coPool.h"

namespace crt {

CoroutinePool::CoroutinePool(size_t max_size, size_t min_size) : CoCtrl::CoroutineController() {
    if (min_size == 0) min_size = 1;
    if (max_size < min_size) max_size = min_size;
    
    this->max_size = max_size;
    this->min_size = min_size;
    //0不使用
    arr_cid.resize(max_size+1);
    registerCoroutine(function_cast<Callback>(&CoroutinePool::manager), this);//manager不收集cid
    arr_cid[0].second = true; //主协程
    for (size_t i = 2; i < max_size+1; i++) {
        arr_cid[i].first = registerCoroutine(
            worker, static_cast<void*>(new std::pair<CoroutinePool*, UINT>(this, i)));
        arr_cid[i].second = false;
    }
    num_alive = max_size;
    CoCtrl::scheduling();
}



CoroutinePool::~CoroutinePool() {
    
}

void CoroutinePool::close() {
    shutdown = true;
    for (size_t i = 1; i < max_size + 1; i++) {
        UINT cid = arr_cid[i].first;
        if (cid == 0) continue;
        resume(cid);
        terminate(cid);
    }
    arr_cid.clear();
}

void CoroutinePool::addTask(CoTask* task) {
    tasks.push(task);
}



int CoroutinePool::terminate(UINT cid) {
    if (cid == 0) return -1;
    for (int i = 0; i < max_size+1; i++) {
        if (arr_cid[i].first == cid) {
            if (arr_cid[i].second == true) {
                num_busy--;
            }
            arr_cid[i].first = 0;
            arr_cid[i].second = false;
            num_alive--;
            CoCtrl::terminate(cid);
            return 0;
        }
    }
    return -2;
}

size_t CoroutinePool::getTasksSize() {
    return tasks.size();
}


crt::UINT CoroutinePool::getIdleCoroutine() {
    for (size_t i = 1; i < max_size + 1; i++) {
        if (arr_cid[i].second == false) {
            return arr_cid[i].first;
        }
    }
}

void CoroutinePool::worker(void* args) {
    auto* p = static_cast<std::pair<CoroutinePool*, UINT>*>(args);
    CoroutinePool* cp = p->first;
    UINT index = p->second;
    delete p;//因为传入的是new，不new真不好传
    p = nullptr;

    while (!cp->shutdown) {
        //任务为空时
        while (cp->tasks.empty()) {
            cp->suspend();//挂起，等激活
            if (cp->getCoroutineStatus(cp->arr_cid[index].first) & Flags::Exit || cp->shutdown) {
                return;
            }
        }
        CoTask* task = cp->tasks.front();
        cp->tasks.pop();
        cp->num_busy++;
        cp->arr_cid[index].second = true;//表示此协程有任务在运行

        task->cid = cp->arr_cid[index].first;
        //运行任务
        if (task->args2 == nullptr) {
            task->func(task->args);
        }
        else {
            //void func(void* arg, void* arg2);
            reinterpret_cast<void (*)(void*, void*)>(task->func)(task->args, task->args2);
        }

        cp->arr_cid[index].second = false;
        cp->num_busy--;
    
        cp->CoCtrl::scheduling();
    }
    return;
}

inline CoTask* CoroutinePool::popTask()
{
    CoTask* task = tasks.front();
    tasks.pop();
    return task;
}

void CoroutinePool::manager(void*) {
    while (!shutdown) {
        this->sleep(5000);//5秒
        size_t tasks_size = this->tasks.size();
        size_t alive = this->num_alive;
        size_t busy = this->num_busy;
        //增量，或减少的数量
        size_t addition = ((alive * add_rate) < 1) ? 1 : alive * add_rate;
        // 任务不到存活协程数的1/2，删除多余的协程
        if (tasks_size * 2 < alive && alive > min_size) {
            // 减少到 min_size 或减少 0.2*alive 个
            size_t eixtNum = alive - addition < min_size ? alive - min_size : addition;
            for (size_t i = 1; i < max_size+1 && eixtNum; i++) {
                //跳过busy的协程，他的工作尚未完成
                if (arr_cid[i].second == true) continue;

                UINT cid = arr_cid[i].first;
                getCoroutine(cid)->flags = Flags::Exit;
                
                //回收置零，下次继续使用
                arr_cid[i].first = 0;
                arr_cid[i].second = false;

                eixtNum--;
                alive--;
            }
        }
        //任务数量超过了协程的2倍，需要更多协程进行处理
        else if (tasks_size / 2 >= busy && alive < max_size) {
            //增加到 max_size 或增加 0.2 * alive 个
            size_t createNum = alive + addition > max_size ? max_size - alive : addition;
            for (size_t i = 1; i < max_size+1 && createNum; i++) {
                //此位置被占用了
                if (arr_cid[i].first != 0) continue;

                arr_cid[i].first = registerCoroutine(
                    worker, new std::pair<CoroutinePool*, UINT>(this, i));
                createNum--;
                alive++;
            }
        }
    }
    return;
}

void CoroutinePool::scheduling() {
    for (size_t i = 1; !tasks.empty() && i < max_size + 1; i++) {
        if (arr_cid[i].first != 0 && arr_cid[i].second == false) {
            resume(arr_cid[i].first);
            CoCtrl::scheduling();
        }
    }
    CoCtrl::scheduling();
}

};
