#pragma once

#include "constants.hpp"
#include "types.hpp"
#include "indicators/indeterminate_progress_bar.hpp"
#include "indicators/dynamic_progress.hpp"
#include "indicators/progress_bar.hpp"
#include "indicators/block_progress_bar.hpp"
#include "utils.hpp"
#include <memory>
#include <unordered_map>
#include <curl/curl.h>
#include <curl/easy.h>

namespace gdpm::http{
	using headers_t = std::unordered_map<string, string>;
	using header = std::pair<string, string>;

	enum method{
		GET,
		POST,
		PUT,
		DELETE
	};

	// REF: https://developer.mozilla.org/en-US/docs/Web/HTTP/Status
	enum response_code{
		CONTINUE						= 100,
		SWITCHING_PROTOCOLS 			= 101,
		PROCESSING						= 102,
		EARLY_HINTS						= 103,
		OK 								= 200,
		CREATED 						= 201,
		ACCEPTED 						= 202,
		NON_AUTHORITATIVE_INFORMATION 	= 203,
		NO_CONTENT						= 204,
		RESET_CONTENT 					= 205,
		PARTIAL_CONTENT 				= 206,
		MULTI_STATUS 					= 207,
		ALREADY_REPORTED 				= 208,
		IM_USED 						= 226,
		MULTIPLE_CHOICES 				= 300,
		MOVED_PERMANENTLY 				= 301,
		FOUND 							= 302,
		SEE_OTHER 						= 303,
		NOT_MODIFIED 					= 304,
		USE_PROXY 						= 305,
		UNUSED 							= 306,
		TEMPORARY_REDIRECT 				= 307,
		PERMANENT_REDIRECT 				= 308,
		BAD_REQUEST 					= 400,
		UNAUTHORIZED 					= 401,
		PAYMENT_REQUIRED 				= 402,
		FORBIDDEN 						= 403,
		NOT_FOUND 						= 404,
		METHOD_NOT_ALLOWED 				= 405,
		NOT_ACCEPTABLE 					= 406,
		PROXY_AUTHENTICATION_REQUIRED 	= 407,
		REQUEST_TIMEOUT 				= 408,
		CONFLICT 						= 409,
		GONE 							= 410,
		LENGTH_REQUIRED 				= 411,
		PRECONDITION_FAILED 			= 412,
		PAYLOAD_TOO_LARGE 				= 413,
		URI_TOO_LONG 					= 414,
		UNSUPPORTED_MEDIA_TYPE 			= 415,
		RANGE_NOT_SATISFIABLE 			= 416,
		EXPECTATION_FAILED 				= 417,
		IM_A_TEAPOT 					= 418,
		MISDIRECTED_REQUEST 			= 421,
		UNPROCESSABLE_CONTENT 			= 422,
		LOCKED 							= 423,
		FAILED_DEPENDENCY 				= 424,
		TOO_EARLY 						= 425,
		UPGRADE_REQUIRED 				= 426,
		PRECONDITION_REQUIRED 			= 428,
		TOO_MANY_REQUESTS 				= 429,
		REQUEST_HEADER_FILEDS_TOO_LARGE = 431,
		UNAVAILABLE_FOR_LEGAL_REASONS 	= 451,
		INTERNAL_SERVER_ERROR 			= 500,
		NOT_IMPLEMENTED 				= 501,
		BAD_GATEWAY 					= 502,
		SERVICE_UNAVAILABLE 			= 503,
		GATEWAY_TIMEOUT 				= 504,
		HTTP_VERSION_NOT_SUPPORTED 		= 505,
		VARIANT_ALSO_NEGOTIATES 		= 506,
		INSUFFICIENT_STORAGE 			= 507,
		LOOP_DETECTED 					= 508,
		NOT_EXTENDED 					= 510,
		NETWORK_AUTHENTICATION_REQUIRED = 511
	};

	enum transfer_type{
		REQUEST,
		DOWNLOAD
	};

	struct response{
		long code = 0;
		string body{};
		headers_t headers{};
		error error();
	};


	struct request {
		headers_t headers = {};
		method method = method::GET;
		size_t timeout = GDPM_CONFIG_TIMEOUT_MS;
		int verbose = 0;
	};

	using namespace indicators;
	// BlockProgressBar bar {
	// 	option::BarWidth{50},
	// 	// option::Start{"["},
	// 	// option::Fill{"="},
	// 	// option::Lead{">"},
	// 	// option::Remainder{" "},
	// 	// option::End{"]"},
	// 	option::PrefixText{"Downloading file "},
	// 	option::PostfixText{""},
	// 	option::ForegroundColor{Color::green},
	// 	option::FontStyles{std::vector<FontStyle>{FontStyle::bold}},
	// };
	// 	// option::ShowElapsedTime{true},
	// 	// option::ShowRemainingTime{true},
	// IndeterminateProgressBar bar_unknown {
	// 	option::BarWidth{50},
	// 	option::Start{"["},
	// 	option::Fill{"."},
	// 	option::Lead{"<==>"},
	// 	option::PrefixText{"Downloading file "},
	// 	option::End{"]"},
	// 	option::PostfixText{""},
	// 	option::ForegroundColor{Color::green},
	// 	option::FontStyles{std::vector<FontStyle>{FontStyle::bold}},
	// };

	struct transfer : public non_copyable{
		transfer(){ curl = curl_easy_init(); }
		transfer(transfer&&){}
		~transfer(){ }

		CURLcode res;
		int id;
		CURL *curl = nullptr;
		FILE *fp = nullptr;
		utils::memory_buffer data = {0};

	};
	using transfers = std::vector<transfer>;
	using responses = std::vector<response>;

	class context : public non_copyable{
	public:
		context();
		~context();

		string url_escape(const string& url);
		response request(const string& url, const http::request& params = http::request());
		response download_file(const string& url, const string& storage_path, const http::request& params = http::request());
		long get_download_size(const string& url);
		long get_bytes_downloaded(const string& url);

	private:
		CURL *curl = nullptr;
	};


	class multi{
	public:
		multi(long max_allowed_transfers = 2);
		~multi();
		string url_escape(const string& url);
		ptr<transfers> make_requests(const string_list& urls, const http::request& params = http::request());
		ptr<transfers> make_downloads(const string_list& url, const string_list& storage_path, const http::request& params = http::request());
		ptr<responses> execute(ptr<transfers> transfers, size_t timeout = 1000);
	
	private:
		DynamicProgress<BlockProgressBar> progress_bars;
		CURLM *cm = nullptr;
		CURLMsg *cmessage = nullptr;
		CURLMcode cres;
		int messages_left = -1;
	};

	curl_slist* add_headers(CURL *curl, const headers_t& headers);
	static size_t write_to_buffer(char *contents, size_t size, size_t nmemb, void *userdata);
	static size_t write_to_stream(char *ptr, size_t size, size_t nmemb, void *userdata);
	static int show_download_progress(void *ptr, curl_off_t total_download, curl_off_t current_downloaded, curl_off_t total_upload, curl_off_t current_upload);

}