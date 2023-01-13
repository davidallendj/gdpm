
#include "package_manager.hpp"
#include "utils.hpp"
#include "rest_api.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "log.hpp"
#include "http.hpp"
#include "cache.hpp"

#include <algorithm>
#include <curl/curl.h>
#include <curl/easy.h>

#include <filesystem>
#include <regex>
#include <fmt/printf.h>
#include <rapidjson/document.h>
#include <cxxopts.hpp>

#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <system_error>


/*
 * For cURLpp examples...see the link below:
 *
 * https://github.com/jpbarrette/curlpp/tree/master/examples
 */

namespace gdpm::package_manager{
	std::vector<std::string> repo_sources;
	CURL *curl;
	CURLcode res;
	config::context config;
	rest_api::context params;
	command_e command;
	std::vector<std::string> packages;
	std::vector<std::string> opts;
	bool skip_prompt = false;
	bool clean_tmp_dir = false;
	int priority = -1;


	int initialize(int argc, char **argv){
		// curl_global_init(CURL_GLOBAL_ALL);
		curl = curl_easy_init();
		config = config::make_context();
		params = rest_api::make_context();
		command = none;

		/* Check for config and create if not exists */
		if(!std::filesystem::exists(config.path)){
			config::save(config.path, config);
		}
		config::load(config.path, config);
		config.enable_sync = true;
		std::string json = to_json(config);

		/* Create the local databases if it doesn't exist already */
		cache::create_package_database();

		/* Run the rest of the program then exit */
		cxxargs args = parse_arguments(argc, argv);
		handle_arguments(args);
		return 0;
	}


	int execute(){
		run_command(command, packages, opts);
		if(clean_tmp_dir)
			clean_temporary(packages);
		return 0;
	}


	void finalize(){
		curl_easy_cleanup(curl);
		config::save(config.path, config);
		// curl_global_cleanup();
	}


