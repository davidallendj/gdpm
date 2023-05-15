#pragma once

#include "config.hpp"
#include "package_manager.hpp"
#include <cstdio>
#include <cxxopts.hpp>
#include <memory>
#include <string>
#include <unordered_map>
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
		p_export, /* reserved keyword */
		list, 
		link,
		clone,
		clean,
		sync,
		remote,
		help, 
		none 
	};

	using package_list 		= std::vector<package_info>;
	using package_titles 	= std::vector<std::string>;
	using cl_arg 			= std::variant<int, bool, float, std::string>;
	using cl_args			= std::vector<cl_arg>;
	using cl_opts			= std::unordered_map<std::string, cl_args>;

	GDPM_DLL_EXPORT int initialize(int argc, char **argv);
	GDPM_DLL_EXPORT int execute();
	GDPM_DLL_EXPORT void finalize();

	/* Package management API */
	GDPM_DLL_EXPORT error install_packages(const std::vector<std::string>& package_titles, bool skip_prompt = false);
	GDPM_DLL_EXPORT error remove_packages(const std::vector<std::string>& package_titles, bool skip_prompt = false);
	GDPM_DLL_EXPORT error remove_all_packages();
	GDPM_DLL_EXPORT error update_packages(const std::vector<std::string>& package_titles, bool skip_prompt = false);
	GDPM_DLL_EXPORT error search_for_packages(const std::vector<std::string>& package_titles, bool skip_prompt = false);
	GDPM_DLL_EXPORT error export_packages(const std::vector<std::string>& paths);
	GDPM_DLL_EXPORT std::vector<std::string> list_information(const std::vector<std::string>& opts, bool print_list = true);
	GDPM_DLL_EXPORT void clean_temporary(const std::vector<std::string>& package_titles);
	GDPM_DLL_EXPORT void link_packages(const std::vector<std::string>& package_titles, const std::vector<std::string>& paths);
	GDPM_DLL_EXPORT void clone_packages(const std::vector<std::string>& package_titles, const std::vector<std::string>& paths);
	
	/* Remote API */
	GDPM_DLL_EXPORT error _handle_remote(const std::vector<std::string>& args, const std::vector<std::string>& opts);
	GDPM_DLL_EXPORT void remote_add_repository(const std::vector<std::string>& repositories);
	GDPM_DLL_EXPORT void remote_remove_respository(const std::vector<std::string>& repositories);
	GDPM_DLL_EXPORT void remote_remove_respository(ssize_t index);
	GDPM_DLL_EXPORT void remote_move_repository(int old_position, int new_position);

	/* Auxiliary Functions */
	GDPM_DLL_EXPORT cxxargs _parse_arguments(int argc, char **argv);
	GDPM_DLL_EXPORT void _handle_arguments(const cxxargs& args);
	GDPM_DLL_EXPORT void run_command(command_e command, const std::vector<std::string>& package_titles, const std::vector<std::string>& opts);
	GDPM_DLL_EXPORT void print_package_list(const rapidjson::Document& json);
	GDPM_DLL_EXPORT void print_package_list(const std::vector<package_info>& packages);
	GDPM_DLL_EXPORT void print_remote_sources();
	GDPM_DLL_EXPORT std::vector<std::string> get_package_titles(const std::vector<package_info>& packages);

	/* Dependency Management API */
	GDPM_DLL_EXPORT std::vector<package_info> synchronize_database(const std::vector<std::string>& package_titles);
	GDPM_DLL_EXPORT std::vector<std::string> resolve_dependencies(const std::vector<std::string>& package_titles);
}