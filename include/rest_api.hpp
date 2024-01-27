#pragma once

#include "constants.hpp"
#include "types.hpp"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>


namespace gdpm::config{
	struct context;
}

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
		constexpr const char *POST_AssetIdUndelete	= "/asset/{id}/undelete";
		constexpr const char *POST_AssetSupportLevel = "/asset/{id}/support_level";
		constexpr const char *POST_Asset			= "/asset";
		constexpr const char *POST_AssetId			= "/asset/{id}";
		constexpr const char *POST_AssetEditId		= "/asset/edit/{id}";
	}

	bool register_account(const string& username, const string& password, const string& email);
	bool login(const string& username, const string& password);
	bool logout();
	// bool change_password()

	enum type_e: 	int	{ any = 0, addon, project };
	enum support_e: int	{ all = 0, official, community, testing };
	enum sort_e: 	int	{ none = 0, rating, cost, name, updated };
	/* TODO: Include all information with categories and not just type*/
	struct category{
		int id;
		int type;
		string name;
	};
	using categories = std::vector<category>;

	struct request_params{
		int			type;
		int 		category;
		int 		support;
		string 		user			= "";
		string		cost			= "";
		string 		godot_version	= "4.3";
		int 		max_results		= 500;
		int 		page;
		int			offset;
		int 		sort;
		bool 		reverse;
		int 		verbose;
	};

	request_params make_from_config(const config::context& config);
	string to_json(const json::document& doc);
	string to_string(type_e type);
	string to_string(support_e support);
	string to_string(sort_e sort);
	error print_params(const request_params& params, const string& filter = "");
	error print_asset(const request_params& params, const string& filter = "", const print::style& style = print::style::list);
	json::document _parse_json(const string& r, int verbose = 0);
	string _prepare_request(const string& url, const request_params& context, const string& filter);

	bool register_account(const string& username, const string& password, const string& email);
	bool login(const string& username, const string& password);
	bool logout();

	json::document configure(const string& url = constants::HostUrl, type_e type = any, int verbose = 0);
	json::document get_assets_list(const string& url, const request_params& params = {}, const string& filter = "");
	json::document get_asset(const string& url, int asset_id, const request_params& params = {}, const string& filter = "");
	bool delete_asset(int asset_id);				// ...for moderators
	bool undelete_asset(int asset_id);				// ...for moderators
	bool set_support_level(int asset_id);			// ...for moderators

	namespace multi{
		json::documents get_assets(const string_list& urls, id_list aset_ids, const request_params& api_params, const string_list& filters);
	}
	
	/*
	POST /asset
	POST /asset/{id}
	POST /asset/edit/{id}
	*/
	void edit_asset(/* need input parameters... */);

	/* GET /asset/{id} */
	void get_asset_edit(int asset_id);

	/* POST /asset/edit/{id}/review */
	string review_asset_edit(int asset_id);

	/* POST /asset/edit/{id}/accept */
	string accept_asset_edit(int asset_id);	// ...for moderators

	/* POST /asset/edit/{id}/reject */
	string reject_asset_edit(int asset_id);
}