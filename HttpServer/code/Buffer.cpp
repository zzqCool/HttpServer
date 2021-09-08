#include "Buffer.h"


// ��ʼ��
Buffer::Buffer(int initBuffSize) : buffer_(initBuffSize), read_pos_(0), write_pos_(0) {}

// ��ÿɶ��ֽ���
size_t Buffer::readable_bytes() const {
    return write_pos_ - read_pos_;
}

// ��ÿ�д�ֽ���
size_t Buffer::writable_bytes() const {
    return buffer_.size() - write_pos_;
}

// ��ÿ�ʼ��λ��
size_t Buffer::prependable_bytes() const {
    return read_pos_;
}

// ��ÿ�ʼ��λ�õĳ���ָ��
const char* Buffer::peek() const {
    return begin_ptr_() + read_pos_;
}

// ��ǰ�ƶ��Ѷ�λ��
void Buffer::retrieve(size_t len) {
    assert(len <= readable_bytes());
    read_pos_ += len;
}

// ��ǰ�ƶ��Ѷ�λ�ã�vector �������洢
void Buffer::retrieve_until(const char* end) {
    assert(peek() <= end);
    retrieve(end - peek());
}

// ��� buffer
void Buffer::retrieve_all() {
    // ֱ������Ϊ��
    bzero(&buffer_[0], buffer_.size());
    read_pos_ = 0;
    write_pos_ = 0;
}

// �� Buffer ��ֵ�� string �����
std::string Buffer::retrieve_all_tostr() {
    std::string str(peek(), readable_bytes());
    retrieve_all();
    return str;
}

// ��ʼдλ�ã����س���ָ��
const char* Buffer::begin_write_const() const {
    return begin_ptr_() + write_pos_;
}

// ��ʼдλ�ã�������ָͨ��
char* Buffer::begin_write() {
    return begin_ptr_() + write_pos_;
}

// �ƶ���дλ��
void Buffer::has_written(size_t len) {
    write_pos_ += len;
}

// �� Buffer ����� string
void Buffer::append(const std::string& str) {
    append(str.data(), str.length());
}

// �� Buffer ������ַ�����
void Buffer::append(const void* data, size_t len) {
    assert(data);
    append(static_cast<const char*>(data), len);
}

// �� Buffer ������ַ�����
void Buffer::append(const char* str, size_t len) {
    assert(str);
    ensure_writeable(len);
    std::copy(str, str + len, begin_write());
    has_written(len);
}

// �� Buffer ����� Buffer
void Buffer::append(const Buffer& buff) {
    append(buff.peek(), buff.readable_bytes());
}

// ȷ��ʣ��ռ��д
void Buffer::ensure_writeable(size_t len) {
    if (writable_bytes() < len) {
        make_space_(len);
    }
    assert(writable_bytes() >= len);
}

// ��ȡ socket ����
ssize_t Buffer::read_fd(int fd, int* saveErrno) {
    char buff[65535];
    struct iovec iov[2];
    const size_t writable = writable_bytes();
    
    /* ��ɢ���� ��֤����ȫ������ */
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
        write_pos_ = buffer_.size();        // ������������ݴ����ַ�����
        append(buff, len - writable);       // ���ݺ�ת���� Buffer
    }
    return len;
}

// �� socket д����
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

// ����ͷָ��
char* Buffer::begin_ptr_() {
    return &*buffer_.begin();
}

// ���س�ͷָ��
const char* Buffer::begin_ptr_() const {
    return &*buffer_.begin();
}

// ���ݣ���֤ʣ��ռ��д
void Buffer::make_space_(size_t len) {
    // ��д�ֽ��� + �Ѷ��ֽ��� < len����Ҫ����
    if (writable_bytes() + prependable_bytes() < len) 
    {
        buffer_.resize(write_pos_ + len + 1);
    }
    // ��д�ֽ��� + �Ѷ��ֽ��� >= len�����Ѷ���������δ�����ݸ��ǣ����ƶ��Ѷ�λ�ú���дλ��
    else 
    {
        size_t readable = readable_bytes();
        std::copy(begin_ptr_() + read_pos_, begin_ptr_() + write_pos_, begin_ptr_());
        read_pos_ = 0;
        write_pos_ = readable;
        assert(readable == readable_bytes());
    }
}