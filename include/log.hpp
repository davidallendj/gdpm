
#pragma once

#include "utils.hpp"
#include "colors.hpp"
#include "types.hpp"
#include <format>
// #include <fmt/core.h>

#if __cplusplus > 201703L
	// #include <format>
#else
#endif
// #include <fmt/printf.h>
// #include <fmt/format.h>

/*
TODO: Allow setting the logging *prefix*
TODO: Write log information to file
*/
namespace gdpm::log
{
	
	enum level{
		NONE = 0,
		INFO,
		WARNING,
		DEBUG,
		ERROR
	};

	struct context {
		int level;
		string prefix;
		string path;
		bool print_to_stdout;
		bool print_to_stderr;
	};

	static void vlog(fmt::string_view format, fmt::format_args args){
		fmt::vprint(format, args);
	}

	static void vlog(FILE *fp, fmt::string_view format, fmt::format_args args){
		fmt::vprint(fp, format, args);
	}

	template <typename S, typename...Args>
	static constexpr void info(const S& format, Args&&...args){
#if GDPM_LOG_LEVEL > NONE
		vlog(
			fmt::format(GDPM_COLOR_LOG_INFO "[INFO {}] {}\n" GDPM_COLOR_LOG_RESET, utils::timestamp(), format),
			// fmt::make_format_args<Args...>(args...)
			fmt::make_format_args(args...)
		);
#endif
	}

	template <typename S, typename...Args>
	static constexpr void info(const S& prefix, const S& format, Args&&...args){
#if GDPM_LOG_LEVEL > INFO
		vlog(
			fmt::format(GDPM_COLOR_LOG_INFO + prefix + GDPM_COLOR_LOG_RESET, format),
			fmt::make_format_args(args...)
		);
#endif
	}

	template <typename S, typename...Args>
	static constexpr void info(const context& context, const S& format, Args&&...args){
#if GDPM_LOG_LEVEL > INFO
		vlog(
			fmt::format(GDPM_COLOR_LOG_INFO + context.prefix + GDPM_COLOR_LOG_RESET, format),
			fmt::make_format_args(args...)
		);
#endif
	}

	template <typename S, typename...Args>
	static constexpr void info_n(const S& format, Args&&...args){
#if GDPM_LOG_LEVEL > INFO
		vlog(
			fmt::format(GDPM_COLOR_LOG_INFO "[INFO {}] {}" GDPM_COLOR_LOG_RESET, utils::timestamp(), format),
			// fmt::make_format_args<Args...>(args...)
			fmt::make_format_args(args...)
		);
#endif
	}

	template <typename S, typename...Args>
	static constexpr void error(const S& format, Args&&...args){
#if GDPM_LOG_LEVEL > ERROR
		vlog(
			fmt::format(GDPM_COLOR_LOG_ERROR "[ERROR {}] {}\n" GDPM_COLOR_LOG_RESET, utils::timestamp(), format),
			// fmt::make_format_args<Args...>(args...)
			fmt::make_format_args(args...)
		);
#endif
	}

	template <typename S, typename...Args>
	static constexpr void error(const S& prefix, const S& format, Args&&...args){
#if GDPM_LOG_LEVEL > ERROR
		vlog(
			fmt::format(GDPM_COLOR_LOG_ERROR + prefix + GDPM_COLOR_LOG_RESET, format),
			fmt::make_format_args(args...)
		);
#endif
	}

	template <typename S, typename...Args>
	static constexpr void error(const context& context, const S& format, Args&&...args){
#if GDPM_LOG_LEVEL > ERROR
		vlog(
			fmt::format(GDPM_COLOR_LOG_ERROR + context.prefix + GDPM_COLOR_LOG_RESET, format),
			fmt::make_format_args(args...)
		);
#endif
	}

	template <typename S, typename...Args>
	static constexpr void debug(const S& format, Args&&...args){
#if GDPM_LOG_LEVEL > DEBUG
		vlog(
			fmt::format(GDPM_COLOR_LOG_DEBUG "[DEBUG {}] {}\n" GDPM_COLOR_LOG_RESET, utils::timestamp(), format),
			// fmt::make_format_args<Args...>(args...)
			fmt::make_format_args(args...)
		);
#endif
	}

	template <typename S, typename...Args>
	static constexpr void debug(const S& prefix, const S& format, Args&&...args){
#if GDPM_LOG_LEVEL > DEBUG
		vlog(
			fmt::format(GDPM_COLOR_LOG_DEBUG + prefix + GDPM_COLOR_LOG_RESET, format),
			fmt::make_format_args(args...)
		);
#endif
	}

	template <typename S, typename...Args>
	static constexpr void debug(const context& context, const S& format, Args&&...args){
#if GDPM_LOG_LEVEL > DEBUG
		vlog(
			fmt::format(GDPM_COLOR_LOG_DEBUG + context.prefix + GDPM_COLOR_LOG_RESET, format),
			fmt::make_format_args(args...)
		);
#endif
	}

	template <typename S, typename...Args>
	static constexpr void print(const S& format, Args&&...args){
		vlog(
			fmt::format("{}", format),
			// fmt::make_format_args<Args...>(args...)
			fmt::make_format_args(args...)
		);
	}

	template <typename S = string, typename...Args>
	static constexpr void println(const S& format, Args&&...args){
		vlog(
			fmt::format("{}\n", format),
			// fmt::make_format_args<Args...>(args...)
			fmt::make_format_args(args...)
		);
	}

	template <typename>
	static constexpr void println(const string& format = ""){
		println(format);
	}

}