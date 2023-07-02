
#include "config.hpp"
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
	
	request_params make_from_config(const config::context& config){
		request_params rp = config.rest_api_params;
		rp.verbose = config.verbose;
		return rp;
	}

	request_params make_request_params(
		type_e 			type, 
		int 			category, 
		support_e 		support, 
		const string& 	user, 
		const string& 	godot_version, 
		int 			max_results, 
		int 			page, 
		sort_e 			sort, 
		bool 			reverse, 
		int 			verbose
	){
		return request_params{
			.type 			= type,
			.category 		= category,
			.support 		= support,
			.user 			= user,
			.godot_version 	= godot_version,
			.max_results 	= max_results,
			.page 			= page,
			.sort 			= sort,
			.reverse 		= reverse,
			.verbose 		= verbose
		};
	}

	bool register_account(
		const string& username, 
		const string& password, 
		const string& email
	){
		return false;
	}

	bool login(
		const string& username, 
		const string& password
	){
		return false;
	}

	bool logout(){
		return false;
	}

	rapidjson::Document _parse_json(
		const string& r, 
		int verbose
	){
		using namespace rapidjson;
		Document d;
		d.Parse(r.c_str());

		StringBuffer buffer;
		PrettyWriter<StringBuffer> writer(buffer);
		d.Accept(writer);
		
		if(verbose > log::INFO)
			log::info("JSON Response: \n{}", buffer.GetString());
		return d;
	}

	string to_json(const rapidjson::Document& doc){
		return doc.GetString();
	}

	string to_string(type_e type){
		string _s{"type="};
		switch(type){
			case any:		_s += "any"; 		break;
			case addon:		_s += "addon";		break;
			case project:	_s += "project";	break;
		}
		return _s;
	}

	string to_string(support_e support){
		string _s{"support="};
		switch(support){
			case all:		_s += "official+community+testing";	break;
			case official:	_s += "official";	break;
			case community:	_s += "community";	break;
			case testing:	_s += "testing";	break;
		}
		return _s;
	}

	string to_string(sort_e sort){
		string _s{"sort="};
		switch(sort){
			case none:		_s += "";			break;
			case rating:	_s += "rating";		break;
			case cost:		_s += "cost";		break;
			case name:		_s += "name";		break;
			case updated:	_s += "updated";	break;
		}
		return _s;
	}

	string _prepare_request(
		const string& url, 
		const request_params &c,
		const string& filter
	){
		string request_url{url};
		request_url += to_string(static_cast<type_e>(c.type));
		request_url += (c.category <= 0) ? "&category=" : "&category="+std::to_string(c.category);
		request_url += "&" + to_string(static_cast<support_e>(c.support));
		request_url += "&" + to_string(static_cast<sort_e>(c.sort));
		request_url += (!filter.empty()) ? "&filter="+filter : "";
		request_url += (!c.godot_version.empty()) ? "&godot_version="+c.godot_version : "";
		request_url += "&max_results=" + std::to_string(c.max_results);
		request_url += "&page=" + std::to_string(c.page);
		request_url += (c.reverse) ? "&reverse" : "";
		return request_url;
	}

	void _print_params(const request_params& params, const string& filter){
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
			filter, 
			params.godot_version,
			params.max_results
		);
	}

	rapidjson::Document configure(
		const string& url, 
		type_e type, 
		int verbose
	){
		http::context http;
		string request_url{url};
		request_url += to_string(type);
		http::response r = http.request_get(url);
		if(verbose > 0)
			log::info("URL: {}", url);
		return _parse_json(r.body);
	}

	rapidjson::Document get_assets_list(
		const string& url, 
		type_e type, 
		int category, 
		support_e support, 
		const string& filter,
		const string& user, 
		const string& godot_version, 
		int max_results, 
		int page, 
		sort_e sort, 
		bool reverse, 
		int verbose
	){
		request_params c{
			.type 			= type,
			.category 		= category,
			.support 		= support,
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

	rapidjson::Document get_assets_list(
		const string& url, 
		const request_params& c,
		const string& filter
	){
		http::context http;
		http::request_params http_params;
		// http_params.headers.insert(http::header("Accept", "*/*"));
		// http_params.headers.insert(http::header("Accept-Encoding", "application/gzip"));
		// http_params.headers.insert(http::header("Content-Encoding", "application/gzip"));
		// http_params.headers.insert(http::header("Connection", "keep-alive"));
		string request_url = _prepare_request(url, c, http.url_escape(filter));
		http::response r = http.request_get(request_url, http_params);
		if(c.verbose >= log::INFO)
			log::info("get_asset().URL: {}", request_url);
		return _parse_json(r.body, c.verbose);
	}


	rapidjson::Document get_asset(
		const string& url, 
		int asset_id, 
		const rest_api::request_params& api_params,
		const string& filter
	){
		/* Set up HTTP request */
		http::context http;
		http::request_params http_params;
		// http_params.headers.insert(http::header("Accept", "*/*"));
		// http_params.headers.insert(http::header("Accept-Encoding", "application/gzip"));
		// http_params.headers.insert(http::header("Content-Encoding", "application/gzip"));
		// http_params.headers.insert(http::header("Connection", "keep-alive"));
		string request_url = utils::replace_all(_prepare_request(url, api_params, http.url_escape(filter)), "{id}", std::to_string(asset_id));
		http::response r = http.request_get(request_url.c_str(), http_params);
		if(api_params.verbose >= log::INFO)
			log::info("get_asset().URL: {}", request_url);
		
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

		string review_asset_edit(int asset_id){
			return string();
		}

		string accept_asset_edit(int asset_id){
			return string();
		}

		string reject_asset_edit(int asset_id){
			return string();
		}

	} // namespace edits
}