#include "WebServer.h"

using namespace std;

int WebServer::sig_pipefd[2] = { 0 };

void WebServer::add_sig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, nullptr) != -1);
}

// ��̬��Ա�޷����ʷǾ�̬��Ա
void WebServer::sig_handler(int sig)
{
    //��֤�����Ŀ������ԣ�����ԭ����errno
    int save_errno = errno;
    int msg = sig;
    send(sig_pipefd[1], (char*)&msg, 1, 0);
    errno = save_errno;
}


WebServer::WebServer(int port, int trigMode, int timeout, bool OptLinger, int threadNum, 
                     const string& user, const string& passwd, const string& dbname, 
                     int sql_port, int max_conn) :
    user(user), password(passwd), database(dbname), sql_port(sql_port), max_sql_num(max_conn),
    port_(port), openLinger_(OptLinger), timeout_(timeout), isClose_(false),
    timer_(new HeapTimer()), threadpool_(new ThreadPool(threadNum)), epoller_(new Epoller())
{
    srcdir_ = getcwd(nullptr, 256);
    assert(srcdir_);
    strncat(srcdir_, "/resources", 16);
    printf("current workplace %s\n", srcdir_);
    HttpConn::user_count = 0;
    HttpConn::srcdir = srcdir_;
    this->conn_pool = ConnectionPool::get_instance();
    conn_pool->init("localhost", user, password, database, sql_port, max_sql_num);

    init_event_mode_(trigMode);
    if (!init_socket_())    isClose_ = true;
}


/* Create listenFd */
bool WebServer::init_socket_() {
    int ret;
    struct sockaddr_in addr;
    if (port_ > 65535 || port_ < 1024) {
        printf("Port:%d error!\n", port_);
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);
    struct linger optLinger = { 0 };
    if (openLinger_) {
        /* ���Źر�: ֱ����ʣ���ݷ�����ϻ�ʱ */
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }

    listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd_ < 0) {
        printf("Create %d socket error!\n", port_);
        return false;
    }

    ret = setsockopt(listenfd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if (ret < 0) {
        close(listenfd_);
        printf("Init linger %d error!\n", port_);
        return false;
    }

    int optval = 1;
    /* �˿ڸ��� */
    /* ֻ�����һ���׽��ֻ������������ݡ� */
    ret = setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if (ret == -1) {
        printf("set socket setsockopt error !\n");
        close(listenfd_);
        return false;
    }

    ret = bind(listenfd_, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0) {
        printf("Bind Port:%d error!\n", port_);
        close(listenfd_);
        return false;
    }

    ret = listen(listenfd_, 6);
    if (ret < 0) {
        printf("Listen port:%d error!\n", port_);
        close(listenfd_);
        return false;
    }

    ret = epoller_->add_fd(listenfd_, listen_event_ | EPOLLIN);
    if (ret == 0) {
        printf("Add listen error!\n");
        close(listenfd_);
        return false;
    }
    // ���������źŹܵ�
    ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sig_pipefd);
    if (ret < 0) {
        printf("Sig_pipefd error\n");
        close(listenfd_);
        return false;
    }

    ret = epoller_->add_fd(sig_pipefd[0], connect_event_ | EPOLLIN);
    if (ret == 0) {
        printf("Add sig_pipe error!\n");
        close(listenfd_);
        return false;
    }
    // ����رյĹܵ��˷������ݵ��µĴ�����к��Դ���
    add_sig(SIGPIPE, SIG_IGN, false);
    add_sig(SIGTERM, sig_handler, false);

    setfd_nonblock(sig_pipefd[1]);
    setfd_nonblock(listenfd_);
    printf("Server port:%d\n", port_);
    return true;
}

WebServer::~WebServer() {
    close(listenfd_);
    isClose_ = true;
    free(srcdir_);
    conn_pool->destroy_pool();
}

void WebServer::init_event_mode_(int trigMode) 
{
    listen_event_ = EPOLLRDHUP;
    connect_event_ = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMode)
    {
    case 0:
        break;
    case 1:
        connect_event_ |= EPOLLET;
        break;
    case 2:
        listen_event_ |= EPOLLET;
        break;
    case 3:
        listen_event_ |= EPOLLET;
        connect_event_ |= EPOLLET;
        break;
    default:
        listen_event_ |= EPOLLET;
        connect_event_ |= EPOLLET;
        break;
    }
    HttpConn::is_ET = (connect_event_ & EPOLLET);
}

// ��ʼ�����ݿ����ӳ�
void WebServer::init_sqlpool() {
    conn_pool = ConnectionPool::get_instance();     // ������ݿ����ӳص�Ψһʵ��
    conn_pool->init("localhost", user, password, database, 3306, max_sql_num);

    // �����ӳ���ȡ��һ������
    MYSQL* mysql = NULL;
    ConnectionRAII mysqlcon(&mysql, conn_pool);

    // ���� user ���е� username��passwd ����
    if (mysql_query(mysql, "SELECT username,passwd FROM user"))
    {
        printf("SELECT error:%s\n", mysql_error(mysql));
    }

    // �ӱ��м��������Ľ����
    MYSQL_RES* result = mysql_store_result(mysql);


    // �����Ӧ���û���������
    while (MYSQL_ROW row = mysql_fetch_row(result))
    {
        string user(row[0]);
        string passwd(row[1]);
        users[user] = passwd;
    }
}


