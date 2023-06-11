
#include "package.hpp"
#include "error.hpp"
#include "log.hpp"
#include "rest_api.hpp"
#include "config.hpp"
#include "cache.hpp"
#include "http.hpp"
#include "remote.hpp"
#include "types.hpp"
#include <future>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>

namespace gdpm::package{
	
	error install(
		const config::context& config,
		const package::title_list& package_titles, 
		const package::params& params
	){
		using namespace rapidjson;

		/* TODO: Need a way to use remote sources from config until none left */

		/*
		Implementation steps:

		1. Synchronize the cache information by making a request using remote API.
		(can skip with `--no-sync` options)
		2. Check if the package is installed. If it is, make sure it is latest version.
		If not, download and update to the latest version.
		3. Extract package contents and copy/move to the correct install location.
		*/

		result_t result = cache::get_package_info_by_title(package_titles);
		package::info_list p_found = {};
		package::info_list p_cache = result.unwrap_unsafe();

		/* Synchronize database information and then try to get data again from
		cache if possible. */
		if(config.enable_sync){
			if(p_cache.empty()){
				result_t result = synchronize_database(config, package_titles);
				p_cache = result.unwrap_unsafe();
			}
		}

		/* Match queried package titles with those found in cache. */
		log::debug("Searching for packages in cache...");
		for(const auto& p_title : package_titles){
			auto found = std::find_if(
				p_cache.begin(), 
				p_cache.end(), 
				[&p_title](const package::info& p){ 
					return p.title == p_title; 
				}
			);
			if(found != p_cache.end()){
				p_found.emplace_back(*found);
			}
		}

		/* If size of package_titles == p_found, then all packages can be installed
		from cache and there's no need to query remote API. However, this will
		only installed the latest *local* version and will not sync with remote. */
		if(p_found.size() == package_titles.size()){
			log::info("Found all packages stored in local cache.");
		}

		/* Found nothing to install so there's nothing to do at this point. */
		if(p_found.empty()){
			error error(
				constants::error::NOT_FOUND,
				"No packages found to install."
			);
			log::error(error);
			return error;
		}
		
		log::println("Packages to install: ");
		for(const auto& p : p_found){
			string output((p.is_installed) ? p.title + " (reinstall)" : p.title);
			log::print("  {}  ", (p.is_installed) ? p.title + " (reinstall)" : p.title);
		}
		log::println("");
		
		if(!config.skip_prompt){
			if(!utils::prompt_user_yn("Do you want to install these packages? (y/n)"))
				return error();
		}

		/* Check if provided param is in remote sources*/
		if(!config.remote_sources.contains(params.remote_source)){
			error error(
				constants::error::NOT_FOUND,
				"Remote resource not found in config."
			);
			log::error(error);
			return error;
		}

		/* Try and obtain all requested packages. */
		std::vector<string_pair> dir_pairs;
		task_list tasks;
		rest_api::request_params rest_api_params = rest_api::make_from_config(config);
		for(auto& p : p_found){	// TODO: Execute each in parallel using coroutines??

			/* Check if a remote source was provided. If not, then try to get packages
			in global storage location only. */

			log::info("Fetching asset data for \"{}\"...", p.title);
			string url{config.remote_sources.at(params.remote_source) + rest_api::endpoints::GET_AssetId};
			string package_dir, tmp_dir, tmp_zip;

			/* Retrieve necessary asset data if it was found already in cache */
			Document doc;
			bool is_valid = p.download_url.empty() || p.category.empty() || p.description.empty() || p.support_level.empty();
			if(is_valid){
				doc = rest_api::get_asset(url, p.asset_id, rest_api_params);
				if(doc.HasParseError() || doc.IsNull()){
					constexpr const char *message = "\nError parsing HTTP response.";
					log::error(message);
					return error(doc.GetParseError(), message);
				}
				p.category			= doc["category"].GetString();
				p.description 		= doc["description"].GetString();
				p.support_level 	= doc["support_level"].GetString();
				p.download_url 		= doc["download_url"].GetString();
				p.download_hash 	= doc["download_hash"].GetString();
			}
			else{
				log::error("Not a valid package.");
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
				log::info("Found cached package. Skipping download.", p.title);
			}
			else{
				/* Download all the package files and place them in tmp directory. */
				log::info_n("Downloading \"{}\"...", p.title);
				std::string download_url = p.download_url;// doc["download_url"].GetString();
				std::string title = p.title;// doc["title"].GetString();
				http::response response = http::download_file(download_url, tmp_zip);
				if(response.code == http::OK){
					log::println("Done.");
				}else{
					error error(
						constants::error::HTTP_RESPONSE_ERROR,
						std::format("HTTP Error: {}", response.code)
					);
					log::error(error);
					return error;
				}
			}

			dir_pairs.emplace_back(string_pair(tmp_zip, package_dir + "/"));

			p.is_installed = true;
			p.install_path = package_dir;

			/* Extract all the downloaded packages to their appropriate directory location. */
			for(const auto& p : dir_pairs)
				utils::extract_zip(p.first.c_str(), p.second.c_str());

			/* Update the cache data with information from  */
			log::info_n("Updating local asset data...");
			cache::update_package_info(p_found);
			log::println("done.");
				// })
			// );
		}

		return error();
	}


