#pragma once

#include "constants.hpp"
#include "error.hpp"
#include "package.hpp"
#include "types.hpp"
#include "result.hpp"
#include "rest_api.hpp"
#include <cstdio>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>
#include <rapidjson/document.h>


namespace gdpm::config{
	struct context;
}

namespace gdpm::package {

	struct info {
		size_t asset_id;
		string type;
		string title;
		string author;
		size_t author_id;
		string version;
		string godot_version;
		string cost;
		string description;
		string modify_date;
		string support_level;
		string category;
		string remote_source;
		string download_url;
		string download_hash;
		bool is_installed;
		string install_path;
		std::vector<info> dependencies;
	};

	enum install_method_e : int{
		GLOBAL_LINK_LOCAL	= 0,
		GLOBAL_CLONE_LOCAL	= 1,
		GLOBAL_ONLY			= 2,
		LOCAL_ONLY			= 3
	};

	struct params {
		args_t 				args;
		var_opts 			opts;
		string_list			paths;
		string_list			input_files;
		string 				remote_source  = "origin";
		install_method_e 	install_method = GLOBAL_LINK_LOCAL;
	};

	using info_list 	= std::vector<info>;
	using title_list 	= std::vector<string>;
	using id_list 		= std::vector<size_t>;
	using path 			= std::string;
	using path_list		= std::vector<path>;
	using path_refs		= std::vector<std::reference_wrapper<const path>>;

	/*! 
	@brief Install a Godot package from the Asset Library in the current project.
	By default, packages are stored in a global directory and linked to the project
	where the tool is executed. Use the `--jobs` option to install packages in 
	parallel. Specify which remote source to use by passing the `--remote` option.
	By default, the first remote source will by used. Alternatively, if the
	`--use-remote=false` option is passed, then the tool will only attempt to fetch the
	package from cache. Use the `--use-cache=false` option to fetch package only from 
	remote source.

	`gdpm install "super cool example package"
		
	To only install a package global without linking to the project, use the 
	`--global-only` option.

	`gdpm install --global-only "super cool example package"

	To install a package to a local project only, use the `--local-only` option. 
	This will extract the package contents to the project location instead of the
	global install location. Use the `--path` option to specify an alternative
	path.

	`gdpm install --local-only "super cool example package" --path addons/example

	To copy the package to a project instead of linking, use the `--clone` option.

	`gdpm install --clone "super cool example package"

	*/
	GDPM_DLL_EXPORT error install(const config::context& config, title_list& package_titles, const params& params = package::params());
	/*!
	@brief Adds package to project locally only.
	@param config
	@param package_titles
	@param params 
	*/
	GDPM_DLL_EXPORT error get(const config::context& config, title_list& package_titles, const params& params = package::params());
	/*!
	@brief Remove's package and contents from local database.
	*/
	GDPM_DLL_EXPORT error remove(const config::context& config, title_list& package_titles, const params& params = package::params());
	GDPM_DLL_EXPORT error remove_all(const config::context& config, const params& params = package::params());
	GDPM_DLL_EXPORT error update(const config::context& config, const title_list& package_titles, const params& params = package::params());
	GDPM_DLL_EXPORT error search(const config::context& config, const title_list& package_titles, const params& params = package::params());
	GDPM_DLL_EXPORT error list(const config::context& config, const params& params = package::params());
	GDPM_DLL_EXPORT error export_to(const path_list& paths);
	GDPM_DLL_EXPORT error clean(const config::context& config, const title_list& package_titles);
	GDPM_DLL_EXPORT error purge(const config::context& config);
	GDPM_DLL_EXPORT error link(const config::context& config, const title_list& package_titles, const params& params = package::params());
	GDPM_DLL_EXPORT error clone(const config::context& config, const title_list& package_titles, const params& params = package::params());
	GDPM_DLL_EXPORT result_t<info_list> fetch(const config::context& config, const title_list& package_titles);


	GDPM_DLL_EXPORT void print_list(const rapidjson::Document& json);
	GDPM_DLL_EXPORT void print_list(const info_list& packages);
	GDPM_DLL_EXPORT void print_table(const info_list& packages);
	GDPM_DLL_EXPORT void print_table(const rapidjson::Document& json);
	GDPM_DLL_EXPORT result_t<info_list> get_package_info(const opts_t& opts);
	GDPM_DLL_EXPORT result_t<title_list> get_package_titles(const info_list& packages);
	GDPM_DLL_EXPORT void read_file_inputs(title_list& package_titles, const path_list& paths);
	GDPM_DLL_EXPORT info_list find_cached_packages(const title_list& package_titles);
	GDPM_DLL_EXPORT info_list find_installed_packages(const title_list& package_titles);
	/* Dependency Management API */
	GDPM_DLL_EXPORT result_t<info_list> resolve_dependencies(const config::context& config, const title_list& package_titles);

	GDPM_DLL_EXPORT string to_json(const info& info, bool pretty_print = false);
}