void WebServer::start() 
{
    ssize_t timeMS = -1;    /* epoll wait timeout == -1 ���¼������� */
    while (!isClose_) {
        if (timeout_ > 0) {
            timeMS = timer_->get_next_tick();
        }
        // ����´��¼����ʱ�䲢�����ȴ�
        int eventCnt = epoller_->wait(timeMS);
        for (int i = 0; i < eventCnt; i++) 
        {
            /* �����¼� */
            int fd = epoller_->get_event_fd(i);
            uint32_t events = epoller_->get_events(i);
            // �ͻ��˽����¼�
            if (fd == listenfd_) 
            {
                deal_listen_();
            }
            else if (fd == sig_pipefd[0])
            {
                deal_sig();
            }
            // �ر��¼�
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) 
            {
                assert(users_.count(fd) > 0);
                close_conn_(&users_[fd]);
            }
            // ���¼�
            else if (events & EPOLLIN) 
            {
                assert(users_.count(fd) > 0);
                deal_read_(&users_[fd]);
            }
            // д�¼�
            else if (events & EPOLLOUT) 
            {
                assert(users_.count(fd) > 0);
                deal_write_(&users_[fd]);
            }
            // �����¼�
            else
            {
                printf("Unexpected event\n");
            }
        }
    }
}

void WebServer::send_error(int fd, const char* info)
{
    assert(fd > 0);
    size_t ret = send(fd, info, strlen(info), 0);
    if (ret < 0)
    {
        printf("send info error in WebServer\n");
    }
    close(fd);
}

void WebServer::close_conn_(HttpConn* client) 
{
    assert(client);
    printf("Client[%d] quit!\n", client->get_fd());
    epoller_->del_fd(client->get_fd());
    client->shutdown();
}

void WebServer::add_client_(int fd, sockaddr_in addr) 
{
    assert(fd > 0);
    users_[fd].init(fd, addr);
    if (timeout_ > 0) 
    {
        timer_->add(fd, timeout_, std::bind(&WebServer::close_conn_, this, &users_[fd]));
    }
    epoller_->add_fd(fd, EPOLLIN | connect_event_);
    setfd_nonblock(fd);
    printf("Client[%d] in!\n", users_[fd].get_fd());
}

void WebServer::deal_listen_() 
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do 
    {
        int fd = accept(listenfd_, (struct sockaddr*)&addr, &len);
        if (fd <= 0) { return; }
        else if (HttpConn::user_count >= MAX_FD) 
        {
            send_error(fd, "Server Busy");
            printf("Clients is full!\n");
            return;
        }
        add_client_(fd, addr);
    } while (listen_event_ & EPOLLET);
}

void WebServer::deal_sig()
{
    ssize_t ret = -1;
    //int sig;      ��������¼�źţ���δ��ʹ��
    char signals[1024];
    ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
    if (ret <= 0)   return;
    else {
        for (int i = 0; i < ret; ++i) {
            switch (signals[i]) {
            case SIGTERM: {
                isClose_ = true;
                break;
                }
            }
        }
    }
}

void WebServer::deal_read_(HttpConn* client) 
{
    assert(client);
    extent_time_(client);
    threadpool_->append(std::bind(&WebServer::on_read_, this, client));
}

void WebServer::deal_write_(HttpConn* client) 
{
    assert(client);
    extent_time_(client);
    threadpool_->append(std::bind(&WebServer::on_write_, this, client));
}


void WebServer::extent_time_(HttpConn* client) 
{
    assert(client);
    if (timeout_ > 0)    
    { 
        timer_->adjust(client->get_fd(), timeout_); 
    }
}

void WebServer::on_read_(HttpConn* client) {
    assert(client);
    ssize_t ret = -1;
    int read_errno = 0;
    ret = client->read_fd(&read_errno);
    if (ret <= 0 && read_errno != EAGAIN) 
    {
        close_conn_(client);
        return;
    }
    // �ų��쳣�����������
    process(client);
}

void WebServer::process(HttpConn* client) 
{
    // ���ճɹ����ȴ�д
    // parse -> parse_request_line -> parse_header -> parse_body -> make_response
    if (client->parse_request())      
    {
        epoller_->mod_fd(client->get_fd(), connect_event_ | EPOLLOUT);
    }
    // ���ܲ��ɹ����ȴ���
    else 
    {
        epoller_->mod_fd(client->get_fd(), connect_event_ | EPOLLIN);
    }
}

void WebServer::on_write_(HttpConn* client) 
{
    assert(client);
    ssize_t ret = -1;
    int write_errno = 0;
    ret = client->write_fd(&write_errno);
    if (client->to_write_bytes() == 0) 
    {
        /* ������� */
        if (client->is_keep_alive()) 
        {
            process(client);
            return;
        }
    }
    else if (ret < 0) 
    {
        // һֱд��ֱ������ EAGAIN ����
        if (write_errno == EAGAIN) 
        {
            /* �������� */
            epoller_->mod_fd(client->get_fd(), connect_event_ | EPOLLOUT);
            return;
        }
    }
    close_conn_(client);
}

int WebServer::setfd_nonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}