	error add(
		const config::context& config,
		const title_list& package_titles,
		const params& params
	){

		return error();
	}


	error remove(
		const config::context& config,
		const string_list& package_titles, 
		const package::params& params
	){
		using namespace rapidjson;
		using namespace std::filesystem;

		/* Find the packages to remove if they're is_installed and show them to the user */
		result_t result = cache::get_package_info_by_title(package_titles);
		std::vector<package::info> p_cache = result.unwrap_unsafe();
		if(p_cache.empty()){
			error error(
				constants::error::NOT_FOUND,
				"Could not find any packages to remove."
			);
			log::error(error);
			return error;
		}

		/* Count number packages in cache flagged as is_installed. If there are none, then there's nothing to do. */
		size_t p_count = 0;
		std::for_each(p_cache.begin(), p_cache.end(), [&p_count](const package::info& p){
				p_count += (p.is_installed) ? 1 : 0;
		});

		if(p_count == 0){
			error error(
				constants::error::NOT_FOUND,
				"\nNo packages to remove."
			);
			log::error(error);
			return error;
		}
		
		log::println("Packages to remove:");
		for(const auto& p : p_cache)
			if(p.is_installed)
				log::print("  {}  ", p.title);
		log::println("");
		
		if(!config.skip_prompt){
			if(!utils::prompt_user_yn("Do you want to remove these packages? (y/n)"))
				return error();
		}

		log::info_n("Removing packages...");
		for(auto& p : p_cache){
			const std::filesystem::path path{config.packages_dir};
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

		return error();
	}


	/** 
	Removes all local packages.
	 */
	error remove_all(
		const config::context& config,
		const package::params& params
	){
		/* Get the list of all packages to remove then remove */
		result_t r_installed = cache::get_installed_packages();
		package::info_list p_installed = r_installed.unwrap_unsafe();
		result_t r_titles = get_package_titles(p_installed);
		package::title_list p_titles = r_titles.unwrap_unsafe();
		return remove(config, p_titles, params);
	}


	error update(
		const config::context& config,
		const package::title_list& package_titles, 
		const package::params& params
	){
		using namespace rapidjson;

		/* If no package titles provided, update everything and then exit */
		rest_api::request_params rest_api_params = rest_api::make_from_config(config);
		if(package_titles.empty()){
			std::string url{constants::HostUrl};
			url += rest_api::endpoints::GET_AssetId;
			Document doc = rest_api::get_assets_list(url, rest_api_params);
			if(doc.IsNull()){
				constexpr const char *message = "Could not get response from server. Aborting.";
				log::error(message);
				return error(constants::error::HOST_UNREACHABLE, message);
			}
			return error();
		}

		/* Fetch remote asset data and compare to see if there are package updates */
		std::vector<std::string> p_updates = {};
		result_t r_cache = cache::get_package_info_by_title(package_titles);
		package::info_list p_cache = r_cache.unwrap_unsafe();

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

		if(!config.skip_prompt){
			if(!utils::prompt_user_yn("Do you want to update the following packages? (y/n)"))
				return error();
		}

		{
			error error;
			error = remove(config, p_updates);
			error = install(config, p_updates, params);
		}
		return error();
	}


	error search(
		const config::context& config,
		const package::title_list &package_titles,
		const package::params& params
	){
		result_t r_cache = cache::get_package_info_by_title(package_titles);
		info_list p_cache = r_cache.unwrap_unsafe();
		
		if(!p_cache.empty() && !config.enable_sync){
			print_list(p_cache);
			return error();
		}

		rest_api::request_params rest_api_params = rest_api::make_from_config(config);
		for(const auto& p_title : package_titles){
			using namespace rapidjson;
		
			rest_api_params.filter = http::url_escape(p_title);
			rest_api_params.verbose = config.verbose;
			rest_api_params.godot_version = config.info.godot_version;
			rest_api_params.max_results = 200;

			std::string request_url{constants::HostUrl};
			request_url += rest_api::endpoints::GET_Asset;
			Document doc = rest_api::get_assets_list(request_url, rest_api_params);
			if(doc.IsNull()){
				error error(
					constants::error::HOST_UNREACHABLE,
					"Could not fetch metadata."
				);
				log::error(error);
				return error;
			}

			log::info("{} package(s) found...", doc["total_items"].GetInt());
			print_list(doc);
		}
		return error();
	}


	error list(
		const config::context& config,
		const package::params& params
	){
		using namespace rapidjson;
		using namespace std::filesystem;

		string show((!params.sub_commands.empty()) ? params.sub_commands[0] : "");
		if(show.empty() || show == "packages"){
			result_t r_installed = cache::get_installed_packages();
			info_list p_installed = r_installed.unwrap_unsafe();
			if(!p_installed.empty()){
				print_list(p_installed);
			} 
			else{
				log::println("empty");
			}
		}
		else if(show == "remote"){
			remote::print_repositories(config);
		}
		else{
			error error(
				constants::error::UNKNOWN_COMMAND,
				"Unrecognized subcommand. Try either 'packages' or 'remote' instead."
			);
			log::error(error);
		}
		return error();
	}


	error export_to(const string_list& paths){
		/* Get all installed package information for export */
		result_t r_installed = cache::get_installed_packages();
		info_list p_installed = r_installed.unwrap_unsafe();

		result_t r_titles = get_package_titles(p_installed);
		title_list p_titles = r_titles.unwrap_unsafe();

		/* Build string of contents with one package title per line */
		string output{};
		std::for_each(p_titles.begin(), p_titles.end(), [&output](const string& p){
			output += p + "\n";
		});

		/* Write contents of installed packages in reusable format */
		for(const auto& path : paths ){
			std::ofstream of(path);
			if(std::filesystem::exists(path)){
				constexpr const char *message = "File or directory exists!";
				log::error(message);
				of.close();
				return error(constants::error::FILE_EXISTS, message);
			}
			log::println("writing contents to file");
			of << output;
			of.close();
		}

		return error();
	}


	error link(
		const config::context& config,
		const title_list& package_titles, 
		const package::params& params
	){
		using namespace std::filesystem;

		path_list paths = {};
		if(params.opts.contains("path")){
			paths = get<path_list>(params.opts.at("path"));
		}

		if(paths.empty()){
			error error(
				constants::error::PATH_NOT_DEFINED,
				"No path set. Use '--path' option to set a path."
			);
			log::error(error);
			return error;
		}
		result_t r_cache = cache::get_package_info_by_title(package_titles);
		info_list p_found = {};
		info_list p_cache = r_cache.unwrap_unsafe();
		if(p_cache.empty()){
			error error(
				constants::error::NOT_FOUND,
				"Could not find any packages to link."
			);
			log::error(error);
			return error;
		}

		for(const auto& p_title : package_titles){
			auto found = std::find_if(p_cache.begin(), p_cache.end(), [&p_title](const package::info& p){ return p.title == p_title; });
			if(found != p_cache.end()){
				p_found.emplace_back(*found);
			}
		}

		if(p_found.empty()){
			error error(
				constants::error::NO_PACKAGE_FOUND,
				"No packages found to link."
			);
			log::error(error);
			return error;
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
					error error(
						constants::error::STD_ERROR,
						std::format("Could not create symlink: {}", ec.message())
					);
					log::error(error);
				}
				log::println("Done.");
			}
		}
		return error();
	}


	error clone(
		const config::context& config,
		const title_list& package_titles, 
		const package::params& params
	){
		using namespace std::filesystem;

		if(params.opts.empty()){
			error error(
				constants::error::PATH_NOT_DEFINED,
				"No path set. Use '--path' option to set a path."
			);
			log::error(error);
			return error;
		}

		result_t r_cache = cache::get_package_info_by_title(package_titles);
		package::info_list p_found = {};
		package::info_list p_cache = r_cache.unwrap_unsafe(); 
		if(p_cache.empty()){
			error error(
				constants::error::NO_PACKAGE_FOUND,
				"Could not find any packages to clone."
			);
			log::error(error);
			return error;
		}

		for(const auto& p_title : package_titles){
			auto found = std::find_if(p_cache.begin(), p_cache.end(), [&p_title](const package::info& p){ return p.title == p_title; });
			if(found != p_cache.end()){
				p_found.emplace_back(*found);
			}
		}

		if(p_found.empty()){
			error error(
				constants::error::NO_PACKAGE_FOUND,
				"No packages found to clone."
			);
			log::error(error);
			return error;
		}

		/* Get the storage paths for all packages to create clones */
		path_list paths = get<path_list>(params.opts.at("--path"));
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
		return error();
	}


	void print_list(const info_list& packages){
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


	void print_list(const rapidjson::Document& json){
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


	result_t<info_list> get_package_info(const title_list& package_titles){
		return cache::get_package_info_by_title(package_titles);
	}


	result_t<title_list> get_package_titles(const info_list &packages){
		title_list package_titles;	
		std::for_each(packages.begin(), packages.end(), [&package_titles](const package::info& p){
			package_titles.emplace_back(p.title);
		});
		return result_t(package_titles, error());
	}


	void clean_temporary(
		const config::context& config,
		const title_list& package_titles
	){
		if(package_titles.empty()){
			log::info("Cleaned all temporary files.");
			std::filesystem::remove_all(config.tmp_dir);
		}
		/* Find the path of each packages is_installed then delete temporaries */
		log::info_n("Cleaning temporary files...");
		for(const auto& p_title : package_titles){
			string tmp_zip = config.tmp_dir + "/" + p_title + ".zip";
			if(config.verbose > 0)
				log::info("Removed '{}'", tmp_zip);
			std::filesystem::remove_all(tmp_zip);
		}
		log::println("Done.");
	}


	template <typename T>
	auto set_if_key_exists(
		const var_opts& o,
		const string& k,
		T& p
	){
		if(o.count(k)){ p = std::get<T>(o.at(k)); }
	}

	params make_params(
		const var_args& args, 
		const var_opts& opts
	){
		params p;
		set_if_key_exists(opts, "remote-source", p.remote_source);
		// set_if_key_exists(opts, "install-method", p.install_method);

		return p;
	}


	result_t<info_list> synchronize_database(
		const config::context& config,
		const title_list& package_titles
	){
		using namespace rapidjson;

		rest_api::request_params rest_api_params = rest_api::make_from_config(config);
		rest_api_params.page 	= 0;
		int page 				= 0;
		int page_length 		= 0;
		int total_items 		= 0;
		int items_left 			= 0;
		// int total_pages = 0;

		log::info("Sychronizing database...");
		do{
			/* Make the GET request to get page data and store it in the local 
			package database. Also, check to see if we need to keep going. */
			std::string url{constants::HostUrl};
			url += rest_api::endpoints::GET_Asset;
			Document doc = rest_api::get_assets_list(url, rest_api_params);
			rest_api_params.page += 1;

			if(doc.IsNull()){
				error error(
					constants::error::EMPTY_RESPONSE,
					"Could not get response from server. Aborting."
				);
				log::error(error);
				return result_t(info_list(), error);
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
				error error;
				error = cache::drop_package_database();
				error = cache::create_package_database();
			}

			info_list packages;
			for(const auto& o : doc["result"].GetArray()){
				// log::println("=======================");
				info p{
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
			error error = cache::insert_package_info(packages);
			if (error.has_occurred()){
				log::error(error);
				/* FIXME: Should this stop here or keep going? */
			}
			/* Make the same request again to get the rest of the needed data 
			using the same request, but with a different page, then update 
			variables as needed. */


		} while(items_left > 0);

		log::println("Done.");

		return cache::get_package_info_by_title(package_titles);
	}


	result_t<info_list> resolve_dependencies(
		const config::context& config,
		const title_list& package_titles
	){
		result_t r_cache = cache::get_package_info_by_title(package_titles);
		info_list p_cache = r_cache.unwrap_unsafe();
		info_list p_deps = {};

		/* Build an graph of every thing to check then install in order */
		for(const auto& p : p_cache){
			if(p.dependencies.empty())
				continue;
			
			/* Check if dependency has a dependency. If so, resolve those first. */
			for(const auto& d : p.dependencies){
				result_t r_temp = resolve_dependencies(config, {d.title});
				info_list temp  = r_temp.unwrap_unsafe();
				utils::move_if_not(temp, p_deps, [](const info& p){ return true; });
			}
		}

		return result_t(p_deps, error());
	}

	string to_json(
		const info& info, 
		bool pretty_pretty
	){
		string json("");

		return json;
	}
}