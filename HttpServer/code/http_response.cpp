#include "http_response.h"
using namespace std;

const unordered_map<string, string> HttpResponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const unordered_map<int, string> HttpResponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const unordered_map<int, string> HttpResponse::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

HttpResponse::HttpResponse() : code_(-1), keep_alive_(false), path_(""), srcdir_(""),  mm_file_(nullptr)
{
    this->mm_file_stat_ = { 0 };
}

HttpResponse::~HttpResponse() {
    unmapfile();
}

void HttpResponse::init(const string& srcdir, string& path, bool keep_alive, int code)
{
    assert(srcdir != "");
    if (mm_file_)   unmapfile(); 
    this->code_ = code;
    this->keep_alive_ = keep_alive;
    this->path_ = path;
    this->srcdir_ = srcdir;
    this->mm_file_ = nullptr;
    this->mm_file_stat_ = { 0 };
}

void HttpResponse::make_response(Buffer& buff)
{
    // 判断请求的资源文件
    if (stat((srcdir_ + path_).data(), &mm_file_stat_) < 0 || S_ISDIR(mm_file_stat_.st_mode))
    {
        code_ = 404;
    }
    else if (!(mm_file_stat_.st_mode & S_IROTH))
    {
        code_ = 403;
    }
    else if (code_ == -1) {
        code_ = 200;
    }
    error_html();
    add_state_line(buff);
    add_header(buff);
    add_content(buff);
}

char* HttpResponse::file()
{
    return mm_file_;
}

size_t HttpResponse::file_len() const
{
    return mm_file_stat_.st_size;
}

// code 为 200 则不进入循环
void HttpResponse::error_html() 
{
    if (CODE_PATH.count(code_) == 1) 
    {
        path_ = CODE_PATH.find(code_)->second;
        stat((srcdir_ + path_).data(), &mm_file_stat_);
    }
}

void HttpResponse::add_state_line(Buffer& buff)
{
    string status;
    if (CODE_STATUS.count(code_) == 1)
    {
        status = CODE_STATUS.find(code_)->second;
    }
    // 若不是记录在内需要处理的请求，统统处理为 BAD_REQUEST
    else
    {
        code_ = 400;
        status = CODE_STATUS.find(code_)->second;
    }
    buff.append("HTTP/1.1 " + to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::add_header(Buffer& buff)
{
    buff.append("Connection: ");
    if (keep_alive_)
    {
        buff.append("keep_alive\r\n");
        buff.append("keep-alive: max=6, timeout=120\r\n");
    }
    else
    {
        buff.append("close\r\n");
    }
    buff.append("Content-type: " + get_file_type() + "\r\n");
}

void HttpResponse::add_content(Buffer& buff)
{
    int srcfd = open((srcdir_ + path_).data(), O_RDONLY);
    if (srcfd < 0) {
        error_content(buff, "File NotFound!");
        return;
    }

    /* 将文件映射到内存提高文件的访问速度
        MAP_PRIVATE 建立一个写入时拷贝的私有映射*/
    int* mm_ret = (int*)mmap(0, mm_file_stat_.st_size, PROT_READ, MAP_PRIVATE, srcfd, 0);
    if (*mm_ret == -1) {
        error_content(buff, "File NotFound!");
        return;
    }
    mm_file_ = (char*)mm_ret;
    close(srcfd);
    buff.append("Content-length: " + to_string(mm_file_stat_.st_size) + "\r\n\r\n");
}

void HttpResponse::unmapfile() {
    if (mm_file_) {
        munmap(mm_file_, mm_file_stat_.st_size);
        mm_file_ = nullptr;
    }
}

string HttpResponse::get_file_type() {
    /* 判断文件类型 */
    string::size_type idx = path_.find_last_of('.');
    if (idx == string::npos) {
        return "text/plain";
    }
    string suffix = path_.substr(idx);
    if (SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

void HttpResponse::error_content(Buffer& buff, string message)
{
    string body;
    string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if (CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    }
    else {
        status = "Bad Request";
    }
    body += to_string(code_) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
    buff.append(body);
}