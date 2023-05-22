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
	extern remote::repository_map repo_sources;
	extern CURL *curl;
	extern CURLcode res;
	extern config::context config;	

	struct cxxargs{
		cxxopts::ParseResult result;
		cxxopts::Options options;
	};

	struct exec_args{
		args_t args;
		opts_t opts;
	};

	enum class action_e{ 
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

GDPM_DLL_EXPORT result_t<exec_args> initialize(int argc, char **argv);
	GDPM_DLL_EXPORT int execute(const args_t& args, const opts_t& opts);
	GDPM_DLL_EXPORT void finalize();

	/* Auxiliary Functions */
	GDPM_DLL_EXPORT cxxargs _parse_arguments(int argc, char **argv);
	GDPM_DLL_EXPORT result_t<exec_args> _handle_arguments(const cxxargs& args);
	GDPM_DLL_EXPORT void run_command(action_e command, const package::title_list& package_titles, const opts_t& opts);
}