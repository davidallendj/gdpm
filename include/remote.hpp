#pragma once

#include "constants.hpp"
#include "error.hpp"
#include "types.hpp"
#include <fmt/core.h>
#include <unordered_map>
#include "config.hpp"

namespace gdpm::remote{
	GDPM_DLL_EXPORT error add_repository(config::context& config, const args_t& args);
	GDPM_DLL_EXPORT error remove_respositories(config::context& config, const args_t& names);
	GDPM_DLL_EXPORT void move_repository(config::context& config, int old_position, int new_position);
	GDPM_DLL_EXPORT void print_repositories(const config::context& config);
}