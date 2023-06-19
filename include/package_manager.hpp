#pragma once

#include "config.hpp"
#include "package.hpp"
#include "package_manager.hpp"
#include "remote.hpp"
#include "result.hpp"
#include <cstdio>
#include <cxxopts.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <rapidjson/document.h>
#include <curl/curl.h>

namespace gdpm::package_manager {
	extern CURL *curl;
	extern CURLcode res;
	extern config::context config;	

	enum class action_e{ 
		install, 
		add, 
		remove, 
		update, 
		search, 
		p_export, /* reserved keyword */
		list, 
		link,
		clone,
		clean,
		config_get,
		config_set,
		fetch,
		sync,
		remote_add,
		remote_remove,
		remote_list,
		ui,
		help, 
		version,
		none 
	};

	GDPM_DLL_EXPORT error initialize(int argc, char **argv);
	GDPM_DLL_EXPORT error parse_arguments(int argc, char **argv);
	GDPM_DLL_EXPORT error finalize();
	GDPM_DLL_EXPORT void run_command(action_e command, const var_args& args, const var_opts& opts);
}