#pragma once

#include <tuple>
#include <functional>
#include <type_traits>
#include <string>
#include <variant>

namespace gdpm{
	class error;

	/*
	Base class to prevent derived class from creating copies.
	*/
	class non_copyable{
	public:
		non_copyable(){}
	
	private:
		non_copyable(const non_copyable&);
		non_copyable& operator=(const non_copyable&);
	};

	/*
	Base class to prevent derived classes from moving objects.
	*/
	class non_movable{
		non_movable(const non_movable&) = delete;
		non_movable(non_movable&&) = delete;
	};

	template <typename T>
	concept error_t = requires{ std::is_same<error, T>::value; };

	using string 		= std::string;
	using string_list 	= std::vector<string>;
	using string_map 	= std::unordered_map<string, string>;
	using string_pair 	= std::pair<string, string>;
	using var 			= std::variant<int, bool, float, std::string>;
	template <typename T = var> 
	using _args_t 		= std::vector<T>;
	using args_t 		= _args_t<string>;
	template <typename Key = std::string, class Value = _args_t<var>> 
	using _opts_t 		= std::unordered_map<Key, Value>;
	using opts_t 		= _opts_t<string, string_list>;

}
