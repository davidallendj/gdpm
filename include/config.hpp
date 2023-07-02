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
		int jobs					= 1;
		int timeout					= 3000;
		bool enable_sync			= true;
		bool enable_cache			= true;
		bool skip_prompt			= false;
		bool enable_file_logging;
		bool clean_temporary;

		int verbose					= log::INFO;
		print::style style			= print::style::list;
		package::info info;
		rest_api::request_params rest_api_params;
	};

	string from_style(const print::style& style);
	print::style to_style(const string& s);
	string to_json(const context& config, bool pretty_print = false);
	error load(std::filesystem::path path, context& config);
	error save(std::filesystem::path path, const context& config);
	error handle_config(config::context& config, const args_t& args, const var_opts& opts);
	error set_property(config::context& config, const string& property, const string& value);
	template <typename T = any>
	T& get_property(const config::context& config, const string& property);
	context make_context(const string& username = GDPM_CONFIG_USERNAME, const string& password = GDPM_CONFIG_PASSWORD, const string& path = GDPM_CONFIG_PATH, const string& token = GDPM_CONFIG_TOKEN, const string& godot_version = GDPM_CONFIG_GODOT_VERSION, const string& packages_dir = GDPM_CONFIG_LOCAL_PACKAGES_DIR, const string& tmp_dir = GDPM_CONFIG_LOCAL_TMP_DIR, const string_map& remote_sources = {GDPM_CONFIG_REMOTE_SOURCES}, int jobs = GDPM_CONFIG_THREADS, int timeout = 0, bool enable_sync = GDPM_CONFIG_ENABLE_SYNC, bool enable_file_logging = GDPM_CONFIG_ENABLE_FILE_LOGGING, int verbose = GDPM_CONFIG_VERBOSE);
	error validate(const rapidjson::Document& doc);
	void print_json(const context& config);
	void print_properties(const context& config, const string_list& properties);

	extern context config;
}