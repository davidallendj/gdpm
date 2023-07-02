#pragma once

#include "constants.hpp"
#include "types.hpp"
#include <unordered_map>
#include <curl/curl.h>
#include <curl/easy.h>

namespace gdpm::http{
	using headers_t = std::unordered_map<string, string>;
	using header = std::pair<string, string>;

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

	struct response{
		long code = 0;
		string body{};
		headers_t headers{};
		error error();
	};


	struct request_params {
		headers_t headers = {};
		size_t timeout = GDPM_CONFIG_TIMEOUT_MS;
		int verbose = 0;
	};

	class context : public non_copyable{
	public:
		context();
		~context();

		inline CURL* const get_curl() const;
		string url_escape(const string& url);
		response request_get(const string& url, const http::request_params& params = http::request_params());
		response request_post(const string& url, const http::request_params& params = http::request_params());
		response download_file(const string& url, const string& storage_path, const http::request_params& params = http::request_params());
		long get_download_size(const string& url);
		long get_bytes_downloaded(const string& url);
	
	private:
		CURL *curl;
		curl_slist* _add_headers(CURL *curl, const headers_t& headers);
	};

	

	extern context http;
}