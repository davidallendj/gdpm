
#include "config.hpp"
#include "error.hpp"
#include "package.hpp"
#include "rest_api.hpp"
#include "constants.hpp"
#include "http.hpp"
#include "log.hpp"
#include "utils.hpp"
#include <curl/curl.h>
#include <list>
#include <string>
#include <ostream>


namespace gdpm::rest_api{
	
	request_params make_from_config(const config::context& config){
		request_params rp = config.rest_api_params;
		rp.verbose = config.verbose;
		return rp;
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
		const request_params& c,
		const string& filter
	){
		string request_url{url};
		request_url += to_string(static_cast<type_e>(std::clamp(c.type, 0, 2)));
		request_url += (c.category <= 0) ? "" : "&category="+std::to_string(c.category);
		request_url += "&" + to_string(static_cast<support_e>(std::clamp(c.support, 0, 3)));
		request_url += (c.sort <= 0) ? "" : "&" + to_string(static_cast<sort_e>(std::clamp(c.sort, 1, 4)));
		request_url += (!filter.empty()) ? "&filter="+filter : "";
		request_url += (!c.user.empty()) ? "&user="+c.user : "";
		request_url += (!c.cost.empty()) ? "&cost="+c.cost : "";
		request_url += (!c.godot_version.empty()) ? "&godot_version="+c.godot_version : "";
		request_url += (c.max_results <= 0 || c.max_results > 500) ? "" : "&max_results=" + std::to_string(c.max_results);
		request_url += (c.page <= 0) ? "" : "&page=" + std::to_string(c.page);
		request_url += (c.reverse) ? "&reverse" : "";
		return request_url;
	}

	error print_params(
		const request_params& params, 
		const string& filter
	){
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
		return error();
	}

	error print_asset(
		const rest_api::request_params& rest_api_params,
		const string& filter,
		const print::style& style
	){
		using namespace rapidjson;
			string request_url{constants::HostUrl + rest_api::endpoints::GET_Asset};
			Document doc = rest_api::get_assets_list(request_url, rest_api_params, filter);
			if(doc.IsNull()){
				return log::error_rc(error(
					ec::HOST_UNREACHABLE,
					"Could not fetch metadata. Aborting."
				));
			}
		if(style == print::style::list)
			package::print_list(doc);
		if(style == print::style::table)
			package::print_table(doc);
		return error();
	}

	rapidjson::Document configure(
		const string& url, 
		type_e type, 
		int verbose
	){
		http::context http;
		string request_url{url};
		request_url += to_string(type);
		http::response r = http.request(url);
		if(verbose > 0)
			log::info("rest_api::configure::url: {}", url);
		return _parse_json(r.body);
	}


	rapidjson::Document get_assets_list(
		const string& url, 
		const request_params& c,
		const string& filter
	){
		http::context http;
		http::request params;
		params.headers.insert(http::header("Accept", "*/*"));
		params.headers.insert(http::header("Accept-Encoding", "application/gzip"));
		params.headers.insert(http::header("Content-Encoding", "application/gzip"));
		params.headers.insert(http::header("Connection", "keep-alive"));
		string prepared_url = _prepare_request(url, c, http.url_escape(filter));
		http::response r = http.request(prepared_url, params);
		if(c.verbose >= log::INFO)
			log::info("rest_api::get_asset_list()::url: {}", prepared_url);
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
		http::request params;
		params.headers.insert(http::header("Accept", "*/*"));
		params.headers.insert(http::header("Accept-Encoding", "application/gzip"));
		params.headers.insert(http::header("Content-Encoding", "application/gzip"));
		params.headers.insert(http::header("Connection", "keep-alive"));
		string prepared_url = utils::replace_all(
			_prepare_request(url, api_params, 
				http.url_escape(filter)
			), "{id}", std::to_string(asset_id));
		http::response r = http.request(prepared_url, params);
		if(api_params.verbose >= log::INFO)
			log::info("rest_api::get_asset()::url: {}", prepared_url);
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


	namespace multi{
		json::documents get_assets(
			const string_list& urls,
			id_list asset_ids,
			const request_params& api_params,
			const string_list& filters
		){
			if(urls.size() == asset_ids.size() && urls.size() == filters.size()){
				log::error(error(ec::ASSERTION_FAILED,
					"multi::get_assets(): urls.size() != filters.size()"));
			}
			http::context http(4);
			http::request params;
			json::documents docs;
			params.headers.insert(http::header("Accept", "*/*"));
			params.headers.insert(http::header("Accept-Encoding", "application/gzip"));
			params.headers.insert(http::header("Content-Encoding", "application/gzip"));
			params.headers.insert(http::header("Connection", "keep-alive"));
			string_list prepared_urls = {};
			
			/* Prepare the URLs for the request_multi() call z*/
			for(size_t i = 0; i < urls.size(); i++){
				const string& url = urls.at(i);
				const string& filter = filters.at(i);
				int asset_id = asset_ids.at(i);
				string prepared_url = utils::replace_all(
						_prepare_request(url, api_params, http.url_escape(filter)),
							"{id}", std::to_string(asset_id));
				prepared_urls.emplace_back(prepared_url);
				if(api_params.verbose >= log::INFO)
					log::info("get_assets(i={})::url: {}", i, prepared_url);
			}
			
			/* Parse JSON string into objects */
			http::responses responses = http.requests(prepared_urls, params);
			for(const auto& response : responses){
				docs.emplace_back(_parse_json(response.body));
			}
			return docs;
		}
	}

	namespace edits{
		void edit_asset(){}
		void get_asset_edit(int asset_id){}
		string review_asset_edit(int asset_id){ return string(); }
		string accept_asset_edit(int asset_id){ return string(); }
		string reject_asset_edit(int asset_id){ return string(); }
	} // namespace edits
}