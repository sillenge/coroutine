#include "coPool.h"

using namespace crt;
using namespace std;

CoroutinePool* cp;

void testFunc1(CoArgs* arg) {
    for (int i = 0; i < 10; i++) {
        //´òÓ¡²ÎÊý
        cout << "" << arg->other << endl;
        cp->sleep(500);
    }
}
void testFunc2(CoArgs* arg) {
    for (;;) {
        cout << "  " << arg->other << endl;
        cp->sleep(1000);
    }
}
void testFunc3(CoArgs* arg) {
    for (;;) {
        cout << "    " << arg->other << endl;
        cp->sleep(3000);
    }
}

void testPool_1() {
    cout << "begin" << endl;
    int* i = new int;
    cout << i << endl;
    cp = new CoroutinePool(2, 4);
    CoTask tasks[6];
    memset(tasks, 0, sizeof(tasks));
    tasks[1].func = testFunc1;
    tasks[1].args.other = (void*)1;
    tasks[2].func = testFunc2;
    tasks[2].args.other = (void*)2;
    tasks[3].func = testFunc3;
    tasks[3].args.other = (void*)3;
    cp->addTask(tasks[1]);
    cp->addTask(tasks[2]);
    cp->addTask(tasks[3]);
    for (int i = 0; i < 50; i++) {
        cp->scheduling();
        usleep(100 * 1000);
    }
    tasks[4].func = testFunc1;
    tasks[4].args.other = (void*)4;
    tasks[5].func = testFunc1;
    tasks[5].args.other = (void*)5;
    cp->addTask(tasks[4]);
    cp->addTask(tasks[5]);
    while (1) {
        cp->scheduling();
        usleep(100 * 1000);
    }
    delete cp;
}

///.vs/coroutine/out/build/linux-debug/coroutine
int main() {

    testPool_1();
}