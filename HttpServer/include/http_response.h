#pragma once

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "Buffer.h"

class HttpResponse
{
public:
	HttpResponse();
	~HttpResponse();

	void init(const std::string& srcdir, std::string& path, bool keep_alive = false, int code = -1);
	void make_response(Buffer& buf);
	void unmapfile();
	char* file();
	size_t file_len() const;
	void error_content(Buffer& buf, std::string message);
	int get_code() const { return code_; }

private:
	void add_state_line(Buffer& line);
	void add_header(Buffer& head);
	void add_content(Buffer& content);

	void error_html();
	std::string get_file_type();

	int code_;
	bool keep_alive_;

	std::string path_;
	std::string srcdir_;

	char* mm_file_;
	struct stat mm_file_stat_;

	static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
	static const std::unordered_map<int, std::string> CODE_STATUS;
	static const std::unordered_map<int, std::string> CODE_PATH;
};