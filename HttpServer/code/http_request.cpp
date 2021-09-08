#include "http_request.h"
#include <iostream>
using namespace std;

const unordered_set<string> HttpRequest::DEFAULT_HTML{
             "/index", "/register", "/login",
             "/welcome", "/video", "/picture", };

const unordered_map<string, int> HttpRequest::DEFAULT_HTML_TAG{
            {"/register.html", 0}, {"/login.html", 1}, };


void HttpRequest::init()
{
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool HttpRequest::is_keep_alive() const
{
    if (header_.count("Connection") == 1)
    {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HttpRequest::parse(Buffer& buff)
{
    const char CRLF[] = "\r\n";
    if (buff.readable_bytes() <= 0)     return false;
    while (buff.readable_bytes() && state_ != FINISH)
    {
        const char* line_end = search(buff.peek(), buff.begin_write_const(), CRLF, CRLF + 2);
        string line(buff.peek(), line_end);
        switch (state_)
        {
        case REQUEST_LINE:
            if (!parse_request_line(line))
            {
                return false;
            }
            parse_path();
            break;
        case HEADERS:
            parse_header(line);
            // 可读字节数
            if (buff.readable_bytes() <= 2)
            {
                state_ = FINISH;
            }
            break;
        case BODY:
            parse_body(line);
            break;
        default:
            break;
        }
        if (line_end == buff.begin_write())     break;
        buff.retrieve_until(line_end + 2);      // 跳过 \r\n
    }
    return true;
}

void HttpRequest::parse_path() {
    if (path_ == "/") {
        path_ = "/index.html";
    }
    else if (DEFAULT_HTML.find(path_) != DEFAULT_HTML.end()) {
        path_ += ".html";
        /*for (auto& item : DEFAULT_HTML) {
            if (item == path_) {
                path_ += ".html";
                break;
            }
        }*/
    }
}

bool HttpRequest::parse_request_line(const string& line)
{
    regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch sub_match;
    if (regex_match(line, sub_match, pattern))
    {   
        method_ = sub_match[1];
        path_ = sub_match[2];
        version_ = sub_match[3];
        state_ = HEADERS;       // 开始解析头部
        return true;
    }
    return false;
}

void HttpRequest::parse_header(const string& line)
{
    regex patten("^([^:]*): ?(.*)$");
    smatch sub_match;
    if (regex_match(line, sub_match, patten)) {
        header_[sub_match[1]] = sub_match[2];
    }
    else {
        state_ = BODY;
    }
}

void HttpRequest::parse_body(const string& line) {
    body_ = line;
    parse_post();
    state_ = FINISH;
    printf("Body:%s, len:%d\n", line.c_str(), line.size());
}

int HttpRequest::convert_hex(char ch) {
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return ch;
}


void HttpRequest::parse_post() {
    if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        parse_encoded_url();
        // 属于登陆或注册界面
        if (DEFAULT_HTML_TAG.count(path_)) {
            // 获得页面标记
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            printf("Page:%s, Tag:%d\n", path_, tag);
            if (tag == 0 || tag == 1) {
                bool islogin = (tag == 1);
                // 用户身份验证成功
                if (verify_user(post_["username"], post_["password"], islogin)) {
                    path_ = "/welcome.html";
                }
                // 用户验证失败
                else {
                    path_ = "/error.html";
                }
            }
        }
    }
}
// url 符号说明：
//  ？ 分隔实际的 url 和参数
//  &  url 中指定的参数间的分隔符
//  =  url 中指定的参数的值
//  %  url 中的中文会被编码成 16 进制，用 % 进行分割
//  +  url 中表示空格
void HttpRequest::parse_encoded_url()
{
    if (body_.size() == 0)  return;
    
    string key, value;
    int num = 0;
    size_t n = body_.size();
    size_t i = 0, j = 0;

    for (; i < n; i++)
    {
        char ch = body_[i];
        switch (ch)
        {
        case '=':
            // '=' 之前为 key
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            // 将 '+' 转换为空格
            body_[i] = ' ';
            break;
        case '%':
            num = convert_hex(body_[i + 1]) * 16 + convert_hex(body_[i + 2]);
            body_[i + 2] = '0' + num % 10;
            body_[i + 1] = '0' + num / 10;
            i += 2;
            break;
        case '&':
            // '=' 之后为 value
            value = body_.substr(j, i - j);
            j = i + 1;
            post_[key] = value;
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    // 接受最后一个参数
    if (post_.count(key) == 0 && j < i)
    {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

bool HttpRequest::verify_user(const string& name, const string& passwd, bool isLogin) {
    if (name == "" || passwd == "") { return false; }
    printf("user name:%s passwd:%s\n", name.c_str(), passwd.c_str());
    MYSQL* sql = nullptr;
    ConnectionRAII(&sql, ConnectionPool::get_instance());
    assert(sql);

    bool flag = false;
    char order[256] = { 0 };
    MYSQL_RES* res = nullptr;

    if (!isLogin) { flag = true; }
    // 查询用户及密码 
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    printf("MYSQL order: %s\n", order);

    if (mysql_query(sql, order)) {
        mysql_free_result(res);
        return false;
    }
    res = mysql_store_result(sql);

    while (MYSQL_ROW row = mysql_fetch_row(res)) {
        printf("MYSQL ROW: %s %s\n", row[0], row[1]);
        string password(row[1]);
        // 登录行为验证密码
        if (isLogin) {
            if (passwd == password) { flag = true; }
            else {
                flag = false;
                printf("pwd error!");
            }
        }
        // 非登录行为但发现数据库中找到同样的用户名
        else {
            // 防止进入注册阶段，因为已有同样的用户
            flag = false;       
            printf("user has used!\n");
        }
    }
    mysql_free_result(res);

    // 注册行为且用户名未被使用
    if (!isLogin && flag == true) {
        printf("regirster!\n");
        bzero(order, 256);
        snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), passwd.c_str());
        printf("MYSQL order: %s\n", order);
        if (mysql_query(sql, order)) {
            printf("Insert error!\n");
            flag = false;
        }
        flag = true;
    }
    ConnectionPool::get_instance()->release_conncetion(sql);
    printf("Verify user success!!\n");
    return flag;
}


string HttpRequest::path() const
{
    return path_;
}

string& HttpRequest::path()
{
    return path_;
}

string HttpRequest::method() const {
    return method_;
}

string HttpRequest::version() const {
    return version_;
}

string HttpRequest::get_post(const std::string& key) const {
    assert(key != "");
    if (post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}

string HttpRequest::get_post(const char* key) const {
    assert(key != nullptr);
    if (post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}