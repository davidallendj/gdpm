
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
	config::config_context config;
	rest_api::asset_list_context params;
	command_e command;
	std::vector<std::string> packages;
	std::vector<std::string> opts;
	bool skip_prompt = false;
	bool clean_tmp_dir = false;
	int priority = -1;


	int initialize(int argc, char **argv){
		// curl_global_init(CURL_GLOBAL_ALL);
		curl = curl_easy_init();
		config = config::make_config();
		params = rest_api::make_context();
		command = none;

		/* Check for config and create if not exists */
		if(!std::filesystem::exists(config.path)){
			config::save(config);
		}
		config = config::load(config.path);
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
		config::save(config);
		// curl_global_cleanup();
	}


	void install_packages(const std::vector<std::string>& package_titles){
		using namespace rapidjson; 		

		/* Check if the package data is already stored in cache. If it is, there 
		is no need to do a lookup to synchronize the local database since we 
		have all the information we need to fetch the asset data. */
		std::vector<package_info> p_found = {};
		std::vector<package_info> p_cache = cache::get_package_info_by_title(package_titles);

		/* Synchronize database information and then try to get data again from
		cache if possible. */
		if(config.enable_sync){
			if(p_cache.empty()){
				log::info("Synchronizing database...");
				p_cache = synchronize_database(package_titles);
				p_cache = cache::get_package_info_by_title(package_titles);
			}
		}

		// FIXME: This does not return the package to be is_installed correctly
		for(const auto& p_title : package_titles){
			auto found = std::find_if(p_cache.begin(), p_cache.end(), [&p_title](const package_info& p){ return p.title == p_title; });
			if(found != p_cache.end()){
				p_found.emplace_back(*found);
			}
		}

		/* Found nothing to install so there's nothing to do at this point. */
		if(p_found.empty()){
			log::error("No packages found to install.");
			return;
		}
		
		log::println("Packages to install: ");
		for(const auto& p : p_found)
			log::print("  {}  ", (p.is_installed) ? p.title + " (reinstall)" : p.title);
		log::println("");
		
		if(!skip_prompt){
			if(!utils::prompt_user_yn("Do you want to install these packages? (y/n)"))
				return;
		}

		using ss_pair = std::pair<std::string, std::string>;
		std::vector<ss_pair> dir_pairs;
		for(auto& p : p_found){
			log::info_n("Fetching asset data for \"{}\"...", p.title);
			std::string url{constants::HostUrl}, package_dir, tmp_dir, tmp_zip;
			url += rest_api::endpoints::GET_AssetId;
			
			/* Retrieve necessary asset data if it was found already in cache */
			Document doc;
			if(p.download_url.empty() || p.category.empty() || p.description.empty() || p.support_level.empty()){
				doc = rest_api::get_asset(url, p.asset_id, config.verbose);
				if(doc.HasParseError() || doc.IsNull()){
					log::error("Could not get a response from server. ({})", doc.GetParseError());
					return;
				}
				p.category			= doc["category"].GetString();
				p.description 		= doc["description"].GetString();
				p.support_level 	= doc["support_level"].GetString();
				p.download_url 		= doc["download_url"].GetString();
				p.download_hash 	= doc["download_hash"].GetString();
			}
			else{
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
			std::ofstream ofs(package_dir + "/package.json");
			OStreamWrapper osw(ofs);
			PrettyWriter<OStreamWrapper> writer(osw);
			doc.Accept(writer);

			/* Check if we already have a stored temporary file before attempting to download */
			if(std::filesystem::exists(tmp_zip) && std::filesystem::is_regular_file(tmp_zip)){
				log::println("Found cached package. Skipping download.", p.title);
			}
			else{
				/* Download all the package files and place them in tmp directory. */
				log::print("Downloading \"{}\"...", p.title);
				std::string download_url = doc["download_url"].GetString();
				std::string title = doc["title"].GetString();
				http::response response = http::download_file(download_url, tmp_zip);
				if(response.code == 200){
					log::println("Done.");
				}else{
					log::error("Something went wrong...(code {})", response.code);
					return;
				}
			}

			dir_pairs.emplace_back(ss_pair(tmp_zip, package_dir + "/"));

			p.is_installed = true;
			p.install_path = package_dir;
		}

		/* Extract all the downloaded packages to their appropriate directory location. */
		for(const auto& p : dir_pairs)
			utils::extract_zip(p.first.c_str(), p.second.c_str());
		
		/* Update the cache data with information from  */ 
		log::info_n("Updating local package database...");
		cache::update_package_info(p_found);
		log::println("Done.");
	}


	void remove_packages(const std::vector<std::string>& package_titles){
		using namespace rapidjson;
		using namespace std::filesystem;

		if(package_titles.empty()){
			log::error("No packages to remove.");
			return;
		}

		/* Find the packages to remove if they're is_installed and show them to the user */
		std::vector<package_info> p_cache = cache::get_package_info_by_title(package_titles);
		if(p_cache.empty()){
			log::error("Could not find any packages to remove.");
			return;
		}

		/* Count number packages in cache flagged as is_installed. If there are none, then there's nothing to do. */
		size_t p_count = 0;
		std::for_each(p_cache.begin(), p_cache.end(), [&p_count](const package_info& p){
				p_count += (p.is_installed) ? 1 : 0;
		});

		if(p_count == 0){
			log::error("No packages to remove.");
			return;
		}
		
		log::println("Packages to remove:");
		for(const auto& p : p_cache)
			if(p.is_installed)
				log::print("  {}  ", p.title);
		log::println("");
		
		if(!skip_prompt){
			if(!utils::prompt_user_yn("Do you want to remove these packages? (y/n)"))
				return;
		}

		log::info_n("Removing packages...");
		for(auto& p : p_cache){
			const path path{config.packages_dir};
			std::filesystem::remove_all(config.packages_dir + "/" + p.title);
			if(config.verbose > 0){
				log::debug("package directory: {}", path.string());
			}

			/* Traverse the package directory */
			for(const auto& entry : recursive_directory_iterator(path)){
				if(entry.is_directory()){
				}
				else if(entry.is_regular_file()){
					std::string filename = entry.path().filename().string();
					std::string pkg_path = entry.path().lexically_normal().string();

					// pkg_path = utils::replace_all(pkg_path, " ", "\\ ");
					if(filename == "package.json"){
						std::string contents = utils::readfile(pkg_path);
						Document doc;
						if(config.verbose > 0){
							log::debug("package path: {}", pkg_path);
							log::debug("contents: \n{}", contents);
						}
						doc.Parse(contents.c_str());
						if(doc.IsNull()){
							log::error("Could not remove packages. Parsing 'package.json' returned NULL.");
							return;
						}
					}
				}
			}
			p.is_installed = false;
		}
		log::println("Done.");
		log::info_n("Updating local package database...");
		cache::update_package_info(p_cache);
		log::println("Done.");
	}


	void update_packages(const std::vector<std::string>& package_titles){
		using namespace rapidjson;
		/* If no package titles provided, update everything and then exit */
		if(package_titles.empty()){

			return;
		}

		/* Fetch remote asset data and compare to see if there are package updates */
		std::vector<package_info> p_updates = {};
		std::vector<package_info> p_cache = cache::get_package_info_by_title(package_titles);

		std::string url{constants::HostUrl};
		url += rest_api::endpoints::GET_Asset;
		Document doc = rest_api::get_assets_list(url, params);

		if(doc.IsNull()){
			log::error("Could not get response from server. Aborting.");
			return;
		}

		for(const auto& p : p_updates){
			for(const auto& o : doc["result"].GetArray()){
				size_t local_version = std::stoul(p.version); 
				std::string remote_version_s = o[""].GetString();
			}
		}
	}


	void search_for_packages(const std::vector<std::string> &package_titles){
		std::vector<package_info> p_cache = cache::get_package_info_by_title(package_titles);

		if(!p_cache.empty()){
			print_package_list(p_cache);
			return;
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
				log::error("Could not search for packages. Are you connected to the internet?");
				return;
			}

			log::info("{} package(s) found...", doc["total_items"].GetInt());
			print_package_list(doc);
		}
	}


	void list_installed_packages(){
		using namespace rapidjson;
		using namespace std::filesystem;		
		const path path{config.packages_dir};
		std::vector<package_info> p_installed = cache::get_installed_packages();
		if(p_installed.empty())
			return;
		log::println("Installed packages:");
		print_package_list(p_installed);
	}

	
	void read_package_contents(const std::string& package_title){

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

		/* Get the storage paths for all packages to create symlinks */
		const path package_dir{config.packages_dir};
		for(const auto& p : p_found){
			for(const auto& path : paths){
				log::info("Creating symlink for \"{}\" package to {}", p.title, path + "/" + p.title);
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
			}
		}
	}


	void clone_packages(const std::vector<std::string>& package_titles, const std::vector<std::string>& paths){
		using namespace std::filesystem;

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
		auto iter = (offset > 0) ? s.begin() + offset : s.end() - offset;
		config.remote_sources.insert(iter, repository);
	}


	void delete_remote_repository(const std::string& repository){
		auto& s = config.remote_sources;

		std::erase(s, repository);
		(void)std::remove_if(s.begin(), s.end(), [&repository](const std::string& rs){
			return repository == rs;
		});
	}


	void delete_remote_repository(size_t index){
		auto& s = config.remote_sources;
		// std::erase(s, index);
	}


	cxxargs parse_arguments(int argc, char **argv){
		/* Parse command-line arguments using cxxopts */
		cxxopts::Options options(
			argv[0], 
			"Package manager made for managing Godot assets."
		);
		options.allow_unrecognised_options();
		options.custom_help("This is a custom help string.");
		options.add_options("Command")
			("input", "", cxxopts::value<std::vector<std::string>>())
			("install", "Install package or packages.", cxxopts::value<std::vector<std::string>>()->implicit_value(""), "<packages...>")
			("remove", "Remove a package or packages.", cxxopts::value<std::vector<std::string>>()->implicit_value(""), "<packages...>")
			("update", "Update a package or packages. This option will update all packages if no argument is supplied.", cxxopts::value<std::vector<std::string>>()->implicit_value(""), "<packages...>")
			("search", "Search for a package or packages.", cxxopts::value<std::vector<std::string>>(), "<packages...>")
			("list", "Show list of is_installed packages.")
			("link", "Create a symlink (or shortcut) to target directory.", cxxopts::value<std::vector<std::string>>(), "<packages...>")
			("clone", "Clone packages into target directory.", cxxopts::value<std::vector<std::string>>(), "<packages...>")
			("clean", "Clean temporary downloaded files.")
			("sync", "Sync local database with remote server.")
			("add-remote", "Set a source repository.", cxxopts::value<std::string>()->default_value(constants::AssetRepo), "<url>")
			("delete-remote", "Remove a source repository from list.", cxxopts::value<std::string>(), "<url>")
			("h,help", "Print this message and exit.")
		;
		options.parse_positional({"input"});
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
			("y,yes", "Bypass yes/no prompt for installing or removing packages.")
			("v,verbose", "Show verbose output.", cxxopts::value<int>()->implicit_value("1")->default_value("0"), "0-5")
		;

		auto result = options.parse(argc, argv);
		return {result, options};
	}


	void handle_arguments(const cxxargs& args){
		auto _get_package_list = [](const cxxopts::ParseResult& result, const char *arg){
			return result[arg].as<std::vector<std::string>>();
		};
		const auto& result = args.result;
		const auto& options = args.options;

		/* Set option variables first to be used in functions below. */
		if(result.count("config")){
			config.path = result["config"].as<std::string>();
			config = config::load(config.path);
			log::info("Config: {}", config.path);
		}
		if(result.count("add-remote")){
			std::string repo = result["remote-add"].as<std::string>();
			config.remote_sources.emplace_back(repo);
		}
		if(result.count("delete-remote")){
			std::string repo = result["remote-add"].as<std::string>();
			auto iter = std::find(config.remote_sources.begin(), config.remote_sources.end(), repo);
			if(iter != config.remote_sources.end())
				config.remote_sources.erase(iter);
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
			rest_api::sort_e sort = rest_api::sort_e::none;
			std::string r = result["sort"].as<std::string>();
			if(r == "none") 		sort = rest_api::sort_e::none;
			else if(r == "rating")	sort = rest_api::sort_e::rating;
			else if(r == "cost")	sort = rest_api::sort_e::cost;
			else if(r == "name")	sort = rest_api::sort_e::name;
			else if(r == "updated") sort = rest_api::sort_e::updated;
			params.sort = sort;
		}
		if(result.count("type")){
			rest_api::type_e type = rest_api::type_e::any;
			std::string r = result["type"].as<std::string>();
			if(r == "any")			type = rest_api::type_e::any;
			else if(r == "addon")	type = rest_api::type_e::addon;
			else if(r == "project")	type = rest_api::type_e::project;
			params.type = type;
		}
		if(result.count("support")){
			rest_api::support_e support = rest_api::support_e::all;
			std::string r = result["support"].as<std::string>();
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

		if(!result.count("input")){
			log::error("Command required. See \"help\" for more information.");
			return;
		}

		std::vector<std::string> argv = result["input"].as<std::vector<std::string>>();
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
		else if (argv[0] == "link" || argv[0] == "--link")		command = link;
		else if(argv[0] == "clone" || argv[0] == "--clone")		command = clone;
		else if(argv[0] == "clean" || argv[0] == "--clean")		command = clean;
		else if(argv[0] == "sync" || argv[0] == "--sync")		command = sync;
		else if(argv[0] == "add-remote" || argv[0] == "--add-remote") command = add_remote;
		else if(argv[0] == "delete-remote" || argv[0] == "--delete-remote") command = delete_remote;
		else if(argv[0] == "help" || argv[0] == "-h" || argv[0] == "--help"){
			log::println("{}", options.help());
		}
	}


	/* Used to run the command AFTER parsing and setting all command line args. */
	void run_command(command_e c, const std::vector<std::string>& package_titles, const std::vector<std::string>& opts){
		switch(c){
			case install: 	install_packages(package_titles); 		break;
			case remove: 	remove_packages(package_titles); 		break;
			case update:	update_packages(package_titles); 		break;
			case search: 	search_for_packages(package_titles); 	break;
			case list: 		list_installed_packages(); 				break;
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
			log::println("{}/{}  {}  id={}\n\t{}, Godot {}, {}, {}, Last Modified: {}",
				p.support_level,
				p.title,
				p.version,
				p.asset_id,
				p.author,
				p.godot_version,
				p.cost,
				p.category,
				p.modify_date
			);
		}
	}


	void print_package_list(const rapidjson::Document& json){
		for(const auto& o : json["result"].GetArray()){
			log::println("{}/{}  {}  id={}\n\t{}, Godot {}, {}, {}, Last Modified: {}",
				o["support_level"]	.GetString(), 
				o["title"]			.GetString(),
				o["version_string"]	.GetString(), 
				o["asset_id"]		.GetString(),
				o["author"]			.GetString(),
				o["godot_version"]	.GetString(),
				o["cost"]			.GetString(),
				o["category"]		.GetString(),
				o["modify_date"]	.GetString()
			);
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

		return cache::get_package_info_by_title(package_titles);
	}

} // namespace gdpm::package_manager