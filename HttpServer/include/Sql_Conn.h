#pragma once

// libmysqlclient.a	
// mysql.h
// locate : ���ڲ��ҷ����������ĵ���������������Ĳ��ң������ڱ����ĵ���Ŀ¼���Ƶ����ݿ��в���
// һ���ļ����ݿ��� /var/lib/slocate/slocate.db �У����� locate �Ĳ��Ҳ�����ʵʱ�ģ��������ݿ�ĸ���Ϊ׼
// һ����ϵͳ�Լ�ά����Ҳ�����ֹ��������ݿ� ������Ϊ��updatedb
// find : ��ָ��Ŀ¼�²����ļ����κ�λ�ڲ���֮ǰ���ַ�����������Ϊ�����ҵ�Ŀ¼��
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

// ���ݿ����ӳأ��ж���ѳ�ʼ���ı������ݿ�����
class ConnectionPool {
public:
	MYSQL* get_connection();					// ��ȡ���ݿ�����
	bool release_conncetion(MYSQL* conn);		// �ͷ�����
	int get_free_count();						// ��ȡ����������
	void destroy_pool();						// ������������

	// ����ģʽ
	static ConnectionPool* get_instance();

	void init(string _url, string _user, string _password, string _DataBaseName, int _port, int _max_conn);

private:
	ConnectionPool();
	~ConnectionPool();

	int max_conn;			// ���������
	int cur_conn;			// ��ǰ��ʹ�õ�������
	int free_conn;			// ��ǰ���е�������
	Lock sql_lock;
	list<MYSQL*> conn_list; //���ӳ�
	Sem reserve;

public:
	string url;				// ������ַ
	int port;				// ���ݿ�˿ں�
	string user;			// ��½���ݿ��û���
	string password;		// ��½���ݿ�����
	string DataBaseName;	// ʹ�����ݿ���
	int close_log;			// ��־����
};

// ��ֱ��ͨ�� MYSQL* �������ӣ�����ͨ�����װʵ�� MYSQL* ���Զ�����
class ConnectionRAII {
public:
	ConnectionRAII(MYSQL** sql, ConnectionPool* conn_pool);
	~ConnectionRAII();

private:
	MYSQL* conn_RAII;
	ConnectionPool* poolRAII;
};