	error install_packages(const std::vector<std::string>& package_titles, bool skip_prompt){
		using namespace rapidjson;
		params.verbose = config.verbose;
		error error;
		

		/* TODO: Need a way to use remote sources from config until none left */

		/* Check if the package data is already stored in cache. If it is, there 
		is no need to do a lookup to synchronize the local database since we 
		have all the information we need to fetch the asset data. */
		std::vector<package_info> p_found = {};
		std::vector<package_info> p_cache = cache::get_package_info_by_title(package_titles);

		/* Synchronize database information and then try to get data again from
		cache if possible. */
		if(config.enable_sync){
			if(p_cache.empty()){
				p_cache = synchronize_database(package_titles);
			}
		}

		for(const auto& p_title : package_titles){
			auto found = std::find_if(p_cache.begin(), p_cache.end(), [&p_title](const package_info& p){ return p.title == p_title; });
			if(found != p_cache.end()){
				p_found.emplace_back(*found);
			}
		}

		/* Found nothing to install so there's nothing to do at this point. */
		if(p_found.empty()){
			log::error("No packages found to install.");
			error.set_code(-1);
			error.set_message("No packages found to install.");
			return error;
		}
		
		log::println("Packages to install: ");
		for(const auto& p : p_found){
			std::string output((p.is_installed) ? p.title + " (reinstall)" : p.title);
			log::print("  {}  ", (p.is_installed) ? p.title + " (reinstall)" : p.title);
		}
		log::println("");
		
		if(!skip_prompt){
			if(!utils::prompt_user_yn("Do you want to install these packages? (y/n)"))
				return error;
		}

		using ss_pair = std::pair<std::string, std::string>;
		std::vector<ss_pair> dir_pairs;
		for(auto& p : p_found){
			log::info("Fetching asset data for \"{}\"...", p.title);

			/* TODO: Try fetching the data with all available remote sources until retrieved */
			for(const auto& remote_url : config.remote_sources){
				std::string url{remote_url}, package_dir, tmp_dir, tmp_zip;

				url += rest_api::endpoints::GET_AssetId;
			
				/* Retrieve necessary asset data if it was found already in cache */
				Document doc;
				// log::debug("download_url: {}\ncategory: {}\ndescription: {}\nsupport_level: {}", p.download_url, p.category, p.description, p.support_level);
				if(p.download_url.empty() || p.category.empty() || p.description.empty() || p.support_level.empty()){
					params.verbose = config.verbose;
					doc = rest_api::get_asset(url, p.asset_id, params);
					if(doc.HasParseError() || doc.IsNull()){
						log::println("");
						log::error("Error parsing HTTP response. (error code: {})", doc.GetParseError());
						error.set_code(doc.GetParseError());
						return error;
					}
					p.category			= doc["category"].GetString();
					p.description 		= doc["description"].GetString();
					p.support_level 	= doc["support_level"].GetString();
					p.download_url 		= doc["download_url"].GetString();
					p.download_hash 	= doc["download_hash"].GetString();
				}
				else{ 
					/* Package for in cache so no remote request. Still need to populate RapidJson::Document to write to package.json.
					NOTE: This may not be necessary at all! 
					*/
					// doc["asset_id"].SetUint64(p.asset_id
					// doc["type"].SetString(p.type, doc.GetAllocator());
					// doc["title"].SetString(p.title, doc.GetAllocator());
					// doc["author"].SetString(p.author, doc.GetAllocator());
					// doc["author_id"].SetUint64(p.author_id);
					// doc["version"].SetString(p.version, doc.GetAllocator());
					// doc["category"].SetString(p.category, doc.GetAllocator());
					// doc["godot_version"].SetString(p.godot_version, doc.GetAllocator());
					// doc["cost"].SetString(p.cost, doc.GetAllocator());
					// doc["description"].SetString(p.description, doc.GetAllocator());
					// doc["support_level"].SetString(p.support_level, doc.GetAllocator());
					// doc["download_url"].SetString(p.download_url, doc.GetAllocator());
					// doc["download_hash"].SetString(p.download_hash, doc.GetAllocator;
				}

				/* Set directory and temp paths for storage */
				package_dir = config.packages_dir + "/" + p.title;
				tmp_dir = config.tmp_dir + "/" + p.title;
				tmp_zip = tmp_dir + ".zip";

				/* Make directories for packages if they don't exist to keep everything organized */
				if(!std::filesystem::exists(config.tmp_dir))
					std::filesystem::create_directories(config.tmp_dir);
				if(!std::filesystem::exists(config.packages_dir))
					std::filesystem::create_directories(config.packages_dir);
				
				/* Dump asset information for lookup into JSON in package directory */
				if(!std::filesystem::exists(package_dir))
					std::filesystem::create_directory(package_dir);
				
				std::ofstream ofs(package_dir + "/asset.json");
				OStreamWrapper osw(ofs);
				PrettyWriter<OStreamWrapper> writer(osw);
				doc.Accept(writer);

				/* Check if we already have a stored temporary file before attempting to download */
				if(std::filesystem::exists(tmp_zip) && std::filesystem::is_regular_file(tmp_zip)){
					log::println("Found cached package. Skipping download.", p.title);
				}
				else{
					/* Download all the package files and place them in tmp directory. */
					log::info_n("Downloading \"{}\"...", p.title);
					std::string download_url = p.download_url;// doc["download_url"].GetString();
					std::string title = p.title;// doc["title"].GetString();
					http::response response = http::download_file(download_url, tmp_zip);
					if(response.code == 200){
						log::println("Done.");
					}else{
						log::error("Something went wrong...(code {})", response.code);
						error.set_code(response.code);
						error.set_message("Error in HTTP response.");
						return error;
					}
				}

				dir_pairs.emplace_back(ss_pair(tmp_zip, package_dir + "/"));

				p.is_installed = true;
				p.install_path = package_dir;
			}
		}

		/* Extract all the downloaded packages to their appropriate directory location. */
		for(const auto& p : dir_pairs)
			utils::extract_zip(p.first.c_str(), p.second.c_str());
		
		/* Update the cache data with information from  */ 
		log::info_n("Updating local asset data...");
		cache::update_package_info(p_found);
		log::println("done.");

		return error;
	}


