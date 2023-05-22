
#include "remote.hpp"
#include "error.hpp"
#include "log.hpp"
#include "types.hpp"
#include <readline/readline.h>

namespace gdpm::remote{
	error _handle_remote(
		config::context& config, 
		const args_t& args, 
		const opts_t& opts
	){
		log::println("_handle_remote");
		for(const auto& arg : args){
			log::println("arg: {}", arg);
		}
		for(const auto& opt : opts){
			log::println("opt: {}:{}", opt.first, utils::join(opt.second));
		}

		/* Check if enough arguments are supplied */
		size_t argc = args.size();
		if (argc < 1){
			print_repositories(config);
			return error();
		}

		/* Check which subcommand is supplied */
		string sub_command = args.front();
		args_t argv(args.begin()+1, args.end());
		if(argv.size() < 2){
			error error(
				constants::error::INVALID_ARGS,
				"Invalid number of args"
			);
			log::error(error);
			return error;
		}
		string name = argv[1];
		string url = argv[2];
		if(sub_command == "add") 			add_repositories(config, {{name, url}});
		else if (sub_command == "remove") 	remove_respositories(config, argv);
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
		std::for_each(names.end(), names.begin(), [&config](const string& repo){
			config.remote_sources.erase(repo);
		});
	}


	void move_respository(
		config::context& config, 
		int old_position, 
		int new_position
	){

	}

	void print_repositories(const config::context& config){
		log::println("Remote sources:");
		const auto &rs = config.remote_sources;
		std::for_each(rs.begin(), rs.end(), [](const string_pair& p){
			log::println("\t{}: {}", p.first, p.second);
		});
	}
}