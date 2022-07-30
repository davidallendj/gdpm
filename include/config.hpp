#pragma once 

#include "constants.hpp"

#include <string>
#include <filesystem>
#include <vector>
#include <set>


namespace gdpm::config{
	struct config_context{
		std::string username;
		std::string password;
		std::string path;
		std::string token;
		std::string godot_version;
		std::string packages_dir;
		std::string tmp_dir;
		std::set<std::string> remote_sources;
		size_t threads;
		size_t timeout;
		bool enable_sync;
		bool enable_file_logging;
		int verbose;
	};
	std::string to_json(const config_context& params);
	config_context load(std::filesystem::path path, int verbose = 0);
	int save(const config_context& config, int verbose = 0);
	config_context make_config(const std::string& username = GDPM_CONFIG_USERNAME, const std::string& password = GDPM_CONFIG_PASSWORD, const std::string& path = GDPM_CONFIG_PATH, const std::string& token = GDPM_CONFIG_TOKEN, const std::string& godot_version = GDPM_CONFIG_GODOT_VERSION, const std::string& packages_dir = GDPM_CONFIG_LOCAL_PACKAGES_DIR, const std::string& tmp_dir = GDPM_CONFIG_LOCAL_TMP_DIR, const std::set<std::string>& remote_sources = {GDPM_CONFIG_REMOTE_SOURCES}, size_t threads = GDPM_CONFIG_THREADS, size_t timeout = 0, bool enable_sync = GDPM_CONFIG_ENABLE_SYNC, bool enable_file_logging = GDPM_CONFIG_ENABLE_FILE_LOGGING, int verbose = GDPM_CONFIG_VERBOSE);

	extern config_context config;
}