	error remove_packages(const std::vector<std::string>& package_titles, bool skip_prompt){
		using namespace rapidjson;
		using namespace std::filesystem;

		error error;
		if(package_titles.empty()){
			std::string message("No packages to remove.");
			log::println("");
			log::error(message);
			error.set_code(-1);
			error.set_message(message);
			return error;
		}

		/* Find the packages to remove if they're is_installed and show them to the user */
		std::vector<package_info> p_cache = cache::get_package_info_by_title(package_titles);
		if(p_cache.empty()){
			std::string message("Could not find any packages to remove.");
			log::println("");
			log::error(message);
			error.set_code(-1);
			error.set_message(message);
			return error;
		}

		/* Count number packages in cache flagged as is_installed. If there are none, then there's nothing to do. */
		size_t p_count = 0;
		std::for_each(p_cache.begin(), p_cache.end(), [&p_count](const package_info& p){
				p_count += (p.is_installed) ? 1 : 0;
		});

		if(p_count == 0){
			std::string message("No packages to remove.");
			log::println("");
			log::error(message);
			error.set_code(-1);
			error.set_message(message);
			return error;
		}
		
		log::println("Packages to remove:");
		for(const auto& p : p_cache)
			if(p.is_installed)
				log::print("  {}  ", p.title);
		log::println("");
		
		if(!skip_prompt){
			if(!utils::prompt_user_yn("Do you want to remove these packages? (y/n)"))
				return error;
		}

		log::info_n("Removing packages...");
		for(auto& p : p_cache){
			const path path{config.packages_dir};
			std::filesystem::remove_all(config.packages_dir + "/" + p.title);
			if(config.verbose > 0){
				log::debug("package directory: {}", path.string());
			}

			/* Traverse the package directory */
			// for(const auto& entry : recursive_directory_iterator(path)){
			// 	if(entry.is_directory()){
			// 	}
			// 	else if(entry.is_regular_file()){
			// 		std::string filename = entry.path().filename().string();
			// 		std::string pkg_path = entry.path().lexically_normal().string();

			// 		// pkg_path = utils::replace_all(pkg_path, " ", "\\ ");
			// 		if(filename == "package.json"){
			// 			std::string contents = utils::readfile(pkg_path);
			// 			Document doc;
			// 			if(config.verbose > 0){
			// 				log::debug("package path: {}", pkg_path);
			// 				log::debug("contents: \n{}", contents);
			// 			}
			// 			doc.Parse(contents.c_str());
			// 			if(doc.IsNull()){
			// 				log::println("");
			// 				log::error("Could not remove packages. Parsing 'package.json' returned NULL.");
			// 				return;
			// 			}
			// 		}
			// 	}
			// }
			p.is_installed = false;
		}
		log::println("Done.");
		log::info_n("Updating local asset data...");
		cache::update_package_info(p_cache);
		log::println("done.");

		return error;
	}


	error update_packages(const std::vector<std::string>& package_titles, bool skip_prompt){
		using namespace rapidjson;
		error error;

		/* If no package titles provided, update everything and then exit */
		if(package_titles.empty()){
			std::string url{constants::HostUrl};
			url += rest_api::endpoints::GET_AssetId;
			Document doc = rest_api::get_assets_list(url, params);
			if(doc.IsNull()){
				std::string message("Could not get response from server. Aborting.");
				log::error(message);
				error.set_code(-1);
				error.set_message(message);
				return error;
			}
			return error;
		}

		/* Fetch remote asset data and compare to see if there are package updates */
		std::vector<std::string> p_updates = {};
		std::vector<package_info> p_cache = cache::get_package_info_by_title(package_titles);

		log::println("Packages to update: ");
		for(const auto& p_title : p_updates)
			log::print("  {}  ", p_title);
		log::println("");

		/* Check version information to see if packages need updates */
		for(const auto& p : p_cache){
			std::string url{constants::HostUrl};
			url += rest_api::endpoints::GET_AssetId;
			Document doc = rest_api::get_asset(url, p.asset_id);
			std::string remote_version = doc["version"].GetString();
			if(p.version != remote_version){
				p_updates.emplace_back(p.title);
			}
		}

		if(!skip_prompt){
			if(!utils::prompt_user_yn("Do you want to update the following packages? (y/n)"))
				return error;
		}

		remove_packages(p_updates);
		install_packages(p_updates);
		return error;
	}


