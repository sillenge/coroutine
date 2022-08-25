
#include "coroutine.h"
//---------------------------test 1------------------------------
using namespace std;
crt::CoroutineController* ccb;
int Cids[5];

typedef unsigned long long ULLONG;

void testFunc1(void* arg) {
    for (;;) {
        cout << "" << 1 << endl;
        ccb->sleep((ULLONG)arg);
    }
}
void testFunc2(void* arg) {
    for (;;) {
        cout << "  " << 2 << endl;
        ccb->sleep((ULLONG)arg);
    }
}
void testFunc3(void* arg) {
    for (;;) {
        cout << "    " << 3 << endl;
        ccb->sleep((ULLONG)arg);
    }
}
void testFunc4(void* arg) {
    for (;;) {
        cout << "      " << 4 << endl;
        ccb->sleep((ULLONG)arg);
    }
}

void test_1() {

    ccb = new crt::CoroutineController();
    Cids[1] = ccb->registerCoroutine(testFunc1, (void*)500);
    Cids[2] = ccb->registerCoroutine(testFunc2, (void*)2000);
    Cids[3] = ccb->registerCoroutine(testFunc3, (void*)1000);
    cout << "main 1" << endl;
    for (int i = 0; i < 1000; i++) {
        usleep(10 * 1000);
        ccb->scheduling();
    }
    ccb->terminate(Cids[1]);//主动结束testFunc1
    ccb->terminate(Cids[3]);//主动结束testFunc3
    Cids[4] = ccb->registerCoroutine(testFunc4, (void*)5000);
    cout << "main 2" << endl;
    for (int i = 0; i < 1000; i++) {
        usleep(10 * 1000);
        ccb->scheduling();
    }
    delete ccb;
}

//--------------------------tset 2------------------------------
void func_1(void* arg) {
    while (1) {
        cout << "" << 1 << endl;
        ccb->suspend();
        ccb->resume(Cids[2]);
    }
}
void func_2(void* arg) {
    while (1) {
        cout << "  " << 2 << endl;
        ccb->suspend();
        ccb->resume(Cids[3]);
    }
}
void func_3(void* arg) {
    while (1) {
        cout << "    " << 3 << endl;
        ccb->suspend();
        ccb->resume(Cids[1]);
    }
}

void test_2() {
    ccb = new crt::CoroutineController();
    Cids[1] = ccb->registerCoroutine(testFunc1, (void*)0);
    Cids[2] = ccb->registerCoroutine(testFunc2, (void*)0);
    Cids[3] = ccb->registerCoroutine(testFunc3, (void*)0);
    for (int i = 0; i < 1000; i++) {
        usleep(500 * 1000);
        ccb->scheduling();
    }
    delete ccb;
}

////不知道怎么写CMAKE测试，只能用土办法
//int main() {
//    test_1();
//    //test_2();
//}
