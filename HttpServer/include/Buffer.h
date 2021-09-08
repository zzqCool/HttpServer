#pragma once

#include <cstring>   //perror
#include <iostream>
#include <unistd.h>  // write
#include <sys/uio.h> //readv
#include <vector> //readv
#include <atomic>
#include <assert.h>

class Buffer
{
public:
    Buffer(int initBuffSize = 1024);
    ~Buffer() = default;

    size_t writable_bytes() const;
    size_t readable_bytes() const;
    size_t prependable_bytes() const;

    const char* peek() const;
    void ensure_writeable(size_t len);
    void has_written(size_t len);

    void retrieve(size_t len);
    void retrieve_until(const char* end);

    void retrieve_all();
    std::string retrieve_all_tostr();

    const char* begin_write_const() const;
    char* begin_write();

    void append(const std::string & str);
    void append(const char* str, size_t len);
    void append(const void* data, size_t len);
    void append(const Buffer & buff);

    ssize_t read_fd(int fd, int* Errno);
    ssize_t write_fd(int fd, int* Errno);

private:
    char* begin_ptr_();
    const char* begin_ptr_() const;
    void make_space_(size_t len);

    std::vector<char> buffer_;
    std::atomic<std::size_t> read_pos_;             // 已读位置
    std::atomic<std::size_t> write_pos_;            // 已写位置
};