	error search_for_packages(const std::vector<std::string> &package_titles, bool skip_prompt){
		std::vector<package_info> p_cache = cache::get_package_info_by_title(package_titles);
		error error;
		
		if(!p_cache.empty() && !config.enable_sync){
			print_package_list(p_cache);
			return error;
		}
		for(const auto& p_title : package_titles){
			using namespace rapidjson;
		
			params.filter = curl_easy_escape(curl, p_title.c_str(), p_title.size());
			params.verbose = config.verbose;
			params.godot_version = config.godot_version;
			params.max_results = 200;

			std::string request_url{constants::HostUrl};
			request_url += rest_api::endpoints::GET_Asset;
			Document doc = rest_api::get_assets_list(request_url, params);
			if(doc.IsNull()){
				std::string message("Could not search for packages.");
				log::error(message);
				error.set_code(-1);
				error.set_message(message);
				return error;
			}

			log::info("{} package(s) found...", doc["total_items"].GetInt());
			print_package_list(doc);
		}
		return error;
	}


	void list_information(const std::vector<std::string>& opts){
		using namespace rapidjson;
		using namespace std::filesystem;

		if(opts.empty() || opts[0] == "packages"){
			const path path{config.packages_dir};
			std::vector<package_info> p_installed = cache::get_installed_packages();
			if(p_installed.empty())
				return;
			log::println("Installed packages:");
			print_package_list(p_installed);
		}
		else if(opts[0] == "remote-sources"){
			print_remote_sources();
		}
		else{
			log::error("Unrecognized subcommand. Use either 'packages' or 'remote-sources' instead.");
			return;
		}
	}


	void clean_temporary(const std::vector<std::string>& package_titles){
		if(package_titles.empty()){
			log::info("Cleaned all temporary files.");
			std::filesystem::remove_all(config.tmp_dir);
		}
		/* Find the path of each packages is_installed then delete temporaries */
		log::info_n("Cleaning temporary files...");
		for(const auto& p_title : package_titles){
			std::string tmp_zip = config.tmp_dir + "/" + p_title + ".zip";
			if(config.verbose > 0)
				log::info("Removed '{}'", tmp_zip);
			std::filesystem::remove_all(tmp_zip);
		}
		log::println("Done.");
	}


	void link_packages(const std::vector<std::string>& package_titles, const std::vector<std::string>& paths){
		using namespace std::filesystem;

		if(paths.empty()){
			log::error("No path set. Use '--path' option to set a path.");
			return;
		}

		std::vector<package_info> p_found = {};
		std::vector<package_info> p_cache = cache::get_package_info_by_title(package_titles);
		if(p_cache.empty()){
			log::error("Could not find any packages to link.");
			return;
		}

		for(const auto& p_title : package_titles){
			auto found = std::find_if(p_cache.begin(), p_cache.end(), [&p_title](const package_info& p){ return p.title == p_title; });
			if(found != p_cache.end()){
				p_found.emplace_back(*found);
			}
		}

		if(p_found.empty()){
			log::error("No packages found to link.");
			return;
		}

		/* Get the storage paths for all packages to create symlinks */
		const path package_dir{config.packages_dir};
		for(const auto& p : p_found){
			for(const auto& path : paths){
				log::info_n("Creating symlink for \"{}\" package to '{}'...", p.title, path + "/" + p.title);
				// std::filesystem::path target{config.packages_dir + "/" + p.title};
				std::filesystem::path target = {current_path().string() + "/" + config.packages_dir + "/" + p.title};
				std::filesystem::path symlink_path{path + "/" + p.title};
				if(!std::filesystem::exists(symlink_path.string()))
					std::filesystem::create_directories(path + "/");
				std::error_code ec;
				std::filesystem::create_directory_symlink(target, symlink_path, ec);
				if(ec){
					log::error("Could not create symlink: {}", ec.message());
				}
				log::println("Done.");
			}
		}
	}


