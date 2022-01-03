
#include "constants.hpp"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

namespace gdpm::rest_api{
	// See GitHub reference: https://github.com/godotengine/godot-asset-library/blob/master/API.md
	namespace endpoints{
		constexpr const char *POST_Register 		= "/register";
		constexpr const char *POST_Login 			= "/login";
		constexpr const char *POST_Logout 			= "/logout";
		constexpr const char *POST_ChangePassword 	= "/change_password";
		constexpr const char *GET_Configure			= "/configure";
		constexpr const char *GET_Asset_NoParams	= "/asset";
		constexpr const char *GET_Asset				= "/asset?";
		constexpr const char *GET_AssetId			= "/asset/{id}"; // ...find_replace
		constexpr const char *POST_AssetIdDelete	= "/asset/{id}/delete";
		constexpr const char *POST_AssetIdUndelete	= "/asset/{id}/delete";
		constexpr const char *POST_AssetSupportLevel = "/asset/{id}/support_level";
		constexpr const char *POST_Asset			= "/asset";
		constexpr const char *POST_AssetId			= "/asset/{id}";
		constexpr const char *POST_AssetEditId		= "/asset/edit/{id}";
	}

	bool register_account(const std::string& username, const std::string& password, const std::string& email);
	bool login(const std::string& username, const std::string& password);
	bool logout();
	// bool change_password()

	enum type_e		{ any, addon, project };
	enum support_e	{ all, official, community, testing };
	enum sort_e		{ none, rating, cost, name, updated };

	struct rest_api_context{
		type_e type;
		int category;
		support_e support;
		std::string filter;
		std::string user;
		std::string godot_version;
		int max_results;
		int page;
		sort_e sort;
		bool reverse;
		int verbose;
	};

	rest_api_context make_context(type_e type = GDPM_DEFAULT_ASSET_TYPE, int category = GDPM_DEFAULT_ASSET_CATEGORY, support_e support = GDPM_DEFAULT_ASSET_SUPPORT, const std::string& filter = GDPM_DEFAULT_ASSET_FILTER, const std::string& user = GDPM_DEFAULT_ASSET_USER, const std::string& godot_version = GDPM_DEFAULT_ASSET_GODOT_VERSION, int max_results = GDPM_DEFAULT_ASSET_MAX_RESULTS, int page = GDPM_DEFAULT_ASSET_PAGE, sort_e sort = GDPM_DEFAULT_ASSET_SORT, bool reverse = GDPM_DEFAULT_ASSET_REVERSE, int verbose = GDPM_DEFAULT_ASSET_VERBOSE);

	std::string to_string(type_e type);
	std::string to_string(support_e support);
	std::string to_string(sort_e sort);
	void _print_params(const rest_api_context& params);
	rapidjson::Document _parse_json(const std::string& r, int verbose = 0);
	std::string _prepare_request(const std::string& url, const rest_api_context& context);

	bool register_account(const std::string& username, const std::string& password, const std::string& email);
	bool login(const std::string& username, const std::string& password);
	bool logout();

	rapidjson::Document configure(const std::string& url = constants::HostUrl, type_e type = any, int verbose = 0);
	rapidjson::Document get_assets_list(const std::string& url = constants::HostUrl, type_e type = GDPM_DEFAULT_ASSET_TYPE, int category = GDPM_DEFAULT_ASSET_CATEGORY, support_e support = GDPM_DEFAULT_ASSET_SUPPORT, const std::string& filter = GDPM_DEFAULT_ASSET_FILTER, const std::string& user = GDPM_DEFAULT_ASSET_USER, const std::string& godot_version = GDPM_DEFAULT_ASSET_GODOT_VERSION, int max_results = GDPM_DEFAULT_ASSET_MAX_RESULTS, int page = GDPM_DEFAULT_ASSET_PAGE, sort_e sort = GDPM_DEFAULT_ASSET_SORT, bool reverse = GDPM_DEFAULT_ASSET_REVERSE, int verbose = GDPM_DEFAULT_ASSET_VERBOSE);
	rapidjson::Document get_assets_list(const std::string& url, const rest_api_context& params = {});
	rapidjson::Document get_asset(const std::string& url, int asset_id, const rest_api_context& params = {});
	bool delete_asset(int asset_id);				// ...for moderators
	bool undelete_asset(int asset_id);				// ...for moderators
	bool set_support_level(int asset_id);			// ...for moderators
	
	/*
	POST /asset
	POST /asset/{id}
	POST /asset/edit/{id}
	*/
	void edit_asset(/* need input parameters... */);

	/* GET /asset/{id} */
	void get_asset_edit(int asset_id);

	/* POST /asset/edit/{id}/review */
	std::string review_asset_edit(int asset_id);

	/* POST /asset/edit/{id}/accept */
	std::string accept_asset_edit(int asset_id);	// ...for moderators

	/* POST /asset/edit/{id}/reject */
	std::string reject_asset_edit(int asset_id);
}