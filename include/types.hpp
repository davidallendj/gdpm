#pragma once

#include <tuple>
#include <functional>
#include <type_traits>
#include <string>
#include <variant>
#include <future>
#include <any>

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

	enum variant_type_index : int{
		INT			= 0,
		FLOAT		= 1,
		BOOL		= 2,
		STRING		= 3,
		STRING_LIST = 4,
		STRING_MAP	= 5,
		SIZE_T		= 6,
	};

	template <typename T>
	concept error_t = requires{ std::is_same<error, T>::value; };

	using string 		= std::string;
	using string_list 	= std::vector<string>;
	using string_map 	= std::unordered_map<string, string>;
	using string_pair 	= std::pair<string, string>;
	using any			= std::any;
	using var 			= std::variant<int, float, bool, string, string_list, string_map, size_t>;
	template <typename T = var> 
	using _args_t 		= std::vector<T>;
	using args_t 		= _args_t<string>;
	using var_args		= _args_t<var>;
	using var_list		= std::vector<var>;
	using var_map		= std::unordered_map<string, var>;
	template <typename Key = string, class Value = _args_t<var>> 
	using _opts_t 		= std::unordered_map<Key, Value>;
	using opts_t 		= _opts_t<string, string_list>;
	using opt			= std::pair<string, string_list>;
	using var_opt		= std::pair<string, var>;
	using var_opts		= _opts_t<string, var>;
	template <typename T = error>
	using _task_list 	= std::vector<std::future<T>>;
	using task_list 	= _task_list<error>;

	inline string_list unwrap(const var_args& args){
		string_list sl;
		std::for_each(args.begin(), args.end(), [&sl](const var& v){
			if(v.index() == STRING){
				sl.emplace_back(std::get<string>(v));
			}
		});
		return sl;
	}


	inline opts_t unwrap(const var_opts& opts){
		opts_t o;
		std::for_each(opts.begin(), opts.end(), [&o](const var_opt& opt){
			if(opt.second.index() == STRING){
				string_list sl;
				string arg(std::get<string>(opt.second)); // must make copy for const&
				sl.emplace_back(arg);
				o.insert(std::pair(opt.first, sl));
			}
		});
		return o;
	}


	template <typename T>
	constexpr void get(const var& v, T& target){
		switch(v.index()){
			case INT: 			/*return*/ target = std::get<int>(v);
			case FLOAT: 		/*return*/ target = std::get<float>(v);
			case BOOL: 			/*return*/ target = std::get<bool>(v);
			case STRING: 		/*return*/ target = std::get<string>(v);
			case STRING_LIST: 	/*return*/ target = std::get<string_list>(v);
			case STRING_MAP:	/*return*/ target = std::get<string_map>(v);
			case SIZE_T:		/*return*/ target = std::get<size_t>(v);
			default: 			/*return*/ target = 0;
		}
	}
}