	void clone_packages(const std::vector<std::string>& package_titles, const std::vector<std::string>& paths){
		using namespace std::filesystem;

		if(paths.empty()){
			log::error("No path set. Use '--path' option to set a path.");
			return;
		}

		std::vector<package_info> p_found = {};
		std::vector<package_info> p_cache = cache::get_package_info_by_title(package_titles);
		if(p_cache.empty()){
			log::error("Could not find any packages to clone.");
			return;
		}

		for(const auto& p_title : package_titles){
			auto found = std::find_if(p_cache.begin(), p_cache.end(), [&p_title](const package_info& p){ return p.title == p_title; });
			if(found != p_cache.end()){
				p_found.emplace_back(*found);
			}
		}

		if(p_found.empty()){
			log::error("No packages to clone.");
			return;
		}

		/* Get the storage paths for all packages to create clones */
		const path package_dir{config.packages_dir};
		for(const auto& p : p_found){
			for(const auto& path : paths){
				log::info("Cloning \"{}\" package to {}", p.title, path + "/" + p.title);
				std::filesystem::path from{config.packages_dir + "/" + p.title};
				std::filesystem::path to{path + "/" + p.title};
				if(!std::filesystem::exists(to.string()))
					std::filesystem::create_directories(to);

				/* TODO: Add an option to force overwriting (i.e. --overwrite) */
				std::filesystem::copy(from, to, copy_options::update_existing | copy_options::recursive);
			}
		}
	}


	void add_remote_repository(const std::string& repository, ssize_t offset){
		auto& s = config.remote_sources;
		// auto iter = (offset > 0) ? s.begin() + offset : s.end() - offset;
		// config.remote_sources.insert(iter, repository);
		config.remote_sources.insert(repository);
	}


	void delete_remote_repository(const std::string& repository){
		auto& s = config.remote_sources;
		s.erase(repository);
		// std::erase(s, repository);
		// (void)std::remove_if(s.begin(), s.end(), [&repository](const std::string& rs){
		// 	return repository == rs;
		// });
	}


	/* TODO: Need to finish implementation...will do that when it's needed. */
	void delete_remote_repository(size_t index){
		auto& s = config.remote_sources;
		// std::erase(s, index);
	}


	std::vector<std::string> resolve_dependencies(const std::vector<std::string>& package_titles){
		std::vector<std::string> p_deps = {};
		std::vector<package_info> p_cache = cache::get_package_info_by_title(package_titles);

		/* Build an graph of every thing to check then install in order */
		for(const auto& p : p_cache){
			if(p.dependencies.empty())
				continue;
			
			/* Check if dependency has a dependency. If so, resolve those first. */
			for(const auto& d : p.dependencies){
				auto temp = resolve_dependencies({d.title});
				utils::move_if_not(temp, p_deps, [](const std::string& p){ return true; });
			}
		}

		return p_deps;
	}


