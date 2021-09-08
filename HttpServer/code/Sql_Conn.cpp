#include "Sql_Conn.h"

using namespace std;

ConnectionPool::ConnectionPool() {
	cur_conn = 0;
	free_conn = 0;
}

// 获得连接池实例
ConnectionPool* ConnectionPool::get_instance() {
	static ConnectionPool conn_pool;
	return &conn_pool;
}

// 初始化函数
void ConnectionPool::init(string _url, string _user, string _password, 
						string _DataBaseName, int _port, int max_conn) {
	this->url = _url;
	this->user = _user;
	this->password = _password;
	this->DataBaseName = _DataBaseName;
	this->port = _port;

	for (int i = 0; i < max_conn; i++) {
		MYSQL* conn = nullptr;
		conn = mysql_init(conn);
		if (!conn) {
			printf("MYSQL error\n");
			exit(1);
		}

		conn = mysql_real_connect(conn, url.c_str(), user.c_str(), password.c_str(), DataBaseName.c_str(), port, nullptr, 0);

		if (!conn) {
			printf("MYSQL error\n");
			exit(1);
		}

		conn_list.push_back(conn);
		free_conn++;
		reserve.post();
	}

	this->max_conn = free_conn;
}

// 有请求时，数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL* ConnectionPool::get_connection() {
	MYSQL* conn = nullptr;
	if (0 == conn_list.size())	return conn;

	reserve.wait();
	sql_lock.lock();

	conn = conn_list.front();
	conn_list.pop_front();

	free_conn--;
	cur_conn++;

	sql_lock.unlock();
	return conn;
}

// 释放当前使用的连接
bool ConnectionPool::release_conncetion(MYSQL* conn) {
	if (nullptr == conn) {
		return false;
	}

	sql_lock.lock();

	conn_list.push_back(conn);
	free_conn++;
	cur_conn--;

	sql_lock.unlock();

	reserve.post();
	return true;
}

// 销毁数据库连接池
void ConnectionPool::destroy_pool() {
	sql_lock.lock();
	if (conn_list.size() > 0) {
		list<MYSQL*>::iterator iter;
		for (iter = conn_list.begin(); iter != conn_list.end(); iter++) {
			MYSQL* conn = *iter;
			mysql_close(conn);
		}

		cur_conn = 0;
		free_conn = 0;
		conn_list.clear();
	}
	sql_lock.unlock();
}

// 当前空闲的连接数
int ConnectionPool::get_free_count() {
	return this->free_conn;
}

// 析构函数
ConnectionPool::~ConnectionPool() {
	destroy_pool();
}


ConnectionRAII::ConnectionRAII(MYSQL** sql, ConnectionPool* conn_pool) {
	*sql = conn_pool->get_connection();
	conn_RAII = *sql;
	poolRAII = conn_pool;
}

ConnectionRAII::~ConnectionRAII() {
	poolRAII->release_conncetion(conn_RAII);
}