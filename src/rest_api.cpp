
#include "rest_api.hpp"
#include "constants.hpp"
#include "http.hpp"
#include "log.hpp"
#include "utils.hpp"
#include <curl/curl.h>
#include <list>
#include <string>
#include <ostream>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>

namespace gdpm::rest_api{
	bool register_account(const std::string& username, const std::string& password, const std::string& email){
		return false;
	}

	bool login(const std::string& username, const std::string& password){
		return false;
	}

	bool logout(){
		return false;
	}

	context make_context(type_e type, int category, support_e support, const std::string& filter, const std::string& user, const std::string& godot_version, int max_results, int page, sort_e sort, bool reverse, int verbose){
		context params{
			.type = type,
			.category = category,
			.support = support,
			.filter = filter,
			.user = user,
			.godot_version = godot_version,
			.max_results = max_results,
			.page = page,
			.sort = sort,
			.reverse = reverse,
			.verbose = verbose
		};
		return params;
	}

	rapidjson::Document _parse_json(const std::string& r, int verbose){
		using namespace rapidjson;
		Document d;
		d.Parse(r.c_str());

		StringBuffer buffer;
		PrettyWriter<StringBuffer> writer(buffer);
		d.Accept(writer);
		
		if(verbose > 1)
			log::info("JSON Response: \n{}", buffer.GetString());
		return d;
	}

	std::string to_string(type_e type){
		std::string _s{"type="};
		switch(type){
			case any:		_s += "any"; 		break;
			case addon:		_s += "addon";		break;
			case project:	_s += "project";	break;
		}
		return _s;
	}

	std::string to_string(support_e support){
		std::string _s{"support="};
		switch(support){
			case all:		_s += "official+community+testing";	break;
			case official:	_s += "official";	break;
			case community:	_s += "community";	break;
			case testing:	_s += "testing";	break;
		}
		return _s;
	}

	std::string to_string(sort_e sort){
		std::string _s{"sort="};
		switch(sort){
			case none:		_s += "";			break;
			case rating:	_s += "rating";		break;
			case cost:		_s += "cost";		break;
			case name:		_s += "name";		break;
			case updated:	_s += "updated";	break;
		}
		return _s;
	}

	std::string _prepare_request(const std::string &url, const context &c){
		std::string request_url{url};
		request_url += to_string(c.type);
		request_url += (c.category <= 0) ? "&category=" : "&category="+fmt::to_string(c.category);
		request_url += "&" + to_string(c.support);
		request_url += "&" + to_string(c.sort);
		request_url += (!c.filter.empty()) ? "&filter="+c.filter : "";
		request_url += (!c.godot_version.empty()) ? "&godot_version="+c.godot_version : "";
		request_url += "&max_results=" + fmt::to_string(c.max_results);
		request_url += "&page=" + fmt::to_string(c.page);
		request_url += (c.reverse) ? "&reverse" : "";
		return request_url;
	}

	void _print_params(const context& params){
		log::println("params: \n"
			"\ttype: {}\n"
			"\tcategory: {}\n"
			"\tsupport: {}\n"
			"\tfilter: {}\n"
			"\tgodot version: {}\n" 
			"\tmax results: {}\n",
			params.type,
			params.category,
			params.support,
			params.filter, 
			params.godot_version,
			params.max_results
		);
	}

	rapidjson::Document configure(const std::string& url, type_e type, int verbose){
		std::string request_url{url};
		request_url += to_string(type);
		http::response r = http::request_get(url);
		if(verbose > 0)
			log::info("URL: {}", url);
		return _parse_json(r.body);
	}

	rapidjson::Document get_assets_list(const std::string& url, type_e type, int category, support_e support, const std::string& filter,const std::string& user, const std::string& godot_version, int max_results, int page, sort_e sort, bool reverse, int verbose){
		context c{
			.type 			= type,
			.category 		= category,
			.support 		= support,
			.filter 		= filter,
			.user 			= user,
			.godot_version 	= godot_version,
			.max_results 	= max_results,
			.page 			= page,
			.sort 			= sort,
			.reverse 		= reverse,
			.verbose 		= verbose
		};
		return get_assets_list(url, c);
	}

	rapidjson::Document get_assets_list(const std::string& url, const context& c){
		std::string request_url = _prepare_request(url, c);
		http::response r = http::request_get(request_url);
		if(c.verbose > 0)
			log::info("URL: {}", request_url);
		return _parse_json(r.body, c.verbose);
	}

	rapidjson::Document get_asset(const std::string& url, int asset_id, const context& params){
		std::string request_url = _prepare_request(url, params);
		utils::replace_all(request_url, "{id}", std::to_string(asset_id));
		http::response r = http::request_get(request_url.c_str());
		if(params.verbose > 0)
			log::info("URL: {}", request_url);
		return _parse_json(r.body);
	}

	bool delete_asset(int asset_id){
		return false;
	}

	bool undelete_asset(int asset_id){
		return false;
	}

	bool set_support_level(int asset_id){
		return false;
	}

	namespace edits{

		void edit_asset(){

		}

		void get_asset_edit(int asset_id){

		}

		std::string review_asset_edit(int asset_id){
			return std::string();
		}

		std::string accept_asset_edit(int asset_id){
			return std::string();
		}

		std::string reject_asset_edit(int asset_id){
			return std::string();
		}

	} // namespace edits
}