	cxxargs parse_arguments(int argc, char **argv){
		/* Parse command-line arguments using cxxopts */
		cxxopts::Options options(
			argv[0], 
			"Experimental package manager made for managing assets for the Godot game engine through the command-line.\n"
		);
		options.allow_unrecognised_options();
		options.custom_help("[COMMAND] [OPTIONS...]");
		options.add_options("Command")
			("command", "Specify the input parameters", cxxopts::value<std::vector<std::string>>())
			("install", "Install package or packages.", cxxopts::value<std::vector<std::string>>()->implicit_value(""), "<packages...>")
			("remove", "Remove a package or packages.", cxxopts::value<std::vector<std::string>>()->implicit_value(""), "<packages...>")
			("update", "Update a package or packages. This will update all packages if no argument is provided.", cxxopts::value<std::vector<std::string>>()->implicit_value(""), "<packages...>")
			("search", "Search for a package or packages.", cxxopts::value<std::vector<std::string>>(), "<packages...>")
			("list", "Show list of installed packages.")
			("link", "Create a symlink (or shortcut) to target directory. Must be used with the `--path` argument.", cxxopts::value<std::vector<std::string>>(), "<packages...>")
			("clone", "Clone packages into target directory. Must be used with the `--path` argument.", cxxopts::value<std::vector<std::string>>(), "<packages...>")
			("clean", "Clean temporary downloaded files.")
			("fetch", "Fetch asset data from remote sources.")
			("add-remote", "Set a source repository.", cxxopts::value<std::string>()->default_value(constants::AssetRepo), "<url>")
			("delete-remote", "Remove a source repository from list.", cxxopts::value<std::string>(), "<url>")
			("quick-remote", "One time remote source use. Source is not saved and override sources used in config.", cxxopts::value<std::vector<std::string>>(), "<url>")
			("h,help", "Print this message and exit.")
			("version", "Show the current version and exit.")
		;
		options.parse_positional({"command"});
		options.positional_help("");
		options.add_options("Options")
			("c,config", "Set the config file path.", cxxopts::value<std::string>())
			("f,file", "Read file to install or remove packages.", cxxopts::value<std::string>(), "<path>")
			("path", "Specify a path to use with a command", cxxopts::value<std::vector<std::string>>())
			("type", "Set package type (any|addon|project).", cxxopts::value<std::string>())
			("sort", "Sort packages in order (rating|cost|name|updated).", cxxopts::value<std::string>())
			("support", "Set the support level for API (all|official|community|testing).")
			("max-results", "Set the max results to return from search.", cxxopts::value<int>()->default_value("500"), "<int>")
			("godot-version", "Set the Godot version to include in request.", cxxopts::value<std::string>())
			("set-priority", "Set the priority for remote source. Lower values are used first (0...100).", cxxopts::value<int>())
			("set-packages-directory", "Set the local package storage location.", cxxopts::value<std::string>())
			("set-temporary-directory", "Set the local temporary storage location.", cxxopts::value<std::string>())
			("timeout", "Set the amount of time to wait for a response.", cxxopts::value<size_t>())
			("no-sync", "Disable synchronizing with remote.", cxxopts::value<bool>()->implicit_value("true")->default_value("false"))
			("y,no-prompt", "Bypass yes/no prompt for installing or removing packages.")
			("v,verbose", "Show verbose output.", cxxopts::value<int>()->implicit_value("1")->default_value("0"), "0-5")
		;

		auto result = options.parse(argc, argv);
		return {result, options};
	}


