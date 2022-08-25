#include "coPool.h"

namespace crt {

CoroutinePool::CoroutinePool(size_t max_size, size_t min_size) : CoCtrl::CoroutineController() {
    if (min_size == 0) min_size = 1;
    if (max_size < min_size) max_size = min_size;
    this->max_size = max_size;
    this->min_size = min_size;
    //0��ʹ��
    arr_cid.resize(max_size+1);
    for (size_t i = 1; i < max_size+1; i++) {
        arr_cid[i].first = registerCoroutine(
            worker, static_cast<void*>(new std::pair<CoroutinePool*, UINT>(this, i)));
        arr_cid[i].second = false;
    }
    num_alive = max_size;
    scheduling();
}



CoroutinePool::~CoroutinePool() {
    
}

void CoroutinePool::close() {
    shutdown = true;
    for (size_t i = 1; i < max_size + 1; i++) {
        UINT cid = arr_cid[i].first;
        if (cid == 0) continue;
        getCoroutine(cid)->flags = Flags::Exit;
    }
    arr_cid.clear();
}

void CoroutinePool::addTask(CoTask& task) {
    tasks.push(task);
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
    delete p;//��Ϊ�������new����new�治�ô�
    p = nullptr;

    while (!cp->shutdown) {
        cp->suspend();//���𣬵ȼ���
        if (cp->getCoroutineStatus(cp->arr_cid[index].first) & Flags::Exit || cp->shutdown) {
            break;
        }
        //����Ϊ��ʱ��idle
        while (cp->tasks.empty()) {
            cp->idleCoroutine(NULL);
        }
        CoTask task = cp->tasks.front();
        cp->tasks.pop();
        cp->num_busy++;
        cp->arr_cid[index].second = true;//��ʾ��Э��������������

        //����ǰ���ò���cid���Դ˷������� ����Э�̵�cid
        task.args.cid = cp->arr_cid[index].first;
        //��������
        task.func(&task.args);

        cp->arr_cid[index].second = false;
        memset(&task, 0, sizeof(CoTask));
        cp->num_busy--;
        cp->scheduling();
    }
    cp->scheduling();
    return;
}

CoTask CoroutinePool::popTask(){
    CoTask task = tasks.front();
    tasks.pop();
    return task;
}

void CoroutinePool::manager(void*) {
    while (!shutdown) {
        this->sleep(5000);//5��
        size_t tasks_size = this->tasks.size();
        size_t alive = this->num_alive;
        size_t busy = this->num_busy;
        //����������ٵ�����
        size_t addition = alive * add_rate == 0 ? 1 : alive * add_rate;
        // ���񲻵����Э������1/2��ɾ�������Э��
        // alive/3 ���ܻ���0���� task_size*3 ���ܻ����������Ӧ�ò�������ô��Э�̵�
        if (tasks_size * 2 < alive && alive > min_size) {
            // ���ٵ� min_size ����� 0.2*alive ��
            size_t eixtNum = alive - addition < min_size ? alive - min_size : addition;
            for (size_t i = 1; i < max_size+1 && eixtNum; i++) {
                //����busy��Э�̣����Ĺ�����δ���
                if (arr_cid[i].second == true) continue;

                UINT cid = arr_cid[i].first;
                getCoroutine(cid)->flags = Flags::Exit;

                //�������㣬�´μ���ʹ��
                arr_cid[i].first = 0;
                arr_cid[i].second = false;
            }
        }
        //��������������Э�̵�2������Ҫ����Э�̽��д���
        else if (tasks_size / 2 > busy && alive < max_size) {
            //���ӵ� max_size ������ 0.2 * alive ��
            size_t createNum = alive + addition > max_size ? max_size - alive : addition;
            for (size_t i = 1; i < max_size+1; i++) {
                //��λ�ñ�ռ����
                if (arr_cid[i].first != 0) continue;

                arr_cid[i].first = registerCoroutine(
                    worker, new std::pair<CoroutinePool*, UINT>(this, i));
            }
        }
        scheduling();
    }
    return;
}

void CoroutinePool::scheduling() {
    for (size_t i = 1; !tasks.empty() && i < max_size + 1; i++) {
        if (arr_cid[i].second == false) {
            resume(arr_cid[i].first);
            CoCtrl::scheduling();
        }
    }
    CoCtrl::scheduling();
}

};
