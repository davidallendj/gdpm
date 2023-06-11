
#include "remote.hpp"
#include "error.hpp"
#include "log.hpp"
#include "types.hpp"
#include <readline/readline.h>

namespace gdpm::remote{
	error handle_remote(
		config::context& config, 
		const args_t& args, 
		const var_opts& opts
	){
		/* Check if enough arguments are supplied */
		size_t argc = args.size();
		if (argc < 1){
			print_repositories(config);
			return error();
		}

		/* Check which subcommand is supplied */
		string sub_command = args.front();
		if(sub_command == "add"){
			if(args.size() < 3 || args.empty()){
				error error(
					constants::error::INVALID_ARG_COUNT,
					"Invalid number of args."
				);
				log::error(error);
				return error;
			}
			string name = args[1];
			string url = args[2];
			add_repositories(config, {{name, url}});
		}
		else if (sub_command == "remove") {
			if(args.size() < 2 || args.empty()){
				error error(
					constants::error::INVALID_ARG_COUNT,
					"Invalid number of args."
				);
				log::error(error);
				return error;
			}
			remove_respositories(config, {args.begin()+1, args.end()});
		}
		// else if (sub_command == "set")		set_repositories(config::context &context, const repository_map &repos)
		else if (sub_command == "list")		print_repositories(config);
		else{
			error error(
				constants::error::UNKNOWN,
				"Unknown sub-command. Try 'gdpm help remote' for options."
			);
			log::error(error);
			return error;
		}
		return error();
	}

	
	void set_repositories(
		config::context& config, 
		const repository_map &repos
	){
		config.remote_sources = repos;
	}


	void add_repositories(
		config::context& config, 
		const repository_map &repos
	){
		std::for_each(repos.begin(), repos.end(), 
			[&config](const string_pair& p){
				config.remote_sources.insert(p);
			}
		);
	}


	void remove_respositories(
		config::context& config, 
		const repo_names& names
	){
		for(auto it = names.begin(); it != names.end();){
			if(config.remote_sources.contains(*it)){
				config.remote_sources.erase(*it);
			}
			it++;
		}
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