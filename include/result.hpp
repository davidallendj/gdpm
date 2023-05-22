#pragma once

#include "log.hpp"
#include "error.hpp"
#include "types.hpp"
#include <functional>
#include <type_traits>

namespace gdpm{
	
	template <class T, error_t U = error>
	class result_t {
	public:
		result_t() = delete;
		result_t(
			std::tuple<T, U> tuple, 
			std::function<T()> ok = []() -> T{}, 
			std::function<void()> error = [](){}
		): data(tuple), fn_ok(ok), fn_error(error)
		{ }

		result_t(
			T target,
			U error,
			std::function<std::unique_ptr<T>()> _fn_ok = []() -> std::unique_ptr<T>{ return nullptr; },
			std::function<void()> _fn_error = [](){}
		): data(std::make_tuple(target, error)), fn_ok(_fn_ok), fn_error(_fn_error)
		{}

		void define(
			std::function<T()> ok,
			std::function<U()> error
		){
			fn_ok = ok;
			fn_error = error;
		}

		constexpr std::unique_ptr<T> unwrap() const { 
			/* First, check if ok() and error() are defined. */
			if(!fn_error || !fn_ok){
				error error(
					constants::error::NOT_DEFINED
				);
				log::error(error);
				return nullptr;
			}
			/* Then, attempt unwrap the data. */
			U err = std::get<U>(data);
			if (err.has_occurred())
				if(fn_error){
					fn_error();
					return nullptr;
				}
			return fn_ok(); 
		}

		constexpr T unwrap_or(T default_value) const {
			U err = std::get<U>(data);
			if(err.has_occurred())
				return default_value;
			return fn_ok();
		}

		constexpr T unwrap_unsafe() const {
			return std::get<T>(data);
		}

	private:
		std::tuple<T, U> data;
		std::function<std::unique_ptr<T>()> fn_ok;
		std::function<void()> fn_error;
	};
}