	void handle_arguments(const cxxargs& args){
		const auto& result = args.result;
		const auto& options = args.options;

		/* Set option variables first to be used in functions below. */
		if(result.count("config")){
			config.path = result["config"].as<std::string>();
			config::load(config.path, config);
			log::info("Config: {}", config.path);
		}
		if(result.count("add-remote")){
			std::string repo = result["remote-add"].as<std::string>();
			// config.remote_sources.emplace_back(repo);
			config.remote_sources.insert(repo);
		}
		if(result.count("delete-remote")){
			std::string repo = result["remote-add"].as<std::string>();
			auto iter = std::find(config.remote_sources.begin(), config.remote_sources.end(), repo);
			if(iter != config.remote_sources.end())
				config.remote_sources.erase(iter);
		}
		if(result.count("remote")){

		}
		if(result.count("file")){
			std::string path = result["file"].as<std::string>();
			std::string contents = utils::readfile(path);
			packages = utils::parse_lines(contents);
		}
		if(result.count("path")){
			opts = result["path"].as<std::vector<std::string>>();
		}
		if(result.count("sort")){
			std::string r = result["sort"].as<std::string>();
			rest_api::sort_e 		sort = rest_api::sort_e::none;
			if(r == "none") 		sort = rest_api::sort_e::none;
			else if(r == "rating")	sort = rest_api::sort_e::rating;
			else if(r == "cost")	sort = rest_api::sort_e::cost;
			else if(r == "name")	sort = rest_api::sort_e::name;
			else if(r == "updated") sort = rest_api::sort_e::updated;
			params.sort = sort;
		}
		if(result.count("type")){
			std::string r = result["type"].as<std::string>();
			rest_api::type_e 		type = rest_api::type_e::any;
			if(r == "any")			type = rest_api::type_e::any;
			else if(r == "addon")	type = rest_api::type_e::addon;
			else if(r == "project")	type = rest_api::type_e::project;
			params.type = type;
		}
		if(result.count("support")){
			std::string r = result["support"].as<std::string>();
			rest_api::support_e 		support = rest_api::support_e::all;
			if(r == "all")				support = rest_api::support_e::all;
			else if(r == "official") 	support = rest_api::support_e::official;
			else if(r == "community") 	support = rest_api::support_e::community;
			else if(r == "testing") 	support = rest_api::support_e::testing;
			params.support = support;
		}
		if(result.count("max-results")){
			params.max_results = result["max-results"].as<int>();
		}
		if(result.count("godot-version")){
			config.godot_version = result["godot-version"].as<std::string>();
		}
		if(result.count("timeout")){
			config.timeout = result["timeout"].as<size_t>();
		}
		if(result.count("no-sync")){
			config.enable_sync = false;
		}
		if(result.count("set-priority")){
			priority = result["set-priority"].as<int>();
		}
		if(result.count("set-packages-directory")){
			config.packages_dir = result["set-packages-directory"].as<std::string>();
		}
		if(result.count("set-temporary-directory")){
			config.tmp_dir = result["set-temporary-directory"].as<std::string>();
		}
		if(result.count("yes")){
			skip_prompt = true;
		}
		if(result.count("link")){
			packages = result["link"].as<std::vector<std::string>>();
		}
		if(result.count("clone")){
			packages = result["clone"].as<std::vector<std::string>>();
		}
		if(result.count("clean")){
			clean_tmp_dir = true;
		}
		config.verbose = 0;
		config.verbose += result["verbose"].as<int>();
		std::string json = to_json(config);
		if(config.verbose > 0){
			log::println("Verbose set to level {}", config.verbose);
			log::println("{}", json);
		}

		if(!result.count("command")){
			log::error("Command required. See \"help\" for more information.");
			return;
		}

		std::vector<std::string> argv = result["command"].as<std::vector<std::string>>();
		std::vector<std::string> opts{argv.begin()+1, argv.end()};
		if(packages.empty() && opts.size() > 0){
			for(const auto& opt : opts){
				packages.emplace_back(opt);
			}
		}

		if(argv[0] == "install" || argv[0] == "--install") 		command = install;
		else if (argv[0] == "remove" || argv[0] == "--remove") 	command = remove;
		else if(argv[0] == "update" || argv[0] == "--update") 	command = update;
		else if(argv[0] == "search" || argv[0] == "--search") 	command = search;
		else if(argv[0] == "list" || argv[0] == "-ls")			command = list;
		else if(argv[0] == "link" || argv[0] == "--link")		command = link;
		else if(argv[0] == "clone" || argv[0] == "--clone")		command = clone;
		else if(argv[0] == "clean" || argv[0] == "--clean")		command = clean;
		else if(argv[0] == "sync" || argv[0] == "--sync")		command = sync;
		else if(argv[0] == "add-remote" || argv[0] == "--add-remote") command = add_remote;
		else if(argv[0] == "delete-remote" || argv[0] == "--delete-remote") command = delete_remote;
		else if(argv[0] == "help" || argv[0] == "-h" || argv[0] == "--help"){
			log::println("{}", options.help());
		}
		else{
			log::error("Unrecognized command. Try 'gdpm help' for more info.");
		}
	}


