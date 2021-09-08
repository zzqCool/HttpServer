#include "http_conn.h"
using namespace std;

const char* HttpConn::srcdir;
std::atomic<int> HttpConn::user_count;
bool HttpConn::is_ET = true;

HttpConn::HttpConn() : fd_(0), is_close_(false) {
	addr_ = { 0 };
}

HttpConn::~HttpConn()
{
	shutdown();
}

void HttpConn::init(int sockfd, const sockaddr_in& addr)
{
	assert(sockfd > 0);
	user_count++;
	addr_ = addr;
	fd_ = sockfd;
    readBuff_.retrieve_all();
    writeBuff_.retrieve_all();
	is_close_ = false;
}

void HttpConn::shutdown() {
	response_.unmapfile();
	if (is_close_ == false)
	{
		is_close_ = true;
		user_count--;
		close(fd_);
	}
}

int HttpConn::get_fd() const {
	return fd_;
};

struct sockaddr_in HttpConn::get_addr() const {
	return addr_;
}

const char* HttpConn::get_ip() const {
    return inet_ntoa(addr_.sin_addr);
}

int HttpConn::get_port() const {
    return addr_.sin_port;
}

ssize_t HttpConn::read_fd(int* save_errno) {
    ssize_t len = -1;
    readBuff_.retrieve_all();
    do {
        len = readBuff_.read_fd(fd_, save_errno);
        if (len <= 0)   break;
    } while (is_ET);
    return len;
}

ssize_t HttpConn::write_fd(int* save_errno) {
    ssize_t len = -1;
    do {
        // ���������Ƚ� iov[0] �ĳ��ȱ�Ϊ�㣬�ٽ� iov[1] �ĳ��ȱ�Ϊ��
        // �����鳤�ȶ�Ϊ����˳�ѭ��
        len = writev(fd_, iov_, iovCnt_);
        if (len <= 0) {
            *save_errno = errno;
            break;
        }
        if (iov_[0].iov_len + iov_[1].iov_len == 0) { break; }      // �������
        // ��һ�� iov[0] ��ȫ��д�룬���ڶ��� iov[1] �е����ݻ�δȫ��д��
        // ֮�� iov[0].iov_len ��Ϊ�㣬�����ƶ� iov[1] �е� iov_base ָ��
        else if (static_cast<size_t>(len) > iov_[0].iov_len) {
            iov_[1].iov_base = (uint8_t*)iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len = iov_[1].iov_len - (len - iov_[0].iov_len);
            if (iov_[0].iov_len) {          // ��� iov_[0] ������
                writeBuff_.retrieve_all();
                iov_[0].iov_len = 0;
            }
        }
        // iov[0] �е����ݻ�δȫ��д��
        else {
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            writeBuff_.retrieve(len);
        }
    } while (is_ET || to_write_bytes() > 10240);
    return len;
}

// ��������н���������������ƴ����Ӧ����
bool HttpConn::parse_request() 
{
    request_.init();
    if (readBuff_.readable_bytes() <= 0)
    {
        return false;
    }
    else if (request_.parse(readBuff_)) 
    {
        response_.init(srcdir, request_.path(), request_.is_keep_alive(), 200);
    }
    else 
    {
        response_.init(srcdir, request_.path(), false, 400);
    }
    writeBuff_.retrieve_all();
    response_.make_response(writeBuff_);
    /* ��Ӧͷ */
    iov_[0].iov_base = const_cast<char*>(writeBuff_.peek());
    iov_[0].iov_len = writeBuff_.readable_bytes();
    iovCnt_ = 1;

    /* �ļ� */
    if (response_.file_len() > 0 && response_.file()) {
        iov_[1].iov_base = response_.file();
        iov_[1].iov_len = response_.file_len();
        iovCnt_ = 2;
    }
    return true;
}