#pragma once

#include <unordered_map>
#include <fcntl.h>       // fcntl()
#include <unistd.h>      // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <arpa/inet.h>

#include "http_conn.h"
#include "threadpool.h"
#include "HeapTimer.h"
#include "Sql_Conn.h"
#include "epoller.h"

class WebServer {
public:
    WebServer(int port, int trigMode, int timeout, bool opt_Linger, int threadNum,
              const string& user, const string& passwd, const string& dbname,
              int sql_port, int max_conn);

    ~WebServer();
    void start();

private:
    bool init_socket_();
    void init_sqlpool();
    void init_event_mode_(int trigMode);
    void add_client_(int fd, sockaddr_in addr);
    void add_sig(int sig, void(handler)(int), bool restar);
    static void sig_handler(int sig);
 
    void deal_listen_();
    void deal_sig();
    void deal_write_(HttpConn* client);
    void deal_read_(HttpConn* client);

    void send_error(int fd, const char* info);
    void extent_time_(HttpConn* client);
    void close_conn_(HttpConn* client);

    void on_read_(HttpConn* client);
    void process(HttpConn* client);
    void on_write_(HttpConn* client);

    static const int MAX_FD = 65536;
    static int setfd_nonblock(int fd);
    static int sig_pipefd[2];

    // 数据库相关
    ConnectionPool* conn_pool;  // 连接池实例
    string user;                // 登陆数据库用户名
    string password;            // 登陆数据库密码
    string database;            // 使用数据库名
    int sql_port;               // 数据库连接端口
    int max_sql_num;            // 最大数据库连接数量
    unordered_map<string, string> users;    // 读取的用户数据和密码

    // 套接字连接选项
    int port_;
    bool openLinger_;
    int timeout_;       /* 毫秒MS */
    bool isClose_;
    int listenfd_;
    char* srcdir_;

    uint32_t listen_event_;
    uint32_t connect_event_;

    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;
};