	/* Used to run the command AFTER parsing and setting all command line args. */
	void run_command(command_e c, const std::vector<std::string>& package_titles, const std::vector<std::string>& opts){
		switch(c){
			case install: 	install_packages(package_titles, skip_prompt); 		break;
			case remove: 	remove_packages(package_titles, skip_prompt); 		break;
			case update:	update_packages(package_titles, skip_prompt); 		break;
			case search: 	search_for_packages(package_titles, skip_prompt); 	break;
			case list: 		list_information(package_titles); 		break;
							/* ...opts are the paths here */
			case link:		link_packages(package_titles, opts);	break;
			case clone:		clone_packages(package_titles, opts);	break;
			case clean:		clean_temporary(package_titles);		break;
			case sync: 		synchronize_database(package_titles);	break;
			case add_remote: add_remote_repository(opts[0], priority);	break;
			case delete_remote: delete_remote_repository(opts[0]); 	break;
			case help: 		/* ...runs in handle_arguments() */		break;
			case none:		/* ...here to run with no command */	break;
		}
	}


	void print_package_list(const std::vector<package_info>& packages){
		for(const auto& p : packages){
			log::println("{}/{}/{}  {}  id={}\n\tGodot {}, {}, {}, Last Modified: {}",
				p.support_level,
				p.author,
				p.title,
				p.version,
				p.asset_id,
				p.godot_version,
				p.cost,
				p.category,
				p.modify_date
			);
		}
	}


	void print_package_list(const rapidjson::Document& json){
		for(const auto& o : json["result"].GetArray()){
			log::println("{}/{}/{}  {}  id={}\n\tGodot {}, {}, {}, Last Modified: {}",
				o["support_level"]	.GetString(), 
				o["author"]			.GetString(),
				o["title"]			.GetString(),
				o["version_string"]	.GetString(), 
				o["asset_id"]		.GetString(),
				o["godot_version"]	.GetString(),
				o["cost"]			.GetString(),
				o["category"]		.GetString(),
				o["modify_date"]	.GetString()
			);
		}
	}

	
	void print_remote_sources(){
		log::println("Remote sources:");
		for(const auto& s : config.remote_sources){
			log::println("\t{}", s);
		}
	}


	std::vector<package_info> synchronize_database(const std::vector<std::string>& package_titles){
		using namespace rapidjson;
		params.verbose = config.verbose;
		params.godot_version = config.godot_version;
		params.page = 0;
		int page = 0;
		int page_length = 0;
		// int total_pages = 0;
		int total_items = 0;
		int items_left = 0;

		log::info("Sychronizing database...");
		do{
			/* Make the GET request to get page data and store it in the local 
			package database. Also, check to see if we need to keep going. */
			std::string url{constants::HostUrl};
			url += rest_api::endpoints::GET_Asset;
			Document doc = rest_api::get_assets_list(url, params);
			params.page += 1;

			if(doc.IsNull()){
				log::error("Could not get response from server. Aborting.");
				return {};
			}

			/* Need to know how many pages left to get and how many we get per 
			request. */
			page 		= doc["page"].GetInt();
			page_length = doc["page_length"].GetInt();
			// total_pages = doc["pages"].GetInt();
			total_items = doc["total_items"].GetInt();
			items_left 	= total_items - (page + 1) * page_length;

			// log::info("page: {}, page length: {}, total pages: {}, total items: {}, items left: {}", page, page_length, total_pages, total_items, items_left);

			if(page == 0){
				cache::drop_package_database();
				cache::create_package_database();
			}

			std::vector<package_info> packages;
			for(const auto& o : doc["result"].GetArray()){
				// log::println("=======================");
				package_info p{
					.asset_id 		= std::stoul(o["asset_id"].GetString()),
					.title 			= o["title"].GetString(),
					.author 		= o["author"].GetString(),
					.author_id 		= std::stoul(o["author_id"].GetString()),
					.version 		= o["version"].GetString(),
					.godot_version 	= o["godot_version"].GetString(),
					.cost 			= o["cost"].GetString(),
					.modify_date 	= o["modify_date"].GetString(),
					.category		= o["category"].GetString(),
					.remote_source	= url
				};
				packages.emplace_back(p);
			}
			cache::insert_package_info(packages);

			/* Make the same request again to get the rest of the needed data 
			using the same request, but with a different page, then update 
			variables as needed. */


		} while(items_left > 0);

		log::println("Done.");

		return cache::get_package_info_by_title(package_titles);
	}

} // namespace gdpm::package_manager


