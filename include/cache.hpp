
#include "constants.hpp"
#include <sqlite3.h>
#include <vector>
#include <string>

namespace gdpm::package_manager{
	struct package_info;
}

namespace gdpm::cache{
	using namespace package_manager;
	int create_package_database(const std::string& cache_path = GDPM_PACKAGE_CACHE_PATH, const std::string& table_name = GDPM_PACKAGE_CACHE_TABLENAME);
	int insert_package_info(const std::vector<package_info>& package, const std::string& cache_path = GDPM_PACKAGE_CACHE_PATH, const std::string& table_name = GDPM_PACKAGE_CACHE_TABLENAME);
	std::vector<package_info> get_package_info_by_id(const std::vector<size_t>& package_ids, const std::string& cache_path = GDPM_PACKAGE_CACHE_PATH, const std::string& table_name = GDPM_PACKAGE_CACHE_TABLENAME);
	std::vector<package_info> get_package_info_by_title(const std::vector<std::string>& package_titles, const std::string& cache_path = GDPM_PACKAGE_CACHE_PATH, const std::string& table_name = GDPM_PACKAGE_CACHE_TABLENAME);
	std::vector<package_info> get_installed_packages(const std::string& cache_path = GDPM_PACKAGE_CACHE_PATH, const std::string& table_name = GDPM_PACKAGE_CACHE_TABLENAME);
	int update_package_info(const std::vector<package_info>& packages, const std::string& cache_path = GDPM_PACKAGE_CACHE_PATH, const std::string& table_name = GDPM_PACKAGE_CACHE_TABLENAME);
	int update_sync_info(const std::vector<std::string>& download_urls, const std::string& cache_path = GDPM_PACKAGE_CACHE_PATH, const std::string& table_name = GDPM_PACKAGE_CACHE_TABLENAME);
	int delete_packages(const std::vector<std::string>& package_titles, const std::string& cache_path = GDPM_PACKAGE_CACHE_PATH, const std::string& table_name = GDPM_PACKAGE_CACHE_TABLENAME);
	int delete_packages(const std::vector<size_t>& package_ids, const std::string& cache_path = GDPM_PACKAGE_CACHE_PATH, const std::string& table_name = GDPM_PACKAGE_CACHE_TABLENAME);
	int drop_package_database(const std::string& cache_path = GDPM_PACKAGE_CACHE_PATH, const std::string& table_name = GDPM_PACKAGE_CACHE_TABLENAME);

	std::string to_values(const package_info& package);
	std::string to_values(const std::vector<package_info>& packages);
}