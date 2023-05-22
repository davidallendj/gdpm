
#pragma once

#include "utils.hpp"
#include "colors.hpp"
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
	template <typename...Args> concept RequireMinArgs = requires (std::size_t min){ sizeof...(Args) > min; };
	
	enum level{
		NONE,
		INFO,
		WARNING,
		DEBUG,
		ERROR
	};

	struct config {
		static int level;
		static std::string prefix;
		static std::string path;
		static bool print_to_stdout;
		static bool print_to_stderr;
	};

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
			// fmt::make_format_args<Args...>(args...)
			fmt::make_format_args(args...)
		);
#endif
	}

	template <typename S, typename...Args>
	static constexpr void info_n(const S& format, Args&&...args){
		vlog(
			fmt::format(GDPM_COLOR_LOG_INFO "[INFO {}] {}" GDPM_COLOR_LOG_RESET, utils::timestamp(), format),
			// fmt::make_format_args<Args...>(args...)
			fmt::make_format_args(args...)
		);
	}

	template <typename S, typename...Args>
	static constexpr void error(const S& format, Args&&...args){
#if GDPM_LOG_LEVEL > 1
		vlog(
			fmt::format(GDPM_COLOR_LOG_ERROR "[ERROR {}] {}\n" GDPM_COLOR_LOG_RESET, utils::timestamp(), format),
			// fmt::make_format_args<Args...>(args...)
			fmt::make_format_args(args...)
		);
#endif
	}

	template <typename S, typename...Args>
	static constexpr void debug(const S& format, Args&&...args){
#if GDPM_LOG_LEVEL > 1
		vlog(
			fmt::format(GDPM_COLOR_LOG_DEBUG "[DEBUG {}] {}\n" GDPM_COLOR_LOG_RESET, utils::timestamp(), format),
			// fmt::make_format_args<Args...>(args...)
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

	template <typename S = std::string, typename...Args>
	static constexpr void println(const S& format, Args&&...args){
		vlog(
			fmt::format("{}\n", format),
			// fmt::make_format_args<Args...>(args...)
			fmt::make_format_args(args...)
		);
	}

	template <typename>
	static constexpr void println(const std::string& format = ""){
		println(format);
	}

}