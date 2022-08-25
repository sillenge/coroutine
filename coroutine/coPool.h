/*
���Э�̳��Ƿ����̳߳صĽṹд��

Э�̳�Ҫ��ƺøо����ѣ��ؼ�������Э�̵�cid��Ҫ����
��ΪЭ�̲��ܲ���ϵͳ���ȣ�����ֻ�����û��Լ�����
��cid�ǿ���Э�̵��ȵĹؼ���������ʱ�����ִ�������cid
����ʹ���̳߳صĽṹдЭ�̳أ������Ծͱ������˺ܶ�
*/
#pragma once
#include "coroutine.h"
#include <queue>
#include <functional>


namespace crt {

struct CoArgs {
    UINT cid;    //��Э�̿�ʼִ�д�����ʱ���
    void* other; //�����Զ������
};
typedef void (*_callback)(CoArgs* args);
struct CoTask {
    _callback func;   //��Ҫִ�е�����
    CoArgs args;
};

//Э���������ʣ��������ͼ����� [min_size, max_size] ��
//�����񳬹�Э�̺ܶ�ʱ alive += alive*0.2 (alive*0.2Ϊ0ʱ����Ϊ1����ͬ)
//��Э�̿��й���ʱ     alive -= alive*0.2 
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
    //Э�̳���ѭ��
    //���� mainArgs ����Ҫ����void*�Ĵ��ι���ֻ�ܴ���һ���ṹ��������Ų����ٴ���
    static void worker(void*);
    
    //<cid , busy_flag>
    //UINT cid : Э��id
    //bool busy_flag : ��Э���Ƿ�����ִ������
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