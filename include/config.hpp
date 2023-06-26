#pragma once 

#include "constants.hpp"
#include "error.hpp"
#include "package.hpp"

#include <rapidjson/document.h>
#include <string>
#include <filesystem>
#include <vector>
#include <unordered_map>


namespace gdpm::package{
	struct info;
	struct params;
}

namespace gdpm::config{
	struct context{
		string username;
		string password;
		string path;
		string token;
		string packages_dir;
		string tmp_dir;
		string_map remote_sources;
		size_t jobs					= 1;
		size_t timeout				= 3000;
		size_t max_results			= 200;
		bool enable_sync			= true;
		bool enable_cache			= true;
		bool skip_prompt			= false;
		bool enable_file_logging;
		bool clean_temporary;
		int verbose;
		package::info info;
		rest_api::request_params api_params;
	};

	string to_json(const context& config, bool pretty_print = false);
	error load(std::filesystem::path path, context& config);
	error save(std::filesystem::path path, const context& config);
	error handle_config(config::context& config, const args_t& args, const var_opts& opts);
	context make_context(const string& username = GDPM_CONFIG_USERNAME, const string& password = GDPM_CONFIG_PASSWORD, const string& path = GDPM_CONFIG_PATH, const string& token = GDPM_CONFIG_TOKEN, const string& godot_version = GDPM_CONFIG_GODOT_VERSION, const string& packages_dir = GDPM_CONFIG_LOCAL_PACKAGES_DIR, const string& tmp_dir = GDPM_CONFIG_LOCAL_TMP_DIR, const string_map& remote_sources = {GDPM_CONFIG_REMOTE_SOURCES}, size_t threads = GDPM_CONFIG_THREADS, size_t timeout = 0, bool enable_sync = GDPM_CONFIG_ENABLE_SYNC, bool enable_file_logging = GDPM_CONFIG_ENABLE_FILE_LOGGING, int verbose = GDPM_CONFIG_VERBOSE);
	error validate(const rapidjson::Document& doc);
	void print_json(const context& config);
	void print_properties(const context& config, const string_list& properties);

	extern context config;
}