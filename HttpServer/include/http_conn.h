#pragma once

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>
#include <atomic>
#include <string>

#include "Buffer.h"
#include "http_request.h"
#include "http_response.h"
#include "Sql_Conn.h"

class HttpConn {
public:
    HttpConn();
    ~HttpConn();

    void init(int sockfd, const sockaddr_in& addr);
    ssize_t read_fd(int* save_errno);
    ssize_t write_fd(int* save_errno);
    void shutdown();

    int get_fd() const;
    int get_port() const;
    const char* get_ip() const;
    sockaddr_in get_addr() const;

    bool parse_request();
    size_t to_write_bytes() {
        return iov_[0].iov_len + iov_[1].iov_len;
    }

    bool is_keep_alive() const {
        return request_.is_keep_alive();
    }

    static bool is_ET;
    static const char* srcdir;
    static std::atomic<int> user_count;

private:

    int fd_;
    struct sockaddr_in addr_;

    bool is_close_;

    int iovCnt_;
    struct iovec iov_[2];

    Buffer readBuff_;          // ¶Á»º³åÇø
    Buffer writeBuff_;         // Ð´»º³åÇø

    HttpRequest request_;
    HttpResponse response_;
};
