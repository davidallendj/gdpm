
#include "http.hpp"
#include "utils.hpp"
#include "log.hpp"
#include "error.hpp"
#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/multi.h>
#include <memory>
#include <stdio.h>
#include <chrono>
#include <type_traits>


namespace gdpm::http{

	context::context(){
		curl_global_init(CURL_GLOBAL_ALL);
		curl = curl_easy_init();
	}


	context::~context(){
		curl_easy_cleanup(curl);
		curl_global_cleanup();
	}


	string context::url_escape(const string &url){
		return curl_easy_escape(curl, url.c_str(), url.size());;
	}


	response context::request(
		const string& url,
		const http::request& params
	){
		CURLcode res;
		utils::memory_buffer buf = utils::make_buffer();
		utils::memory_buffer data = utils::make_buffer();
		response r;

#if (GDPM_DELAY_HTTP_REQUESTS == 1)
		using namespace std::chrono_literals;
		utils::delay();
#endif
		if(curl){
			curl_slist *list = add_headers(curl, params.headers);
			if(params.method == method::POST){
				string h;
				std::for_each(
					params.headers.begin(),
					params.headers.end(),
					[&h](const string_pair& kv){
						h += kv.first + "=" + kv.second + "&";
					}
				);
				h.pop_back();
				h = url_escape(h);
				curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, h.size());
				curl_easy_setopt(curl, CURLOPT_POSTFIELDS, h.c_str());
			}
			else if(params.method == method::GET){
				curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
			}
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&buf);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_buffer);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
			curl_easy_setopt(curl, CURLOPT_XFERINFODATA, (void*)&data);
			curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, show_download_progress);
			curl_easy_setopt(curl, CURLOPT_USERAGENT, constants::UserAgent.c_str());
			curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, params.timeout);
			res = curl_easy_perform(curl);
			curl_slist_free_all(list);
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &r.code);
			if(res != CURLE_OK && params.verbose > 0)
				log::error(ec::LIBCURL_ERR,
					std::format("http::context::request::curl_easy_perform(): {}", curl_easy_strerror(res))
				);
			curl_easy_cleanup(curl);
		}

		r.body = buf.addr;
		utils::free_buffer(buf);
		return r;
	}


	response context::download_file(
		const string& url, 
		const string& storage_path, 
		const http::request& params
	){
		// CURL *curl = nullptr;
		CURLcode res;
		response r;
		FILE *fp;

#if (GDPM_DELAY_HTTP_REQUESTS == 1)
		using namespace std::chrono_literals;
		utils::delay();
#endif
		if(curl){
			fp = fopen(storage_path.c_str(), "wb");
			utils::memory_buffer *data;
			curl_slist *list = add_headers(curl, params.headers);
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
			curl_easy_setopt(curl, CURLOPT_HEADER, 0);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_stream);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
			curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &data);
			curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, show_download_progress);
			curl_easy_setopt(curl, CURLOPT_USERAGENT, constants::UserAgent.c_str());
			curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, params.timeout);
			res = curl_easy_perform(curl);
			curl_slist_free_all(list);

			/* Get response code, process error, save data, and close file. */
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &r.code);
			if(res != CURLE_OK && params.verbose > 0){
				log::error(ec::LIBCURL_ERR,
					std::format("http::context::download_file::curl_easy_perform() failed: {}", curl_easy_strerror(res))
				);
			}
			fclose(fp);
		}
		return r;
	}

	long context::get_download_size(const string& url){
		CURLcode res;
		if(curl){
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			res = curl_easy_perform(curl);
			if(!res){
				curl_off_t cl;
				res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &cl);
				if(!res){
					log::debug("download size: {}", cl);
				}
				return cl;
			}
		}
		return -1;
	}


	long context::get_bytes_downloaded(const string& url){
		CURLcode res;
		if(curl){
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			res = curl_easy_perform(curl);
			if(!res){
				curl_off_t dl;
				res = curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD_T, &dl);
				if(!res){
					/* TODO: Integrate the `indicators` progress bar here. */
				}
			}
		}
		return -1;
	}


	multi::multi(long max_allowed_transfers){
		curl_global_init(CURL_GLOBAL_ALL);
		if(max_allowed_transfers > 1)
			cm = curl_multi_init();
			curl_multi_setopt(cm, CURLMOPT_MAXCONNECTS, (long)max_allowed_transfers);
	}

	multi::~multi(){
		if(cm != nullptr)
			curl_multi_cleanup(cm);
		curl_global_cleanup();
	}


	string multi::url_escape(const string &url){
		return curl_easy_escape(cm, url.c_str(), url.size());;
	}


	ptr<transfers> multi::make_requests(
		const string_list& urls,
		const http::request& params
	){
		if(cm == nullptr){
			log::error(error(PRECONDITION_FAILED, 
				"http::multi::make_downloads(): multi client not initialized.")
			);
			return std::make_unique<transfers>();
		}
		if(urls.size() <= 0){
			log::warn("No requests to make.");
			return std::make_unique<transfers>();
		}
		ptr<transfers> ts = std::make_unique<transfers>();
		for(const auto& url : urls){
			transfer t;
			if(t.curl){
				curl_slist *list = add_headers(t.curl, params.headers);
				if(params.method == method::POST){
					string h;
					std::for_each(
						params.headers.begin(),
						params.headers.end(),
						[&h](const string_pair& kv){
							h += kv.first + "=" + kv.second + "&";
						}
						
					);
					h.pop_back();
					h = url_escape(h);
					curl_easy_setopt(t.curl, CURLOPT_POSTFIELDSIZE, h.size());
					curl_easy_setopt(t.curl, CURLOPT_POSTFIELDS, h.c_str());
				}
				else if(params.method == method::GET){
					curl_easy_setopt(t.curl, CURLOPT_CUSTOMREQUEST, "GET");
				}
				curl_easy_setopt(t.curl, CURLOPT_URL, url.c_str());
				curl_easy_setopt(t.curl, CURLOPT_WRITEDATA, (void*)&t.data);
				curl_easy_setopt(t.curl, CURLOPT_WRITEFUNCTION, write_to_buffer);
				curl_easy_setopt(t.curl, CURLOPT_NOPROGRESS, false);
				curl_easy_setopt(t.curl, CURLOPT_XFERINFODATA, &t.data);
				curl_easy_setopt(t.curl, CURLOPT_XFERINFOFUNCTION, show_download_progress);
				curl_easy_setopt(t.curl, CURLOPT_USERAGENT, constants::UserAgent.c_str());
				curl_easy_setopt(t.curl, CURLOPT_TIMEOUT_MS, params.timeout);
			}
		}

		return ts;
	}


	ptr<transfers> multi::make_downloads(
		const string_list& urls,
		const string_list& storage_paths,
		const http::request& params
	){
		if(cm == nullptr){
			log::error(error(ec::PRECONDITION_FAILED, 
				"http::multi::make_downloads(): multi client not initialized.")
			);
			return std::make_unique<transfers>();
		}
		if(urls.size() != storage_paths.size()){
			log::error(error(ec::ASSERTION_FAILED, 
				"http::context::make_downloads(): urls.size() != storage_paths.size()"
			));
		}
		ptr<transfers> ts = std::make_unique<transfers>();
		for(size_t i = 0; i < urls.size(); i++){
			const string& url 			= urls.at(i);
			const string& storage_path 	= storage_paths.at(i);
			response r;
			transfer t;
			t.id = i;
			
			if(t.curl){
				t.fp = fopen(storage_path.c_str(), "wb");
				curl_slist *list = add_headers(t.curl, params.headers);
				curl_easy_setopt(t.curl, CURLOPT_URL, url.c_str());
				// curl_easy_setopt(t.curl, CURLOPT_PRIVATE, url.c_str());
				curl_easy_setopt(t.curl, CURLOPT_FAILONERROR, true);
				curl_easy_setopt(t.curl, CURLOPT_HEADER, 0);
				curl_easy_setopt(t.curl, CURLOPT_FOLLOWLOCATION, true);
				curl_easy_setopt(t.curl, CURLOPT_WRITEDATA, t.fp);
				curl_easy_setopt(t.curl, CURLOPT_WRITEFUNCTION, write_to_stream);
				curl_easy_setopt(t.curl, CURLOPT_NOPROGRESS, false);
				curl_easy_setopt(t.curl, CURLOPT_XFERINFODATA, &t.data);
				curl_easy_setopt(t.curl, CURLOPT_XFERINFOFUNCTION, show_download_progress);
				curl_easy_setopt(t.curl, CURLOPT_USERAGENT, constants::UserAgent.c_str());
				curl_easy_setopt(t.curl, CURLOPT_TIMEOUT_MS, params.timeout);
				cres = curl_multi_add_handle(cm, t.curl);
				curl_slist_free_all(list);
				if(cres != CURLM_OK){
					log::error(ec::LIBCURL_ERR,
						std::format("http::context::make_downloads(): {}", curl_multi_strerror(cres))
					);
				}
				ts->emplace_back(std::move(t));
				/* NOTE: Should the file pointer be closed here? */
			}
		}

		return ts;
	}


	ptr<responses> multi::execute(
		ptr<transfers> transfers,
		size_t timeout
	){
		if(cm == nullptr){
			log::error(error(PRECONDITION_FAILED, 
				"http::multi::execute(): multi client not initialized")
			);
			return std::make_unique<responses>();
		}
		if(transfers->empty()){
			log::debug("http::multi::execute(): no transfers found");
			return std::make_unique<responses>();
		}
		size_t transfers_left = transfers->size();
		ptr<responses> responses = std::make_unique<http::responses>(transfers->size());
		do{
			int still_alive = 1;
			cres = curl_multi_perform(cm, &still_alive);

			while((cmessage = curl_multi_info_read(cm, &messages_left))){
				if(cmessage->msg == CURLMSG_DONE){
					char *url = nullptr;
					transfer& t = transfers->at(transfers_left-1);
					response& r = responses->at(transfers_left-1);
					t.curl = cmessage->easy_handle;
					curl_easy_getinfo(cmessage->easy_handle, CURLINFO_EFFECTIVE_URL, &url);
					curl_easy_getinfo(cmessage->easy_handle, CURLINFO_RESPONSE_CODE, &r.code);
					if((int)cmessage->data.result != CURLM_OK){
						log::error(error(ec::LIBCURL_ERR,
							std::format("http::context::execute({}): {} <url: {}>", (int)cmessage->data.result, curl_easy_strerror(cmessage->data.result), url))
						);
					}
					curl_multi_remove_handle(cm, t.curl);
					curl_easy_cleanup(t.curl);
					if(t.fp) fclose(t.fp);
					transfers->pop_back();
					transfers_left -= 1;
				}
				else{
					log::error(error(ec::LIBCURL_ERR,
						std::format("http::context::execute(): {}", (int)cmessage->msg))
					);
				}
			}
			if(transfers_left)
				curl_multi_wait(cm, NULL, 0, timeout, NULL);
		}while(transfers_left);
		return responses;
	}


	curl_slist* add_headers(
		CURL *curl,
		const headers_t& headers
	){
		struct curl_slist *list = NULL;
		if(!headers.empty()){
			for(const auto& header : headers){
				string h = header.first + ": " + header.second;
				list = curl_slist_append(list, h.c_str());
			}
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
		}
		return list;
	}


	size_t write_to_buffer(
		char *contents, 
		size_t size, 
		size_t nmemb, 
		void *userdata
	){
		size_t realsize = size * nmemb;
		utils::memory_buffer *m = (utils::memory_buffer*)userdata;

		m->addr = (char*)realloc(m->addr, m->size + realsize + 1);
		if(m->addr == nullptr){
			/* Out of memory */
			log::error("Could not allocate memory (realloc returned NULL).");
			return 0;
		}

		memcpy(&(m->addr[m->size]), contents, realsize);
		m->size += realsize;
		m->addr[m->size] = 0;

		return realsize;
	}


	size_t write_to_stream(
		char *ptr, 
		size_t size, 
		size_t nmemb, 
		void *userdata
	){
		if(nmemb == 0)
			return 0;
					
		return fwrite(ptr, size, nmemb, (FILE*)userdata);
	}


	int show_download_progress(
		void *ptr,
		curl_off_t total_download,
		curl_off_t current_downloaded,
		curl_off_t total_upload,
		curl_off_t current_upload
	){
		if(current_downloaded >= total_download)
			return 0;
		using namespace indicators;
		show_console_cursor(false);
		// if(total_download != 0){
		// 	// double percent = std::floor((current_downloaded / (total_download)) * 100);
		// 	bar.set_option(option::MaxProgress{total_download});
		// 	// bar.set_option(option::HideBarWhenComplete{false});
		// 	bar.set_progress(current_downloaded);
		// 	bar.set_option(option::PostfixText{
		// 			utils::convert_size(current_downloaded) + " / " + 
		// 			utils::convert_size(total_download)
		// 	});
		// 	if(bar.is_completed()){
		// 		bar.set_option(option::PrefixText{"Download completed."});
		// 		bar.mark_as_completed();
		// 	}
		// } else {
		// 	if(bar_unknown.is_completed()){
		// 		bar_unknown.set_option(option::PrefixText{"Download completed."});
		// 		bar_unknown.mark_as_completed();
		// 	} else {
		// 		bar.tick();
		// 		bar_unknown.set_option(
		// 			option::PostfixText(std::format("{}", utils::convert_size(current_downloaded)))
		// 		);

		// 	}
		// }
		show_console_cursor(true);
		utils::memory_buffer *m = (utils::memory_buffer*)ptr;
		return 0;
	}
}