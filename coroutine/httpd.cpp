#include "httpd.h"

using namespace std;

int MyHpptd::sys_error(const char* str) {
    perror(str);
    exit(-1);
}

/*************************************
 * sock: ��������������ȡ����
 * buffer: ����ȡ�����ݴ���buffer��
 * size: ��ȡ���ݵ���󳤶ȣ���һ�г���size����sizeΪ���ֵ�ض�
 *************************************/
int MyHpptd::get_line(int sock, char* buffer, int size) {
    int cur = 0; //��ǰ���±�
    char ch_end = '\0';
    int count;
    while ((cur < size - 1) && ch_end != '\n') {
        count = recv(sock, &ch_end, 1, 0);
        if (count > 0) {
            if (ch_end == '\r') { //�� �س� ����
                count = recv(sock, &ch_end, 1, MSG_PEEK);
                if ((count > 0) && (ch_end == '\n')) { // \r\n����
                    recv(sock, &ch_end, 1, 0); // ����\n
                }
                else { //�ֶ�����
                    ch_end = '\n';
                }
            }
            buffer[cur] = ch_end;
            cur++;
        }
        else {
            ch_end = '\n';
        }
    }
    buffer[cur] = '\0';
    return cur;
}


void MyHpptd::disconnect(int fd_client) {
    int status = epoll_ctl(fd_epoll, EPOLL_CTL_DEL, fd_client, NULL);
    if (status == -1) {
        sys_error("epoll_ctr delete client fd error:");
    }
    close(fd_client);
    cout << "client disconnected" << endl;
}

void MyHpptd::http_request(const char* request, int fd_client) {
    char method[12], path[1024], protocol[12];
    //sscanf(request, "%[^ ] %[^ ] %[^ ]", method, path, protocol);
    int offset = 0;
    offset += getWord(request         , method)+1;
    offset += getWord(request + offset, path)+1;
    offset += getWord(request + offset, protocol);
    printf("method = %s, path = %s, protocol = %s\n", method, path, protocol);
    char* file = path + 1;//ȥ��path����ǰ��� / ��÷��ʵ��ļ���
    if (strcmp(path, "/") == 0) { //�û�û��ָ��Ҫ���ʵ���Դ��Ĭ�ϵ�ǰλ��
        file[0] = '.';
        file[1] = '/';
        file[2] = '\0';
    }

    struct stat st;
    int status = stat(file, &st);
    if (status == -1) {
        send_error(fd_client, 404, "Not Found", "NO suchh file or directory");
        return;
    }

    if (S_ISDIR(st.st_mode)) {
        send_respond_head(fd_client, 200, "OK", get_file_type(".html"), -1);
        send_dir(fd_client, file);
    }
    else if (S_ISREG(st.st_mode)) {
        send_respond_head(fd_client, 200, "OK", get_file_type(file), -1);
        send_file(fd_client, file);
    }
}

void MyHpptd::send_respond_head(int fd_clinet, int status, const char* title, const char* type, long len) {
    // ʹ��Э�̺�sprintf�������ض�λ��������⣬�ض�λ�����е��ѽ��
    // sprintfʹ�������壬call������ʱ��ת���ǹ̶�ƫ��
    // ���������Լ������˶�ջ�����Թ̶���ת������ֵĵط����ᷢ���δ���
    // ����İ취Ҳ�ܼ򵥣�ֱ���ҵ��������壬Ȼ���ú���ָ����ü���
    // ������ÿ�α����ʱ����������ĵ�ַ�����̶�������ֻ�ܶ�̬����
    // ������api������Ҳ����ʹ������������
    // ����û�����Ҫ����ΪsscanfҲ���ض�λ��������⣬��������Ҳ����������
    // Ѱ�Һ��ʵķ������ԭ���ĺ������ɣ��Ͼ������и��õİ취���(ostringstream)
    //char buffer[1024];
    //memset(buffer, 0, sizeof(buffer));
    //sprintf(buffer, "http/1.1 %d %s\r\n"
    //                "Content-Type:%s\r\n"
    //                "Content-Length:%ld\r\n"
    //                "\r\n", status, title, type, len);
    ostringstream buf;
    buf << "http/1.1 " << status << " " << title  << "\r\n"
        << "Content-Type:" << type                << "\r\n"
        << "Content-Length:" << len               << "\r\n" 
        << "\r\n";
    string str = buf.str();
    send(fd_clinet, str.c_str(), str.size(), 0);
}


