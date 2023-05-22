#pragma once 

#include "constants.hpp"
#include "error.hpp"

#include <rapidjson/document.h>
#include <string>
#include <filesystem>
#include <vector>
#include <unordered_map>

namespace gdpm::config{
	struct context{
		string username;
		string password;
		string path;
		string token;
		string godot_version;
		string packages_dir;
		string tmp_dir;
		string_map remote_sources;
		size_t threads;
		size_t timeout;
		bool enable_sync;
		bool enable_file_logging;
		int verbose;
	};
	string to_json(const context& params);
	error load(std::filesystem::path path, context& config, int verbose = 0);
	error save(std::filesystem::path path, const context& config, int verbose = 0);
	context make_context(const std::string& username = GDPM_CONFIG_USERNAME, const string& password = GDPM_CONFIG_PASSWORD, const string& path = GDPM_CONFIG_PATH, const string& token = GDPM_CONFIG_TOKEN, const string& godot_version = GDPM_CONFIG_GODOT_VERSION, const string& packages_dir = GDPM_CONFIG_LOCAL_PACKAGES_DIR, const string& tmp_dir = GDPM_CONFIG_LOCAL_TMP_DIR, const string_map& remote_sources = {GDPM_CONFIG_REMOTE_SOURCES}, size_t threads = GDPM_CONFIG_THREADS, size_t timeout = 0, bool enable_sync = GDPM_CONFIG_ENABLE_SYNC, bool enable_file_logging = GDPM_CONFIG_ENABLE_FILE_LOGGING, int verbose = GDPM_CONFIG_VERBOSE);
	error validate(const rapidjson::Document& doc);

	extern context config;
}