#include "coPool.h"

using namespace crt;
using namespace std;

CoroutinePool* cp;

void testFunc_1(void* args) {
    for (int i = 0; i < 12; i++) {
        cout << "" << args << endl;
        cp->sleep(500);
    }
}
void testFunc_2(void* args) {
    for (;;) {
        cout << "   " << args << endl;
        cp->sleep(1000);
    }
}
void testFunc_3(void* args) {
    for (;;) {
        cout << "      " << args << endl;
        cp->sleep(3000);
    }
}

void testPool_1() {
    CoTask tasks[6];
    cp = new CoroutinePool(2, 3);
    tasks[1].func = testFunc_1;
    tasks[1].args = reinterpret_cast<void*>(1);
    tasks[2].func = testFunc_2;
    tasks[2].args = reinterpret_cast<void*>(2);
    tasks[3].func = testFunc_3;
    tasks[3].args = reinterpret_cast<void*>(3);
    cp->addTask(&tasks[1]);
    cp->addTask(&tasks[2]);
    cp->addTask(&tasks[3]);
    for (int i = 0; i < 10; i++) {
        cout << "--------------" << endl;
        cp->scheduling();
        usleep(500 * 1000);
    }
    
    //结束testFunc_2
    cp->terminate(tasks[2].cid);
    tasks[4].func = testFunc_1;
    tasks[4].args = reinterpret_cast<void*>(4);
    tasks[5].func = testFunc_2;
    tasks[5].args = reinterpret_cast<void*>(5);
    cp->addTask(&tasks[4]);
    cp->addTask(&tasks[5]); //1的12次没完了，所以还没轮到5
    cout << "--------------2---------------" << endl;
    for (;;) {
        cp->scheduling();
        usleep(100 * 1000);
    }
    delete cp;
}

//.vs/coroutine/out/build/linux-debug/coroutine
//int main() {
//
//    testPool_1();
//}