void MyHpptd::send_error(int fd_clinet, int status, const char* title, const char* text) {
    //char buffer[4096] = { 0 };
    //sprintf(buffer,
    //    "%s %d %s\r\n"
    //    "Content-Type:%s\r\n"
    //    "Content-Length:%d\r\n"
    //    "Connection: close\r\n"
    //    "\r\n", 
    //    "HTTP/1.1", status, title, "text/html", -1);
    //send(fd_clinet, buffer, strlen(buffer), 0);
    //memset(buffer, 0, sizeof(buffer));

    //sprintf(buffer, "<html><head><title>%d %s</title></head>\n", status, title);
    //sprintf(buffer + strlen(buffer), "<body bgcolor=\"#cc99cc\"><h2 align=\"center\">%d %s</h4>\n", status, title);
    //sprintf(buffer + strlen(buffer), "%s\n", text);
    //sprintf(buffer + strlen(buffer), "<hr>\n</body>\n</html>\n");
    //send(fd_clinet, buffer, strlen(buffer), 0);

    ostringstream buf;
    buf << "HTTP/1.1 " << status << " " << title << "\r\n"
        << "Content-Type:text/html" << "\r\n"
        << "Content-Length:-1" << "\r\n"
        << "Connection: close" << "\r\n"
        << "\r\n";
    
    buf << "<html><head><title>" << status << title << "</title></head>\n"
        << "<body bgcolor=\"#cc99cc\"><h2 align=\"center\">" << status << title <<"</h4>\n"
        << text << "\n"
        << "<hr>\n</body>\n</html>\n";
    string str = buf.str();
    send(fd_clinet, str.c_str(), str.size(), 0);

    return;
}

void MyHpptd::send_dir(int fd_clinet, const char* dir_name) {
    //char buffer[4096];
    //sprintf(buffer,
    //    "<html><head><title>Ŀ¼��: %s</title></head>"
    //    "<body><h1>��ǰĿ¼: %s</h1><table>", dir_name, dir_name);
    ostringstream buf;
    string str;
    buf << "<html><head><title>directory:" << dir_name << "</title></head>\n"
        << "<body><h1>current directory: " << dir_name << " </h1><table>\n";
    str = buf.str();
    int status = send(fd_clinet, str.c_str(), str.size(), 0);
    buf.str("");
    str.clear();
    if (status == -1) {
        sys_error("send error(329):");
    }
    char encstr[1024] = { 0 };
    //char path[1024] = { 0 }; //��ŵ�ǰ���������ļ���ַ(�������ļ���Ҳ������Ŀ¼)
    string path;
    dirent** pp_dir;
    
    int file_num = scandir(dir_name, &pp_dir, NULL, alphasort);
    for (int i = 0; i < file_num; i++) {
        char* name = pp_dir[i]->d_name;
        //sprintf(path, "%s%s", dir_name, name);
        path = dir_name + string(name);
        struct stat st;
        stat(path.c_str(), &st);
        //��һЩ�������ת��� %xx ������html����ʱ����
        encode_str(name, encstr, sizeof(encstr));
        if (S_ISREG(st.st_mode)) {//���ļ�
            //sprintf(buffer + strlen(buffer),
            //    "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
            //    encstr, name, (long)st.st_size);
            buf << "<tr><td><a href=\"" << encstr << "\">" << name << "</a></td>"
                << "<td>" << st.st_size << "</td></tr>";
        }
        else if (S_ISDIR(st.st_mode)) {
            //sprintf(buffer + strlen(buffer),
            //    "<tr><td><a href=\"%s/\">%s/</a></td><td>%ld</td></tr>",
            //    encstr, name, (long)st.st_size);
            buf << "<tr><td><a href=\"" << encstr << "/\">" << name << "</a></td>"
                << "<td>" << st.st_size << "</td></tr>";
        }
        str = buf.str();
        status = send(fd_clinet, str.c_str(), str.size(), 0);
        if (status == -1) {
            if (errno == EAGAIN) {
                perror("send error:");
                continue;
            }
            else if (errno == EINTR) {
                perror("send error:");
                continue;
            }
            else {
                perror("send error:");
                exit(1);
            }
        }
        //memset(buffer, 0, sizeof(buffer));
        buf.str("");
        str.clear();
    }
    free(pp_dir); //�����������˿ռ䣬����ͷŵ�
    //sprintf(buffer, "</table></body></html>");
    buf << "</table></body></html>";
    str = buf.str();

    if (send(fd_clinet, str.c_str(), str.size(), 0) == -1) {
        sys_error("send error(370):");
    }
    printf("dir message send OK!!!!\n");
}

