#include "Buffer.h"


// 初始化
Buffer::Buffer(int initBuffSize) : buffer_(initBuffSize), read_pos_(0), write_pos_(0) {}

// 获得可读字节数
size_t Buffer::readable_bytes() const {
    return write_pos_ - read_pos_;
}

// 获得可写字节数
size_t Buffer::writable_bytes() const {
    return buffer_.size() - write_pos_;
}

// 获得开始读位置
size_t Buffer::prependable_bytes() const {
    return read_pos_;
}

// 获得开始读位置的常量指针
const char* Buffer::peek() const {
    return begin_ptr_() + read_pos_;
}

// 向前移动已读位置
void Buffer::retrieve(size_t len) {
    assert(len <= readable_bytes());
    read_pos_ += len;
}

// 向前移动已读位置，vector 是连续存储
void Buffer::retrieve_until(const char* end) {
    assert(peek() <= end);
    retrieve(end - peek());
}

// 清空 buffer
void Buffer::retrieve_all() {
    // 直接设置为零
    bzero(&buffer_[0], buffer_.size());
    read_pos_ = 0;
    write_pos_ = 0;
}

// 将 Buffer 赋值给 string 后清空
std::string Buffer::retrieve_all_tostr() {
    std::string str(peek(), readable_bytes());
    retrieve_all();
    return str;
}

// 开始写位置，返回常量指针
const char* Buffer::begin_write_const() const {
    return begin_ptr_() + write_pos_;
}

// 开始写位置，返回普通指针
char* Buffer::begin_write() {
    return begin_ptr_() + write_pos_;
}

// 移动已写位置
void Buffer::has_written(size_t len) {
    write_pos_ += len;
}

// 向 Buffer 后添加 string
void Buffer::append(const std::string& str) {
    append(str.data(), str.length());
}

// 向 Buffer 后添加字符数组
void Buffer::append(const void* data, size_t len) {
    assert(data);
    append(static_cast<const char*>(data), len);
}

// 向 Buffer 后添加字符数组
void Buffer::append(const char* str, size_t len) {
    assert(str);
    ensure_writeable(len);
    std::copy(str, str + len, begin_write());
    has_written(len);
}

// 向 Buffer 后添加 Buffer
void Buffer::append(const Buffer& buff) {
    append(buff.peek(), buff.readable_bytes());
}

// 确保剩余空间可写
void Buffer::ensure_writeable(size_t len) {
    if (writable_bytes() < len) {
        make_space_(len);
    }
    assert(writable_bytes() >= len);
}

// 读取 socket 内容
ssize_t Buffer::read_fd(int fd, int* saveErrno) {
    char buff[65535];
    struct iovec iov[2];
    const size_t writable = writable_bytes();
    
    /* 分散读， 保证数据全部读完 */
    iov[0].iov_base = begin_ptr_() + write_pos_;
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if (len < 0) {
        *saveErrno = errno;
    }
    else if (static_cast<size_t>(len) <= writable) {
        write_pos_ += len;
    }
    else {
        write_pos_ = buffer_.size();        // 将多出的内容暂存至字符串中
        append(buff, len - writable);       // 扩容后转移至 Buffer
    }
    return len;
}

// 向 socket 写数据
ssize_t Buffer::write_fd(int fd, int* saveErrno) {
    size_t readable = readable_bytes();
    ssize_t len = write(fd, peek(), readable);
    if (len < 0) {
        *saveErrno = errno;
        return len;
    }
    read_pos_ += len;
    return len;
}

// 返回头指针
char* Buffer::begin_ptr_() {
    return &*buffer_.begin();
}

// 返回常头指针
const char* Buffer::begin_ptr_() const {
    return &*buffer_.begin();
}

// 扩容，保证剩余空间可写
void Buffer::make_space_(size_t len) {
    // 可写字节数 + 已读字节数 < len，需要扩容
    if (writable_bytes() + prependable_bytes() < len) 
    {
        buffer_.resize(write_pos_ + len + 1);
    }
    // 可写字节数 + 已读字节数 >= len，将已读数据利用未读数据覆盖，并移动已读位置和已写位置
    else 
    {
        size_t readable = readable_bytes();
        std::copy(begin_ptr_() + read_pos_, begin_ptr_() + write_pos_, begin_ptr_());
        read_pos_ = 0;
        write_pos_ = readable;
        assert(readable == readable_bytes());
    }
}