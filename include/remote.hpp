#pragma once

#include "constants.hpp"
#include "error.hpp"
#include "types.hpp"
#include <fmt/core.h>
#include <unordered_map>
#include "config.hpp"

namespace gdpm::remote{
	using repo_names 		= string_list;
	using repo_urls 		= string_list;
	using repository_map 	= string_map;

	GDPM_DLL_EXPORT error handle_remote(config::context& config, const args_t& args, const var_opts& opts);
	GDPM_DLL_EXPORT void set_repositories(config::context& context, const repository_map& repos);
	GDPM_DLL_EXPORT void add_repositories(config::context& context, const repository_map& repos);
	GDPM_DLL_EXPORT void remove_respositories(config::context& context, const repo_names& names);
	GDPM_DLL_EXPORT void move_repository(config::context& context, int old_position, int new_position);
	GDPM_DLL_EXPORT void print_repositories(const config::context& context);
}