void MyHpptd::send_file(int fd_clinet, const char* file_name) {
    int fd = open(file_name, O_RDONLY);
    if (fd == -1) { //���ļ�ʧ��
        send_error(fd_clinet, 404, "Not Found", "NO such file or direntry");
        return;
    }

    char buffer[4096];
    int read_count;
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        read_count = read(fd, buffer, sizeof(buffer));
        if (read_count <= 0) break;//���������ݣ��˳�
        int status = send(fd_clinet, buffer, read_count, 0);
        if (status == -1) { // ���ͳ���
            if (errno == EAGAIN) {
                perror("send error:");
                continue;
            }
            else if (errno == EINTR) {
                perror("send error:");
                continue;
            }
            else {
                perror("send error:");
                exit(1);
            }
        }

    }
    if (read_count == -1) { //���ļ�����
        perror("read file error");
        exit(1);
    }
    //�ر��ļ�������
    close(fd);
}

const char* MyHpptd::get_file_type(const char* name) {
    // ����������ҡ�.���ַ�, �粻���ڷ���NULL
    const char* dot = strrchr(name, '.');
    if (dot == NULL)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp(dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}

size_t MyHpptd::getWord(const char* src, char* dst) {
    int i = 0;
    while (src[i] != ' ' && src[i] != '\n') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
    return i;
}

int MyHpptd::hexit(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}

void MyHpptd::encode_str(const char* src, char* dst, int size) {
    int cur;

    for (cur = 0; *src != '\0' && cur + 4 < size; ++src) {
        if (isalnum(*src) || strchr("/_.-~", *src) != (char*)0) {
            *dst = *src;
            ++dst;
            ++cur;
        }
        else {
            sprintf(dst, "%%%02x", (int)*src & 0xff);
            dst += 3;
            cur += 3;
        }
    }
    *dst = '\0';
}

MyHpptd::MyHpptd(const char* path_workspace /*= ""*/, int port /*= 5000*/, int max_size /*= 1024*/) {
    //��������Ŀ¼
    struct stat st;
    stat(path_workspace, &st);
    
    if (path_workspace != "") {
        if (chdir(path_workspace) == -1) {
            cout << S_ISDIR(st.st_mode) << getcwd(NULL, NULL) << endl;
            sys_error("chdir error");
        }
    }

    this->port = port;
    this->max_size = max_size;
    ev_ready = new epoll_event[max_size];

    //����epoll��
    fd_epoll = epoll_create(max_size);
    if (fd_epoll == -1) {
        sys_error("epoll_create error:");
    }
    
    int status = 0;
    fd_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_listen == -1) {
        sys_error("listen socket() error");
    }
    sockaddr_in addr_server;
    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(port);
    addr_server.sin_addr.s_addr = INADDR_ANY;

    //�˿ڸ���
    int flag = 1;
    setsockopt(fd_listen, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    //��
    status = bind(fd_listen, (sockaddr*)&addr_server, sizeof(addr_server));

    if (status == -1) {
        sys_error("listen bind() error");
    }

    //��������
    status = listen(fd_listen, 64);

    //��listen_fd��ӵ� epoll ����
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd_listen;
    status = epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd_listen, &ev);
    if (status == -1) {
        sys_error("init_listen_fd epoll_ctl ADD error:");
    }
    cp = new crt::CoroutinePool(4, 8);
}

