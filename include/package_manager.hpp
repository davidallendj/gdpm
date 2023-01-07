#pragma once

#include "config.hpp"
#include <cstdio>
#include <cxxopts.hpp>
#include <memory>
#include <string>
#include <vector>

#include <rapidjson/document.h>
#include <curl/curl.h>

namespace gdpm::package_manager{
	extern std::vector<std::string> repo_sources;
	extern CURL *curl;
	extern CURLcode res;
	extern config::context config;

	struct package_info{
		size_t asset_id;
		std::string type;
		std::string title;
		std::string author;
		size_t author_id;
		std::string version;
		std::string godot_version;
		std::string cost;
		std::string description;
		std::string modify_date;
		std::string support_level;
		std::string category;
		std::string remote_source;
		std::string download_url;
		std::string download_hash;
		bool is_installed;
		std::string install_path;
		std::vector<package_info> dependencies;
	};

	struct cxxargs{
		cxxopts::ParseResult result;
		cxxopts::Options options;
	};

	enum command_e{ 
		install, 
		remove, 
		update, 
		search, 
		list, 
		link,
		clone,
		clean,
		sync,
		add_remote,
		delete_remote, 
		help, 
		none 
	};

	GDPM_DLL_EXPORT int initialize(int argc, char **argv);
	GDPM_DLL_EXPORT int execute();
	GDPM_DLL_EXPORT void finalize();
	GDPM_DLL_EXPORT error install_packages(const std::vector<std::string>& package_titles, bool skip_prompt = false);
	GDPM_DLL_EXPORT error remove_packages(const std::vector<std::string>& package_titles, bool skip_prompt = false);
	GDPM_DLL_EXPORT error update_packages(const std::vector<std::string>& package_titles, bool skip_prompt = false);
	GDPM_DLL_EXPORT error search_for_packages(const std::vector<std::string>& package_titles, bool skip_prompt = false);
	GDPM_DLL_EXPORT void list_information(const std::vector<std::string>& opts);
	GDPM_DLL_EXPORT void clean_temporary(const std::vector<std::string>& package_titles);
	GDPM_DLL_EXPORT void link_packages(const std::vector<std::string>& package_titles, const std::vector<std::string>& paths);
	GDPM_DLL_EXPORT void clone_packages(const std::vector<std::string>& package_titles, const std::vector<std::string>& paths);
	GDPM_DLL_EXPORT void add_remote_repository(const std::string& repository, ssize_t offset = -1);
	GDPM_DLL_EXPORT void delete_remote_repository(const std::string& repository);
	GDPM_DLL_EXPORT void delete_remote_repository(ssize_t index);

	GDPM_DLL_EXPORT cxxargs parse_arguments(int argc, char **argv);
	GDPM_DLL_EXPORT void handle_arguments(const cxxargs& args);
	GDPM_DLL_EXPORT void run_command(command_e command, const std::vector<std::string>& package_titles, const std::vector<std::string>& opts);
	GDPM_DLL_EXPORT void print_package_list(const rapidjson::Document& json);
	GDPM_DLL_EXPORT void print_package_list(const std::vector<package_info>& packages);
	GDPM_DLL_EXPORT void print_remote_sources();

	GDPM_DLL_EXPORT std::vector<package_info> synchronize_database(const std::vector<std::string>& package_titles);
	GDPM_DLL_EXPORT std::vector<std::string> resolve_dependencies(const std::vector<std::string>& package_titles);
}