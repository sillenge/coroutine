#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>

#include "coPool.h"

template<typename ResultFunction, typename Function>
ResultFunction function_cast(Function fun) {
    ResultFunction result = ResultFunction(*(reinterpret_cast<int*>(&fun)));
    return result;
}


class MyHpptd {
private:
    int max_size;   //epollÊ÷´óĞ¡
    int port;       //¶Ë¿Ú
    int fd_epoll;   //epollÃèÊö·û
    int fd_listen;  //¼àÌıÃèÊö·û
    epoll_event* ev_ready;//¾ÍĞ÷ÊÂ¼ş
    crt::CoroutinePool* cp;
protected:
    int sys_error(const char* str);
    int get_line(int sock, char* buffer, int size);
    void disconnect(int fd_client);
    void http_request(const char* request, int fd_client);
    void send_respond_head(int fd_clinet, int status, const char* title, const char* type, long len);
    void send_error(int fd_clinet, int status, const char* title, const char* text);
    void send_dir(int fd_clinet, const char* dir_name);
    void send_file(int fd_clinet, const char* file_name);
    const char* get_file_type(const char* name);
    size_t getWord(const char* src, char* dst);
    int hexit(char c);
    void encode_str(const char* src, char* dst, int size);
public:
    MyHpptd(const char* path_workspace = "", int port = 5000, int max_size = 1024);
    ~MyHpptd();
    void do_accept();
    void do_read(int fd_client);
    void run();
    
};