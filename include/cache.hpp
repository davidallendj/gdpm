
#pragma once

#include "constants.hpp"
#include "package.hpp"
#include "error.hpp"
#include "result.hpp"
#include <sqlite3.h>
#include <vector>
#include <string>


namespace gdpm::cache {
	struct params {
		string cache_path	= GDPM_PACKAGE_CACHE_PATH;
		string table_name	= GDPM_PACKAGE_CACHE_TABLENAME;
	};

	error create_package_database(bool overwrite = false, const params& = params());
	error insert_package_info(const package::info_list& packages, const params& = params());
	result_t<package::info_list> get_package_info_by_id(const package::id_list& package_ids, const params& = params());
	result_t<package::info_list> get_package_info_by_title(const package::title_list& package_titles, const params& params = cache::params());
	result_t<package::info_list> get_installed_packages(const params& = params());
	error update_package_info(const package::info_list& packages, const params& = params());
	error update_sync_info(const args_t& download_urls, const params& = params());
	error delete_packages(const package::title_list& package_titles, const params& = params());
	error delete_packages(const package::id_list& package_ids, const params& = params());
	error drop_package_database(const params& = params());

	result_t<string> to_values(const package::info& package);
	result_t<string> to_values(const package::info_list& packages);
}