
#include "package.hpp"
#include "colors.hpp"
#include "error.hpp"
#include "log.hpp"
#include "rest_api.hpp"
#include "config.hpp"
#include "cache.hpp"
#include "http.hpp"
#include "remote.hpp"
#include "types.hpp"
#include "utils.hpp"
#include <filesystem>
#include <functional>
#include <future>
#include <rapidjson/error/en.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <tabulate/table.hpp>

namespace gdpm::package{
	
	error install(
		const config::context& config,
		package::title_list& package_titles, 
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

		/* Synchronize database information and then try to get data again from
		cache if possible. */
		if(config.enable_sync){
			result_t result = fetch(config, package_titles);
			error error = result.get_error();
			if(error.has_occurred()){
				return log::error_rc(ec::UNKNOWN, "package::install(): could not synchronize database.");
			}
		}
		/* Append files from --file option */
		read_file_inputs(package_titles, params.input_files);
		result_t result = cache::get_package_info_by_title(package_titles);
		package::info_list p_cache = result.unwrap_unsafe();

		/* Found nothing to install so there's nothing to do at this point. */
		if(p_cache.empty()){
			return log::error_rc(
				ec::NOT_FOUND, /* TODO: change to PACKAGE_NOT_FOUND */
				"package::install(): no packages found to install."
			);
		}
		
		/* Show packages to install */
		{
			using namespace tabulate;
			Table table;
			table.format()
				.border_top("")
				.border_bottom("")
				.border_left("")
				.border_right("")
				.corner("")
				.padding_top(0)
				.padding_bottom(0);
			table.add_row({"Title", "Author", "Category", "Version", "Godot", "Last Modified", "Installed?"});
			table[0].format()
				.font_style({FontStyle::underline, FontStyle::bold});
			for(const auto& p : p_cache){
				table.add_row({p.title, p.author, p.category, p.version, p.godot_version, p.modify_date, (p.is_installed) ? "✔️": "❌"});
				size_t index = table.size() - 1;
				table[index][3].format().font_align(FontAlign::center);
				table[index][4].format().font_align(FontAlign::center);
				table[index][6].format().font_align(FontAlign::center);
			}
			table.print(std::cout);
			log::println("");
		}
		
		/* Skip prompt if set in config */
		if(!config.skip_prompt){
			if(!utils::prompt_user_yn("Do you want to install these packages? (Y/n)"))
				return error();
		}

		/* Check if provided remote param is in remote sources */
		if(!config.remote_sources.contains(params.remote_source)){
			return log::error_rc(
				ec::NOT_FOUND, /* TODO: change to REMOTE_NOT_FOUND */
				"package::install(): remote source not found in config."
			);
		}

		// task_list tasks;
		/* Retrieve necessary asset data if it was found already in cache */
		std::vector<string_pair> target_extract_dirs;
		rest_api::request_params rest_api_params = rest_api::make_from_config(config);
		package::title_list p_download_urls;
		package::path_list p_storage_paths;
		for(auto& p : p_cache){
			log::info_n("Fetching asset data for \"{}\"...", p.title);
			string url{config.remote_sources.at(params.remote_source) + rest_api::endpoints::GET_AssetId};
			string package_dir = config.packages_dir + "/" + p.title;
			string tmp_dir 	= config.tmp_dir + "/" + p.title;
			string tmp_zip 	= tmp_dir + ".zip";

			Document doc;
			bool is_data_missing = p.download_url.empty() || p.category.empty() || p.description.empty() || p.support_level.empty();
			if(is_data_missing){
				doc = rest_api::get_asset(url, p.asset_id, rest_api_params);
				if(doc.HasParseError() || doc.IsNull()){
					return log::error_rc(
						ec::JSON_ERR,
						std::format("package::install(): error parsing JSON: {}", 
							GetParseError_En(doc.GetParseError()))
					);
				}
				p.category			= doc["category"].GetString();
				p.description 		= doc["description"].GetString();
				p.support_level 	= doc["support_level"].GetString();
				p.download_url 		= doc["download_url"].GetString();
				p.download_hash 	= doc["download_hash"].GetString();
				log::println("Done");
			}
			else{
				log::println("");
				log::info("Found asset data for \"{}\".", p.title);
			}
		
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
			target_extract_dirs.emplace_back(string_pair(tmp_zip, package_dir + "/"));

			/* Check if we already have a stored temporary file before attempting to download */
			if(std::filesystem::exists(tmp_zip) && std::filesystem::is_regular_file(tmp_zip)){
				log::info("Found cached package for \"{}\".", p.title);
			}
			else{
				p_download_urls.emplace_back(p.download_url);
				p_storage_paths.emplace_back(tmp_zip);
			}
		}

		/* Make sure the number of urls matches storage locations */
		if(p_download_urls.size() != p_storage_paths.size()){
			return log::error_rc(error(ec::ASSERTION_FAILED,
				"package::install(): p_left.size() != p_storage.size()"
			));
		}

		/* Download ZIP files using download url */
		if(config.jobs > 1){
			http::context http(config.jobs);
			http::responses responses = http.download_files(p_download_urls, p_storage_paths);
			
			/* Check for HTTP response errors */
			for(const auto& r : responses){
				if(r.code != http::OK){
					log::error(error(ec::HTTP_RESPONSE_ERR,
						std::format("HTTP error: {}", r.code)
					));
				}
			}
		}
		else{
			http::context http;
			for(size_t i = 0; i < p_download_urls.size(); i++){
				http::response r = http.download_file(
					p_download_urls[i], 
					p_storage_paths[i]
				);
				if(r.code != http::OK){
					log::error(error(ec::HTTP_RESPONSE_ERR,
						std::format("HTTP error: {}", r.code)
					));
				}
			}
		}

		/* Extract all the downloaded packages to their appropriate directory location. */
		for(const auto& p : target_extract_dirs){
			error error = utils::extract_zip(p.first.c_str(), p.second.c_str());
			if(error.has_occurred()){
				return error;
			}
			log::println("Done.");
		}
		
		/* Update the cache data */
		for(auto& p : p_cache){
			p.is_installed = true;
			p.install_path = config.packages_dir + "/" + p.title;
		}

		log::info_n("Updating local asset data...");
		error error = cache::update_package_info(p_cache);
		if(error.has_occurred()){
			string prefix = std::format(log::get_error_prefix(), utils::timestamp());
			log::println(GDPM_COLOR_LOG_ERROR"\n{}{}" GDPM_COLOR_RESET, prefix, error.get_message());
			return error;
		}
		if(config.clean_temporary){
			clean(config, package_titles);
		}
		log::println("Done.");

		return error;
	}


