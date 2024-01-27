#pragma once 

#include "constants.hpp"
#include "types.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string_view>
#include <string>
#include <fstream>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <thread>
#include <set>

#include <fmt/format.h>
#include <fmt/chrono.h>


typedef long curl_off_t;
namespace gdpm::utils {

	using namespace std::chrono_literals;
	// extern indicators::DynamicProgress<indicators::ProgressBar> bars;
	struct memory_buffer{
		char *addr = nullptr;
		size_t size = 0;
	};

	static memory_buffer make_buffer(){
		return memory_buffer{
			.addr = (char*)malloc(1), /* ...will grow as needed in curl_write_to_stream */
			.size = 0
		};
	}

	static void free_buffer(memory_buffer& buf){
		free(buf.addr);
	}

	/* Use ISO 8601 for default timestamp format. */
	static inline auto timestamp(const std::string& format = GDPM_TIMESTAMP_FORMAT){
		time_t t = std::time(nullptr);
#if GDPM_ENABLE_TIMESTAMPS == 1
		return fmt::format(fmt::runtime("{"+format+"}"), fmt::localtime(t));
#else
		return "";
#endif
		// return fmt::format(format, std::chrono::system_clock::now());
	}

	template <typename T, typename...Args>
	void push_back(std::vector<T>& v, Args&&...args)
	{ 
		static_assert((std::is_constructible_v<T, Args&&>&& ...));
		(v.emplace_back(std::forward<Args>(args)), ...);
	}

	// A make_tuple wrapper for enforcing certain requirements
	template <typename...Args>
	auto range(Args...args)
	{
		// Limit number of args to only 2
		static_assert(sizeof...(Args) != 1, "Ranges requires only 2 arguments");
		return std::make_tuple(std::forward<Args>(args)...); 
	}
	template <class T, class F>
	void move_if_not(std::vector<T>& from, std::vector<T>& to, F pred){
		auto part = std::partition(from.begin(), from.end(), pred);
		std::move(part, from.end(), std::back_inserter(from));
		from.erase(part);
	}
	template <class T>
	std::vector<T> append(const std::vector<T>& a, const std::vector<T>& b){
		a.insert(std::end(a), std::begin(b), std::end(b));
		return a;
	}

	template <typename...Args>
	std::string format(std::string_view fmt, Args&&...args){
		return std::vformat(fmt, std::make_format_args(args...));
	}

	bool to_bool(const std::string& s);
	std::vector<std::string> split_lines(const std::string& contents);
	std::string readfile(const std::string& path);
	std::string to_lower(const std::string& s);
	std::string trim(const std::string& s);
	std::string trim_left(const std::string& s);
	std::string trim_left(const std::string& s, const std::string& ref);
	std::string trim_right(const std::string& s);
	std::string trim_right(const std::string& s, const std::string& ref);
	std::vector<std::string> parse_lines(const std::string& s);
	std::string replace_first(const std::string& s, const std::string& from, const std::string& to);
	std::string replace_all(const std::string& s, const std::string& from, const std::string& to);
	error extract_zip(const char *archive, const char *dest, int verbose = 0);
	std::string prompt_user(const char *message);
	bool prompt_user_yn(const char *message);
	void delay(std::chrono::milliseconds milliseconds = GDPM_REQUEST_DELAY);
	std::string join(const std::vector<std::string>& target, const std::string& delimiter = ", ");
	std::string join(const std::unordered_map<std::string, std::string>& target, const std::string& prefix = "", const std::string& delimiter = "\n");
	std::string convert_size(long size);
	
	// TODO: Add function to get size of decompressed zip

	namespace json {
		std::string from_array(const std::set<std::string>& a, const std::string& prefix);
		std::string from_object(const std::unordered_map<std::string, std::string>& m, const std::string& prefix, const std::string& spaces);
	}
}
