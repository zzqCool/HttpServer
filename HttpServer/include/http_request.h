#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <assert.h>
#include <errno.h>

#include "Sql_Conn.h"
#include "Buffer.h"

class HttpRequest
{
public:
	enum PARSE_STATE
	{
		REQUEST_LINE,
		HEADERS,
		BODY,
		FINISH
	};

	enum HTTPCODE
	{
		NO_REQUEST = 0,
		GET_REQUEST,
		BAD_REQUEST,
		NO_RESOURCE,
		FORBIDDENT_REQUEST,
		FILE_REQUEST,
		INTERNAL_ERROR,
		CLOSED_CONNECTION
	};

	HttpRequest() { init(); }
	~HttpRequest() = default;

	void init();
	bool parse(Buffer& str);

	std::string path() const;
	std::string& path();
	std::string method() const;
	std::string version() const;
	std::string get_post(const std::string& key) const;
	std::string get_post(const char* key) const;

	bool is_keep_alive() const;

private:
	bool parse_request_line(const std::string& line);
	void parse_header(const std::string& line);
	void parse_body(const std::string& line);

	void parse_path();
	void parse_post();
	void parse_encoded_url();

	static bool verify_user(const string& name, const string& passwd, bool islogin);

	PARSE_STATE state_;
	std::string method_, path_, version_, body_;
	std::unordered_map<std::string, std::string> header_;
	std::unordered_map<std::string, std::string> post_;
		

	static const std::unordered_set<std::string> DEFAULT_HTML;
	static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
	static int convert_hex(char ch);
};