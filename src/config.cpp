#include "config.hpp"
#include "error.hpp"
#include "log.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "error.hpp"
#include "types.hpp"

// RapidJSON
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>


// fmt
#include <fmt/format.h>
#include <cxxopts.hpp>
#include <iostream>
#include <string>
#include <string_view>
#include <ostream>
#include <fstream>
#include <ios>
#include <memory>
#include <set>
#include <unordered_map>


namespace gdpm::config{
	context config;
	
	string to_json(
		const context& config, 
		bool pretty_print
	){
		/* Build a JSON string to pass to document */
		string prefix = (pretty_print) ? "\n\t" : "";
		string spaces = (pretty_print) ? "    " : "";
		string json{
			"{" + prefix + "\"username\":" + spaces + "\"" + config.username + "\","
			+ prefix + "\"password\":" + spaces + "\"" + config.password + "\","
			+ prefix + "\"path\":"+ spaces + "\"" + config.path + "\","
			+ prefix + "\"token\":" + spaces + "\"" + config.token + "\","
			+ prefix + "\"godot_version\":" + spaces + "\"" + config.info.godot_version + "\","
			+ prefix + "\"packages_dir\":" + spaces + "\"" + config.packages_dir + "\","
			+ prefix + "\"tmp_dir\":" + spaces + "\"" + config.tmp_dir + "\","
			+ prefix + "\"remote_sources\":" + spaces + utils::json::from_object(config.remote_sources, prefix, spaces) + ","
			+ prefix + "\"threads\":" + spaces + fmt::to_string(config.jobs) + ","
			+ prefix + "\"timeout\":" + spaces + fmt::to_string(config.timeout) + ","
			+ prefix + "\"enable_sync\":" + spaces + fmt::to_string(config.enable_sync) + ","
			+ prefix + "\"enable_file_logging\":" + spaces + fmt::to_string(config.enable_file_logging)
			+ "\n}"
		};
		return json;
	}


	error load(
		std::filesystem::path path, 
		context& config
	){
		std::fstream file;
		file.open(path, std::ios::in);
		if(!file){
			if(config.verbose)
				log::info("No configuration file found. Creating a new one.");
			config = make_context();
			save(config.path, config);
			return error();
		}
		else if(file.is_open()){
			/* 
			 * See RapidJson docs:
			 *
			 * https://rapidjson.org/md_doc_tutorial.html
			 */
			using namespace rapidjson;
			
			/* Read JSON from config, parse, and check document. Must make sure that program does not crash here and use default config instead! */
			string contents, line;
			while(std::getline(file, line))
				contents += line + "\n";

			if(config.verbose > 0)
				log::info("Loading configuration file...\n{}", contents.c_str());
			
			Document doc;
			ParseErrorCode status = doc.Parse(contents.c_str()).GetParseError();

			if(!doc.IsObject()){
				error error(
					constants::error::FILE_NOT_FOUND,
					"Could not load config file."
				);
				log::error(error);
				return error;
			}

			error error = validate(doc);
			if(error()){
				log::error(error);
				return error;
			}

			/* Make sure contents were read correctly. */
			// if(!status){
			// 	log::error("config::load: Could not parse contents of file (Error: {}/{}).", GetParseError_En(status), doc.GetErrorOffset());

			// 	return context();
			// }

			/* Must check if keys exists first, then populate `_config_params`. */
			if(doc.HasMember("remote_sources")){
				if(doc["remote_sources"].IsObject()){
					const Value& srcs = doc["remote_sources"];
					for(auto& src : srcs.GetObject()){
						// config.remote_sources.push_back(src.GetString());
						config.remote_sources.insert(
							string_pair(src.name.GetString(), src.value.GetString())
						);
					}
				} else {
					gdpm::error error(
						constants::error::INVALID_KEY,
						"Could not read key `remote_sources`."
					);
					log::error(error);
					return error;
				}
			}
			auto _get_value_string = [](Document& doc, const char *property){
				if(doc.HasMember(property))
					if(doc[property].IsString())
						return doc[property].GetString();
				return "";
			};
			auto _get_value_int = [](Document& doc, const char *property){
				if(doc.HasMember(property))
					if(doc[property].IsInt())
						return doc[property].GetInt();
				return 0;
			};

			config.username 			= _get_value_string(doc, "username");
			config.password 			= _get_value_string(doc, "password");
			config.path 				= _get_value_string(doc, "path");
			config.token 				= _get_value_string(doc, "token");
			config.info.godot_version 	= _get_value_string(doc, "godot_version");
			config.packages_dir 		= _get_value_string(doc, "packages_dir");
			config.tmp_dir 				= _get_value_string(doc, "tmp_dir");
			config.jobs 				= _get_value_int(doc, "threads");
			config.enable_sync 			= _get_value_int(doc, "enable_sync");
			config.enable_file_logging 	= _get_value_int(doc, "enable_file_logging");
		}
		return error();
	}


