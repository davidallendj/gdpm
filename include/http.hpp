#pragma once

#include "constants.hpp"
#include "types.hpp"
#include <unordered_map>

namespace gdpm::http{
	using headers_t = std::unordered_map<string, string>;
	enum response_code{
		OK 			= 200,
		NOT_FOUND 	= 400
	};
	struct response{
		long code = 0;
		string body{};
		headers_t headers{};
		error error();
	};


	struct params {
		size_t timeout = GDPM_CONFIG_TIMEOUT_MS;
		int verbose = 0;
	};

	string url_escape(const string& url);
	response request_get(const string& url, const http::params& params = http::params());
	response request_post(const string& url, const char *post_fields="", const http::params& params = http::params());
	response download_file(const string& url, const string& storage_path, const http::params& params = http::params());
	
}