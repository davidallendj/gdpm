
#pragma once

#include "utils.hpp"
#include "colors.hpp"
#include <fmt/core.h>

#if __cplusplus > 201703L
	// #include <format>
#else
#endif
#include <fmt/printf.h>
#include <fmt/format.h>

namespace gdpm::log
{
	template <typename...Args> concept RequireMinArgs = requires (std::size_t min){ sizeof...(Args) > min; };
	
	static void vlog(fmt::string_view format, fmt::format_args args){
		fmt::vprint(format, args);
	}

	static void vlog(FILE *fp, fmt::string_view format, fmt::format_args args){
		fmt::vprint(fp, format, args);
	}

	template <typename S, typename...Args>
	static constexpr void info(const S& format, Args&&...args){
#if GDPM_LOG_LEVEL > 0
		vlog(
			fmt::format(GDPM_COLOR_LOG_INFO "[INFO {}] {}\n" GDPM_COLOR_LOG_RESET, utils::timestamp(), format),
			fmt::make_args_checked<Args...>(format, args...)
		);
#endif
	}

	template <typename S, typename...Args>
	static constexpr void info_n(const S& format, Args&&...args){
		vlog(
			fmt::format(GDPM_COLOR_LOG_INFO "[INFO {}] {}" GDPM_COLOR_LOG_RESET, utils::timestamp(), format),
			fmt::make_args_checked<Args...>(format, args...)
		);
	}

	template <typename S, typename...Args>
	static constexpr void error(const S& format, Args&&...args){
#if GDPM_LOG_LEVEL > 1
		vlog(
			fmt::format(GDPM_COLOR_LOG_ERROR "[ERROR {}] {}\n" GDPM_COLOR_LOG_RESET, utils::timestamp(), format),
			fmt::make_args_checked<Args...>(format, args...)
		);
#endif
	}

	template <typename S, typename...Args>
	static constexpr void debug(const S& format, Args&&...args){
#if GDPM_LOG_LEVEL > 1
		vlog(
			fmt::format(GDPM_COLOR_LOG_DEBUG "[DEBUG {}] {}\n" GDPM_COLOR_LOG_RESET, utils::timestamp(), format),
			fmt::make_args_checked<Args...>(format, args...)
		);
#endif
	}

	template <typename S, typename...Args>
	static constexpr void print(const S& format, Args&&...args){
		vlog(
			fmt::format("{}", format),
			fmt::make_args_checked<Args...>(format, args...)
		);
	}

	template <typename S, typename...Args>
	static constexpr void println(const S& format, Args&&...args){
		vlog(
			fmt::format("{}\n", format),
			fmt::make_args_checked<Args...>(format, args...)
		);
	}

}