	error save(
		std::filesystem::path path, 
		const context& config
	){
		using namespace rapidjson;

		/* Build a JSON string to pass to document */
		string json = to_json(config);
		if(config.verbose > 0)
			log::info("Saving configuration file...\n{}", json.c_str());
		
		/* Dump JSON config to file */
		Document doc;
		doc.Parse(json.c_str());
		std::ofstream ofs(path);
		OStreamWrapper osw(ofs);

		PrettyWriter<OStreamWrapper> writer(osw);
		doc.Accept(writer);

		return gdpm::error();
	}


	error handle_config(
		config::context& config,
		const args_t& args,
		const var_opts& opts
	){
		std::for_each(opts.begin(), opts.end(), [](const var_opt& p){
			log::println("opt: {}", p.first);

			if (p.second.index() == STRING_LIST){
				string_list p_list = get<string_list>(p.second);
				std::for_each(p_list.begin(), p_list.end(), [](const string& o){
					log::println("\t{}", o);
				});
			}
		});

		log::println("opts count: {}", opts.size());
		if(opts.contains("--username")){
			string_list v = get<string_list>(opts.at("--username"));
			log::println("username: {}", v[0]);
			if(v.empty()){
				log::println("username: {}", v[0]);
			}
			else
				config.username = v[0];
		}
		if(opts.contains("godot_version")){
			string_list v = get<string_list>(opts.at("godot-version"));
			if(v.empty()){
				// print godot-version
			}
			else
				config.info.godot_version = get<string_list>(opts.at("godot-version"))[0];
		}
		if(opts.contains("threads")){
			string_list v = get<string_list>(opts.at("threads"));
			if(v.empty()){
				
			}
		}

		save(config.path, config);

		return error();
	}


	context make_context(
		const string& username, 
		const string& password, 
		const string& path, 
		const string& token, 
		const string& godot_version, 
		const string& packages_dir, 
		const string& tmp_dir, 
		const string_map& remote_sources, 
		size_t threads, 
		size_t timeout, 
		bool enable_sync, 
		bool enable_file_logging, 
		int verbose
	){
		context config {
			.username 				= username,
			.password 				= password,
			.path 					= path,
			.token 					= token,
			.packages_dir 			= (packages_dir.empty()) ? string(getenv("HOME")) + ".gdpm" : packages_dir,
			.tmp_dir 				= tmp_dir,
			.remote_sources 		= remote_sources,
			.jobs 					= threads,
			.timeout 				= timeout,
			.enable_sync 			= enable_sync,
			.enable_file_logging 	= enable_file_logging,
			.verbose 				= verbose,
			.info 					= {
				.godot_version = godot_version,
			},
		};
		return config;
	}


	error validate(const rapidjson::Document& doc){
		error error(constants::error::INVALID_CONFIG, "");
		if(!doc.IsObject()){
			error.set_message("Document is not a JSON object.");
			return error;
		}
		if(!doc.HasMember("remote_sources")){
			error.set_message("Could not find `remote_sources` in config.");
			return error;
		}
		if(!doc["remote_sources"].IsObject()){
			error.set_message("Key `remote_sources` is not a JSON object.");
			return error;
		}
		error.set_code(constants::error::NONE);
		return error;
	}

	void print_json(const context& config){
		log::println("{}", to_json(config, true));
	}

	void _print_property(
		const context& config,
		const string& property
	){
		if(property.empty())					return;
		else if(property == "username") 		log::println("username: {}", config.username);
		else if(property == "password") 		log::println("password: {}", config.password);
		else if(property == "path") 			log::println("path: {}", config.path);
		else if(property == "token") 			log::println("token: {}", config.token);
		else if(property == "packages_dir") 	log::println("package directory: {}", config.packages_dir);
		else if(property == "tmp_dir") 			log::println("temporary directory: {}", config.tmp_dir);
		else if(property == "remote_sources") 	log::println("remote sources: \n{}", utils::join(config.remote_sources, "\t", "\n"));
		else if(property == "jobs") 			log::println("parallel jobs: {}", config.jobs);
		else if(property == "timeout") 			log::println("timeout: {}", config.timeout);
		else if(property == "sync") 			log::println("enable sync: {}", config.enable_sync);
		else if(property == "cache") 			log::println("enable cache: {}", config.enable_cache);
		else if(property == "prompt") 			log::println("skip prompt: {}", config.skip_prompt);
		else if(property == "logging") 			log::println("enable file logging: {}", config.enable_file_logging);
		else if(property == "clean") 			log::println("clean temporary files: {}", config.clean_temporary);
		else if(property == "verbose") 			log::println("verbose: {}", config.verbose);
	}

	void print_properties(
		const context& config,
		const string_list& properties
	){
		if(properties.empty()){
			_print_property(config, "username");
			_print_property(config, "password");
			_print_property(config, "path");
			_print_property(config, "token");
			_print_property(config, "packages_dir");
			_print_property(config, "tmp_dir");
			_print_property(config, "remote_sources");
			_print_property(config, "jobs");
			_print_property(config, "timeout");
			_print_property(config, "sync");
			_print_property(config, "cache");
			_print_property(config, "prompt");
			_print_property(config, "logging");
			_print_property(config, "clean");
			_print_property(config, "verbose");
		}
		std::for_each(
			properties.begin(),
			properties.end(),
			[&config](const string& property){
				_print_property(config, property);
			}
		);
	}

}