#pragma once

// libmysqlclient.a	
// mysql.h
// locate : 用于查找符合条件的文档，但并不是是真的查找，而是在保存文档和目录名称的数据库中查找
// 一般文件数据库在 /var/lib/slocate/slocate.db 中，所以 locate 的查找并不是实时的，而以数据库的更新为准
// 一般是系统自己维护，也可以手工升级数据库 ，命令为：updatedb
// find : 在指定目录下查找文件。任何位于参数之前的字符串都将被视为欲查找的目录名
#include <error.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <list>
#include <iostream>
#include <stdbool.h>
#include "locker.h"
#include <mysql/mysql.h>

using namespace std;

// 数据库连接池，有多个已初始化的本地数据库连接
class ConnectionPool {
public:
	MYSQL* get_connection();					// 获取数据库连接
	bool release_conncetion(MYSQL* conn);		// 释放连接
	int get_free_count();						// 获取空闲连接数
	void destroy_pool();						// 销毁所有连接

	// 单例模式
	static ConnectionPool* get_instance();

	void init(string _url, string _user, string _password, string _DataBaseName, int _port, int _max_conn);

private:
	ConnectionPool();
	~ConnectionPool();

	int max_conn;			// 最大连接数
	int cur_conn;			// 当前已使用的连接数
	int free_conn;			// 当前空闲的连接数
	Lock sql_lock;
	list<MYSQL*> conn_list; //连接池
	Sem reserve;

public:
	string url;				// 主机地址
	int port;				// 数据库端口号
	string user;			// 登陆数据库用户名
	string password;		// 登陆数据库密码
	string DataBaseName;	// 使用数据库名
	int close_log;			// 日志开关
};

// 不直接通过 MYSQL* 进行连接，而是通过类包装实现 MYSQL* 的自动析构
class ConnectionRAII {
public:
	ConnectionRAII(MYSQL** sql, ConnectionPool* conn_pool);
	~ConnectionRAII();

private:
	MYSQL* conn_RAII;
	ConnectionPool* poolRAII;
};