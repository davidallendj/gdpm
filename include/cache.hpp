
#include <sqlite3.h>
#include <vector>
#include <string>

namespace gdpm::package_manager{
	struct package_info;
}

namespace gdpm::cache{
	using namespace package_manager;
	int create_package_database();
	int insert_package_info(const std::vector<package_info>& package);
	std::vector<package_info> get_package_info_by_id(const std::vector<size_t>& package_ids);
	std::vector<package_info> get_package_info_by_title(const std::vector<std::string>& package_titles);
	std::vector<package_info> get_installed_packages();
	int update_package_info(const std::vector<package_info>& packages);
	int update_sync_info(const std::vector<std::string>& download_urls);
	int delete_packages(const std::vector<std::string>& package_titles);
	int delete_packages(const std::vector<size_t>& package_ids);
	int drop_package_database();

	std::string to_values(const package_info& package);
	std::string to_values(const std::vector<package_info>& packages);
}