#pragma once

#include "constants.hpp"

#include <unordered_map>

namespace gdpm::http{
	struct response{
		long code = 0;
		std::string body{};
		std::unordered_map<std::string, std::string> headers{};
	};

	response request_get(const std::string& url, size_t timeout = GDPM_CONFIG_TIMEOUT_MS, int verbose = 0);
	response request_post(const std::string& url, const char *post_fields="", size_t timeout = GDPM_CONFIG_TIMEOUT_MS, int verbose = 0);
	response download_file(const std::string& url, const std::string& storage_path, size_t timeout = GDPM_CONFIG_TIMEOUT_MS, int verbose = 0);
	
}