	error get(
		const config::context& config,
		title_list& package_titles,
		const params& params
	){
		using namespace rapidjson;

		/* Download and install package(s) in local project without storing
		package info in the package database. This will check for packages stored
		in local cache first. */
		result_t result = cache::get_package_info_by_title(package_titles);
		package::info_list p_found = {};
		package::info_list p_cache = result.unwrap_unsafe();
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

		/* Install the ones found from cache first. */

		/* Check if provided param is in remote sources*/
		if(!config.remote_sources.contains(params.remote_source)){
			return log::error_rc(ec::NOT_FOUND,
				"Remote source not found in config."
			);
		}

		/* Install the other packages from remte source. */
		std::vector<string_pair> dir_pairs;
		task_list tasks;
		rest_api::request_params rest_api_params = rest_api::make_from_config(config);
		package::info_list p_left;
		for(auto& p : p_found){	// TODO: Execute each in parallel using coroutines??

			/* Check if a remote source was provided. If not, then try to get packages
			in global storage location only. */

			log::info("Fetching asset data for \"{}\"...", p.title);
			string url{config.remote_sources.at(params.remote_source) + rest_api::endpoints::GET_AssetId};
			string package_dir, tmp_dir, tmp_zip;

			/* Retrieve necessary asset data if it was found already in cache */
			Document doc;
			bool is_missing_data = p.download_url.empty() || p.category.empty() || p.description.empty() || p.support_level.empty();
			if(is_missing_data){
				doc = rest_api::get_asset(url, p.asset_id, rest_api_params);
				if(doc.HasParseError() || doc.IsNull()){
					return log::error_rc(error(
						constants::error::JSON_ERR,
						std::format("Error parsing JSON: {}", GetParseError_En(doc.GetParseError()))
					));
				}
				p.category			= doc["category"].GetString();
				p.description 		= doc["description"].GetString();
				p.support_level 	= doc["support_level"].GetString();
				p.download_url 		= doc["download_url"].GetString();
				p.download_hash 	= doc["download_hash"].GetString();
			}
			else{
				log::info("Found asset data found for \"{}\"", p.title);
			}

			/* Set directory and temp paths for storage */
			package_dir = std::filesystem::current_path().string() + "/" + p.title;//config.packages_dir + "/" + p.title;
			tmp_dir 	= std::filesystem::current_path().string() + "/" + p.title + ".tmp";
			tmp_zip 	= tmp_dir + ".zip";

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
			else {
				p_left.emplace_back(p);
			}
		} // for loop

		/* Get the packages not found in cache and download */
		{
			string_list urls;
			for(const auto& p : p_left){
				urls.emplace_back(p.download_url);
			}
			http::context http;
			http::responses responses = http.requests(urls);

			for(const auto& response : responses){
				if(response.code == http::OK){
					log::println("Done.");
				}else{
					return log::error_rc(error(
						constants::error::HTTP_RESPONSE_ERR,
						std::format("HTTP Error: {}", response.code)
					));
				}
			}
		}

		/* Extract all packages and update cache database */
		for(auto& p : p_found){
			string package_dir = std::filesystem::current_path().string() + "/" + p.title;//config.packages_dir + "/" + p.title;
			string tmp_dir 	= std::filesystem::current_path().string() + "/" + p.title + ".tmp";
			string tmp_zip 	= tmp_dir + ".zip";

			dir_pairs.emplace_back(string_pair(tmp_zip, package_dir + "/"));

			p.is_installed = true;
			p.install_path = package_dir;

			/* Extract all the downloaded packages to their appropriate directory location. */
			for(const auto& p : dir_pairs){
				error error = utils::extract_zip(p.first.c_str(), p.second.c_str());
			}

			/* Remove temporary download archive */
			for(const auto& p : p_found){
				string tmp_zip = std::filesystem::current_path().string() 
					+ "/" + p.title + ".tmp.zip";
				std::filesystem::remove_all(tmp_zip);
			}
		}
		return error();
	}