MyHpptd::~MyHpptd() {
    if (ev_ready) {
        delete[] ev_ready;
        ev_ready = nullptr;
    }
    if (cp) {
        delete cp;
        cp = nullptr;
    }
}

void MyHpptd::do_accept() {
    
    sockaddr_in addr_clinet;
    socklen_t len_clinet = sizeof(addr_clinet);

    int fd_clinet = accept(fd_listen, (sockaddr*)&addr_clinet, &len_clinet);
    if (fd_clinet == -1) {
        sys_error("do accept error: ");
    }

    //��ӡ���ӵĿͻ�����Ϣ
    /*char ch_ip[64];
    memset(ch_ip, 0, sizeof(ch_ip));*/
    //inet_ntop(AF_INET, &addr_clinet.sin_addr.s_addr, ch_ip, sizeof(ch_ip));
    //printf("New Client: %s : %d,  client_fd = %d\n",
    //    ch_ip,
    //    ntohs(addr_clinet.sin_port),
    //    fd_clinet);

    //���� client_fd Ϊ������
    //F_GETFL: �ļ�״̬��־
    //F_GETFD: �ļ������ʱ�־
    int flag = fcntl(fd_clinet, F_GETFL); //��ȡ flag
    flag |= O_NONBLOCK; //���÷�����
    fcntl(fd_clinet, F_SETFL, flag); //д��

        //���ӵ��½ڵ� �ҵ� epoll����
    epoll_event ev;
    ev.data.fd = fd_clinet;
    ev.events = EPOLLIN | EPOLLET; //�� | ���ش���
    int status = epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd_clinet, &ev);
    if (status == -1) {
        perror("epoll_ctl add client fd error:");
    }
}

void MyHpptd::do_read(int fd_client) {
    cout << "===============before do read=================\n" << endl;
    char line[1024];
    memset(line, 0, sizeof(line));
    int len = get_line(fd_client, line, sizeof(line));

    if (len == 0) {
        
        //�ر�����
        //disconnect(fd_client);
    }
    else {
        cout << "================request head===============" << endl;
        //cout << "line data��" << line << endl;
        // ��������û����,��������
        char buffer[1024];
        do {
            //cout << buffer;
            memset(buffer, 0, sizeof(buffer));
            len = get_line(fd_client, buffer, sizeof(buffer));
        } while (buffer[0] != '\n' && len != -1);
        cout << "=================END================" << endl;
    }
    if (strncasecmp("get", line, 3) == 0) {
        cout << "send fd:" << fd_client << endl;
        http_request(line, fd_client);
        disconnect(fd_client);
    }
    cout << "=================after do read=========== ======\n" << endl;
}

void MyHpptd::run() {
    crt::CoTask add_accept;
    add_accept.func = function_cast<crt::_callback>(&MyHpptd::do_accept);
    add_accept.args = this;
    while (1) {
        int ready_num = epoll_wait(fd_epoll, ev_ready, max_size, 0);
        if (ready_num == -1) {
            sys_error("epoll_wait error: ");
        }

        //�����������Ľڵ�
        for (int i = 0; i < ready_num; i++) {
            epoll_event& cur_ev = ev_ready[i];
            //���Ƕ��¼���ֱ�Ӻ���
            if (!(cur_ev.events & EPOLLIN)) {
                continue;
            }
            //���������¼�
            if (cur_ev.data.fd == fd_listen) {
                cout << "connecting... " << endl;
                cp->addTask(&add_accept);
            }
            //���¼�
            else {
                
                crt::CoTask read_task;
                read_task.func = function_cast<void (*)(void*)>(&MyHpptd::do_read);
                read_task.args = this;
                read_task.args2 = reinterpret_cast<void*>(ev_ready->data.fd);
                cp->addTask(&read_task);
                
            }
        }
        cp->scheduling();
    }
}

int main(int args, void* argv[]) {
    
    const char* path = "/home/sillenge/newFile";
    
    if (args > 1) {
        path = static_cast<char*>(argv[1]);
    }
    MyHpptd mh(path);
    mh.run();
}