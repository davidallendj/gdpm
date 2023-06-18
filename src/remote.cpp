
#include "remote.hpp"
#include "error.hpp"
#include "log.hpp"
#include "types.hpp"
#include <readline/readline.h>

namespace gdpm::remote{
	
	error add_repository(
		config::context &config, 
		const args_t &args
	){
		/* Check if enough args were provided. */
		log::debug("arg count: {}\nargs: {}", args.size(), utils::join(args));
		if (args.size() < 2){
			return error(
				constants::error::INVALID_ARG_COUNT,
				"Requires a remote name and url argument"
			);
		}

		/* Get the first two args */
		log::println("{}", args[0]);
		config.remote_sources.insert({args[0], args[1]});
		return error();
	}


	error remove_respositories(
		config::context& config, 
		const args_t& args
	){
		log::debug("arg count: {}\nargs: {}", args.size(), utils::join(args));
		if(args.size() < 1){
			return error(
				constants::error::INVALID_ARG_COUNT,
				"Requires at least one remote name argument"
			);
		}

		for(auto it = args.begin(); it != args.end();){
			if(config.remote_sources.contains(*it)){
				log::println("{}", *it);
				config.remote_sources.erase(*it);
			}
			it++;
		}

		return error();
	}


	void move_respository(
		config::context& config, 
		int old_position, 
		int new_position
	){

	}

	void print_repositories(const config::context& config){
		const auto &rs = config.remote_sources;
		std::for_each(rs.begin(), rs.end(), [](const string_pair& p){
			log::println("{}: {}", p.first, p.second);
		});
	}
}