	error remove(
		const config::context& config,
		string_list& package_titles, 
		const package::params& params
	){
		using namespace rapidjson;
		using namespace std::filesystem;


		/* Append package titles from --file option */
		read_file_inputs(package_titles, params.input_files);

		/* Find the packages to remove if they're is_installed and show them to the user */
		result_t result = cache::get_package_info_by_title(package_titles);
		package::info_list p_cache = result.unwrap_unsafe();
		if(p_cache.empty()){
			return log::error_rc(ec::NOT_FOUND,
				"Could not find any packages to remove."
			);
		}

		/* Count number packages in cache flagged as is_installed. If there are none, then there's nothing to do. */
		size_t p_count = 0;
		std::for_each(p_cache.begin(), p_cache.end(), [&p_count](const package::info& p){
				p_count += (p.is_installed) ? 1 : 0;
		});

		if(p_count == 0){
			return log::error_rc(ec::NOT_FOUND,
				"No packages to remove."
			);
		}
		
		{
			using namespace tabulate;
			Table table;
			table.format()
				.border_top("")
				.border_bottom("")
				.border_left("")
				.border_right("")
				.corner("")
				.padding_top(0)
				.padding_bottom(0);
			table.add_row({"Title", "Author", "Category", "Version", "Godot", "Last Modified", "Installed?"});
			table[0].format()
				.font_style({FontStyle::underline, FontStyle::bold});
			for(const auto& p : p_cache){
				table.add_row({p.title, p.author, p.category, p.version, p.godot_version, p.modify_date, (p.is_installed) ? "✔️": "❌"});
				size_t index = table.size() - 1;
				table[index][3].format().font_align(FontAlign::center);
				table[index][4].format().font_align(FontAlign::center);
				table[index][6].format().font_align(FontAlign::center);
			}
			table.print(std::cout);
			log::println("");
		}
		
		if(!config.skip_prompt){
			if(!utils::prompt_user_yn("Do you want to remove these packages? (Y/n)"))
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
		if(config.clean_temporary){
			clean(config, package_titles);
		}
		log::info_n("Updating local asset data...");
		{
			error error = cache::update_package_info(p_cache);
			if(error.has_occurred()){
				log::error("\nsqlite: {}", error.get_message());
				return error;
			}
		}
		log::println("Done.");

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
			string url{constants::HostUrl + rest_api::endpoints::GET_AssetId};
			Document doc = rest_api::get_assets_list(url, rest_api_params);
			if(doc.IsNull()){
				return log::error_rc(ec::HOST_UNREACHABLE, 
					"package::update(): could not get response from server. Aborting."
				);
			}
			return error();
		}

		/* Fetch remote asset data and compare to see if there are package updates */
		package::title_list p_updates = {};
		result_t r_cache = cache::get_package_info_by_title(package_titles);
		package::info_list p_cache = r_cache.unwrap_unsafe();

		log::println("Packages to update: ");
		for(const auto& p_title : p_updates)
			log::print("  {}  ", p_title);
		log::println("");

		/* Check version information to see if packages need updates */
		for(const auto& p : p_cache){
			string url{constants::HostUrl + rest_api::endpoints::GET_AssetId};
			Document doc = rest_api::get_asset(url, p.asset_id);
			string remote_version = doc["version"].GetString();
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

		/* Do a generic query with no filter */
		rest_api::request_params rest_api_params = rest_api::make_from_config(config);
		if(package_titles.empty()){
			return print_asset(rest_api_params);
		}

		/* Query each package title supplied as input */
		for(const auto& p_title : package_titles){
			error error = print_asset(rest_api_params, p_title, config.style);
			if(error.has_occurred()){
				log::error(error);
				continue;
			}
		}
		return error();
	}


	error list(
		const config::context& config,
		const package::params& params
	){
		using namespace rapidjson;
		using namespace std::filesystem;

		string show((!params.args.empty()) ? params.args[0] : "");
		if(show.empty() || show == "packages"){
			result_t r_installed = cache::get_installed_packages();
			info_list p_installed = r_installed.unwrap_unsafe();
			if(!p_installed.empty()){
				if(config.style == print::style::list)
					print_list(p_installed);
				else if(config.style == print::style::table){
					print_table(p_installed);
				}
			} 
		}
		else if(show == "remote"){
			remote::print_repositories(config);
		}
		else{
			log::error_rc(ec::UNKNOWN_COMMAND,
				"package::list(): unrecognized subcommand...try either 'packages' or 'remote' instead."
			);
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
			if(std::filesystem::exists(path)){
				if(utils::prompt_user_yn("File or directory exists. Do you want to overwrite it?")){

				}
				else {
					return log::error_rc(ec::FILE_EXISTS,
						"package::export_to(): file or directory exists!"
					);
				}
			}
			std::ofstream of(path);
			log::println("export: {}", path);
			of << output;
			of.close();
		}

		return error();
	}


	error clean(
		const config::context& config,
		const title_list& package_titles
	){
		if(package_titles.empty()){
			if(!config.skip_prompt){
				if(!utils::prompt_user_yn("Are you sure you want to clean all temporary files? (Y/n)")){
					return error();
				}
			}
			std::filesystem::remove_all(config.tmp_dir);
			return error();
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
		return error();
	}


	error purge(const config::context& config){
		if(!config.skip_prompt){
			if(!utils::prompt_user_yn("Are you sure you want to purge all installed packages? (Y/n)")){
				return error();
			}
		}
		/* Remove all packages installed in global location */
		std::filesystem::remove_all(config.packages_dir);
		return cache::drop_package_database();
	}


	error link(
		const config::context& config,
		const title_list& package_titles, 
		const package::params& params
	){
		using namespace std::filesystem;

		if(params.paths.empty()){
			return log::error_rc(error(
				constants::error::MALFORMED_PATH,
				"Path is required"
			));
		}

		/* Check for packages in cache to link */
		result_t r_cache = cache::get_package_info_by_title(package_titles);
		info_list p_found = {};
		info_list p_cache = r_cache.unwrap_unsafe();
		if(p_cache.empty()){
			return log::error_rc(error(
				constants::error::NOT_FOUND,
				"Could not find any packages to link in cache."
			));
		}

		for(const auto& p_title : package_titles){
			auto found = std::find_if(p_cache.begin(), p_cache.end(), [&p_title](const package::info& p){ return p.title == p_title; });
			if(found != p_cache.end()){
				p_found.emplace_back(*found);
			}
		}

		if(p_found.empty()){
			return log::error_rc(error(
				constants::error::NO_PACKAGE_FOUND,
				"No packages found to link."
			));
		}

		/* Get the storage paths for all packages to create symlinks */
		const path package_dir{config.packages_dir};
		for(const auto& p : p_found){
			for(const auto& path : params.paths){
				log::println("link: \"{}\" -> '{}'...", p.title, path + "/" + p.title);
				// std::filesystem::path target{config.packages_dir + "/" + p.title};
				std::filesystem::path target = {current_path().string() + "/" + config.packages_dir + "/" + p.title};
				std::filesystem::path symlink_path{path + "/" + p.title};
				if(!std::filesystem::exists(symlink_path.string()))
					std::filesystem::create_directories(path + "/");
				std::error_code ec;
				std::filesystem::create_directory_symlink(target, symlink_path, ec);
				if(ec){
					log::error(error(
						constants::error::STD_ERR,
						std::format("Could not create symlink: {}", ec.message())
					));
				}
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

		if(params.paths.empty()){
			return log::error_rc(error(
				constants::error::MALFORMED_PATH,
				"Path is required"
			));
		}

		result_t r_cache = cache::get_package_info_by_title(package_titles);
		package::info_list p_found = {};
		package::info_list p_cache = r_cache.unwrap_unsafe();

		/* Check for installed packages to clone */
		if(p_cache.empty()){
			return log::error_rc(error(
				constants::error::NO_PACKAGE_FOUND,
				"Could not find any packages to clone in cache."
			));
		}

		for(const auto& p_title : package_titles){
			auto found = std::find_if(
				p_cache.begin(), 
				p_cache.end(), 
				[&p_title](const package::info& p){ 
					return p.title == p_title; 
			});
			if(found != p_cache.end()){
				p_found.emplace_back(*found);
			}
		}

		if(p_found.empty()){
			return log::error_rc(error(
				constants::error::NO_PACKAGE_FOUND,
				"No packages found to clone."
			));
		}

		/* Get the storage paths for all packages to create clones */
		// path_list paths = path_list({params.args.back()});
		const path package_dir{config.packages_dir};
		for(const auto& p : p_found){
			for(const auto& path : params.paths){
				log::println("clone: \"{}\" -> {}", p.title, path + "/" + p.title);
				std::filesystem::path from{config.packages_dir + "/" + p.title};
				std::filesystem::path to{path + "/" + p.title};
				if(!std::filesystem::exists(to.string()))
					std::filesystem::create_directories(to); /* This should only occur if using a --force flag */

				/* TODO: Add an option to force overwriting (i.e. --overwrite) */
				std::filesystem::copy(from, to, copy_options::update_existing | copy_options::recursive);
			}
		}
		return error();
	}


	result_t<info_list> fetch(
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

		log::info_n("Sychronizing database...");
		do{
			/* Make the GET request to get page data and store it in the local 
			package database. Also, check to see if we need to keep going. */
			string url{constants::HostUrl + rest_api::endpoints::GET_Asset};
			Document doc = rest_api::get_assets_list(url, rest_api_params);
			rest_api_params.page += 1;

			if(doc.IsNull()){
				log::println("");
				return result_t(info_list(), log::error_rc(
					ec::EMPTY_RESPONSE,
					"Could not get response from server. Aborting."
				));
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


	void print_list(const info_list& packages){
		for(const auto& p : packages){
			log::println(
				GDPM_COLOR_BLUE"{}/" 
				GDPM_COLOR_RESET "{}/{}/{}  " 
				GDPM_COLOR_GREEN "v{}  " 
				GDPM_COLOR_CYAN "{}  " 
				GDPM_COLOR_RESET "Godot {},  {}",
				p.support_level,
				p.category,
				p.author,
				p.title,
				p.version,
				p.modify_date,
				// p.asset_id,
				p.godot_version,
				p.cost
			);
		}
	}


	void print_list(const rapidjson::Document& json){
		for(const auto& o : json["result"].GetArray()){
			log::println(
				GDPM_COLOR_BLUE"{}/" 
				GDPM_COLOR_CYAN "{}/" 
				GDPM_COLOR_RESET "{}/{}  " 
				GDPM_COLOR_GREEN "v{}  " 
				GDPM_COLOR_CYAN "{}  " 
				GDPM_COLOR_RESET "Godot {},  {}",
				o["support_level"]	.GetString(), 
				utils::to_lower(o["category"].GetString()),
				o["author"]			.GetString(),
				o["title"]			.GetString(),
				o["version_string"]	.GetString(), 
				o["modify_date"]	.GetString(),
				// o["asset_id"]		.GetString(),
				o["godot_version"]	.GetString(),
				o["cost"]			.GetString()
			);
		}
	}


	void print_table(const info_list& packages){
		using namespace tabulate;
		Table table;
		table.add_row({
			"Asset Name",
			 "Author",
			 "Category",
			 "Version",
			 "Godot Version",
			 "License/Cost",
			 "Last Modified",
			 "Support"
		});
		for(const auto& p : packages){
			table.add_row({
				p.title, 
				p.author,
				p.category,
				p.version,
				p.godot_version,
				p.cost,
				p.modify_date,
				p.support_level
			});
		}
		table.print(std::cout);
	}


	void print_table(const rapidjson::Document& json){
		using namespace tabulate;
		Table table;
		table.add_row({
			"Asset Name",
			 "Author",
			 "Category",
			 "Version",
			 "Godot Version",
			 "License/Cost",
			 "Last Modified",
			 "Support"
		});
		for(const auto& o : json["result"].GetArray()){
			table.add_row({
				o["title"]			.GetString(), 
				o["author"]			.GetString(),
				o["category"]		.GetString(),
				o["version_string"]	.GetString(),
				o["godot_version"]	.GetString(),
				o["cost"]			.GetString(),
				o["modify_date"]	.GetString(),
				o["support_level"]	.GetString()
			});
		}
		table.print(std::cout);
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


	template <typename T>
	auto set_if_key_exists(
		const var_opts& o,
		const string& k,
		T& p
	){
		if(o.count(k)){ p = std::get<T>(o.at(k)); }
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


	void read_file_inputs(
		title_list& package_titles,
		const path_list& paths
	){
		for(const auto& filepath : paths){
			string contents = utils::readfile(filepath);
			title_list input_titles = utils::split_lines(contents);
			package_titles.insert(
				std::end(package_titles),
				std::begin(input_titles),
				std::end(input_titles)
			);
		}
	}

	info_list find_cached_packages(const title_list& package_titles){
		/* Download and install package(s) in local project without storing
		package info in the package database. This will check for packages stored
		in local cache first. */
		result_t result = cache::get_package_info_by_title(package_titles);
		package::info_list p_found = {};
		package::info_list p_cache = result.unwrap_unsafe();
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
		return p_found;
	}

	
	info_list find_installed_packages(const title_list& package_titles){
		result_t result = cache::get_package_info_by_title(package_titles);
		package::info_list p_installed = {};
		package::info_list p_cache = result.unwrap_unsafe();
		log::debug("Searching for installed packages in cache...");
		for(const auto& p_title : package_titles){
			auto found = std::find_if(
				p_cache.begin(), 
				p_cache.end(), 
				[&p_title](const package::info& p){ 
					return p.title == p_title && p.is_installed; 
				}
			);
			if(found != p_cache.end()){
				p_installed.emplace_back(*found);
			}
		}

		return p_installed;
	}


	string to_json(
		const info& info, 
		bool pretty_pretty
	){
		string json("");

		return json;
	}
}