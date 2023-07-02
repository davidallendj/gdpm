
#pragma once

#include "clipp.h"
#include "utils.hpp"
#include "colors.hpp"
#include "types.hpp"
#include <format>
#include <bitset>
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
	
	enum level_e : int{
		NONE 	= 0,
		ERROR	= 1,
		INFO	= 2,
		DEBUG	= 3,
		WARNING	= 4,
	};

	enum flag_opt {
		PRINT_STDOUT = 0b00000001,
		PRINT_STDERR = 0b00000010
	};

	struct prefix {
		string contents 		= "";
		string enclosing_start 	= "]";
		string enclosing_end 	= "[";
	};

	static int level						= INFO;
	static prefix prefix					= {};
	static string suffix					= "";
	static string path						= "";
	static std::bitset<8> flags				= PRINT_STDOUT | PRINT_STDERR;
	static bool print_to_stdout;	
	static bool print_to_stderr;

	inline constexpr level_e to_level(int l){
		return static_cast<level_e>(l);
	}

	inline constexpr int to_int(const level_e& l){
		return static_cast<int>(l);
	}

	inline constexpr void set_flag(uint8_t flag, bool value){
		(value) ? flags.set(flag) : flags.reset(flag);
	}

	inline constexpr bool get_flag(uint8_t flag){
		return flags.test(flag);
	}

	inline constexpr void set_prefix_if(const std::string& v, bool predicate = prefix.contents.empty()){
		prefix.contents = (predicate) ? v : prefix.contents;
	}

	inline constexpr void set_suffix_if(const std::string& v, bool predicate = suffix.empty()){
		suffix = (predicate) ? v : suffix;
	}

	inline constexpr const char* get_info_prefix() { return "[INFO {}] "; }
	inline constexpr const char* get_error_prefix() { return "[ERROR {}] "; }
	inline constexpr const char* get_debug_prefix() { return "[DEBUG {}] "; }
	inline constexpr const char* get_warning_prefix() { return "[WARN {}] "; }

	static void vlog(fmt::string_view format, fmt::format_args args){
		fmt::vprint(format, args);
	}

	static void vlog(FILE *fp, fmt::string_view format, fmt::format_args args){
		fmt::vprint(fp, format, args);
	}

	template <typename S, typename...Args>
	static constexpr void info(const S& format, Args&&...args){
		if(log::level < to_int(log::INFO))
			return;
#if GDPM_LOG_LEVEL > NONE
		set_prefix_if(fmt::format( get_info_prefix(), utils::timestamp()), true);
		set_suffix_if("\n");
		vlog(
			fmt::format(GDPM_COLOR_LOG_INFO "{}{}{}" GDPM_COLOR_LOG_RESET, prefix.contents, format, suffix),
			fmt::make_format_args(args...)
		);
#endif
	}

	template <typename S, typename...Args>
	static constexpr void info_n(const S& format, Args&&...args){
		if(log::level < to_int(log::INFO))
			return;
#if GDPM_LOG_LEVEL > INFO
		set_prefix_if(fmt::format(get_info_prefix(), utils::timestamp()), true);
		set_suffix_if("", true);
		vlog(
			fmt::format(GDPM_COLOR_LOG_INFO "{}{}{}" GDPM_COLOR_LOG_RESET, prefix.contents, format, suffix),
			fmt::make_format_args(args...)
		);
#endif
	}

	template <typename S, typename...Args>
	static constexpr void error(const S& format, Args&&...args){
		if(log::level < to_int(log::ERROR))
			return;
#if GDPM_LOG_LEVEL > ERROR
		set_prefix_if(std::format(get_error_prefix(), utils::timestamp()), true);
		set_suffix_if("\n");
		vlog(
			fmt::format(GDPM_COLOR_LOG_ERROR "{}{}{}" GDPM_COLOR_LOG_RESET, prefix.contents, format, suffix),
			fmt::make_format_args(args...)
		);
#endif
	}

	template <typename S, typename...Args>
	static constexpr void debug(const S& format, Args&&...args){
		if(log::level < to_int(log::DEBUG))
			return;
#if GDPM_LOG_LEVEL > DEBUG
		set_prefix_if(std::format(get_debug_prefix(), utils::timestamp()), true);
		set_suffix_if("\n");
		vlog(
			fmt::format(GDPM_COLOR_LOG_DEBUG "{}{}{}" GDPM_COLOR_LOG_RESET, prefix.contents, format, suffix),
			fmt::make_format_args(args...)
		);
#endif
	}

	template <typename S, typename...Args>
	static constexpr void warn(const S& format, Args&&...args){
		if(log::level < to_int(log::WARNING))
			return;
#if GDPM_LOG_LEVEL > WARN
		set_prefix_if(std::format(get_warning_prefix(), utils::timestamp()), true);
		set_suffix_if("\n");
		vlog(
			fmt::format(GDPM_COLOR_LOG_WARNING "{}{}{}" GDPM_COLOR_LOG_RESET, prefix.contents, format, suffix),
			fmt::make_format_args(args...)
		);
#endif
	}


	template <typename S, typename...Args>
	static constexpr void print(const S& format, Args&&...args){
		vlog(
			fmt::format("{}", format),
			fmt::make_format_args(args...)
		);
	}

	template <typename S = string, typename...Args>
	static constexpr void println(const S& format, Args&&...args){
		vlog(
			fmt::format("{}\n", format),
			fmt::make_format_args(args...)
		);
	}

	template <typename>
	static constexpr void println(const string& format = ""){
		println(format);
	}

}