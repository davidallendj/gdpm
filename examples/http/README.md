# GDPM Rest API Example

This is an example showing how to use the REST API designed to query the Godot Asset library in C++. It is built using the `libcurl` library.

Here is a snippet making a HTTP get and post request.

```c++
#include "http.hpp"

#include <string>
#include <unordered_map>

using string 	= std::string;
using headers_t = std::unordered_map<string, string>;

string url = "www.example.com";
http::context http;
http::request params = {{"user-agent", "firefox"}};
http::response r = http::request(url, params);
if(r.code == http::OK){ /* ...do something... */ }

r = http::request(url, params, http::method::POST);
if(r.code == http::OK){ /* ...do something.. */ }
```

Here's an example using the `multi` interface for parallel requests and downloads:

```c++
#include "http.hpp"

#include <memory>

template <typename T>
using ptr 		= std::unique_ptr<T>;
using strings 	= std::vector<std::string>;
using transfers = std::vector<http::transfer>;
using responses = std::vector<http::response>;

http::multi http;
http::request params = {{"user-agent", "firefox"}}
params.headers.insert(http::header("Accept", "*/*"));
params.headers.insert(http::header("Accept-Encoding", "application/gzip"));
params.headers.insert(http::header("Content-Encoding", "application/gzip"));
params.headers.insert(http::header("Connection", "keep-alive"));

strings request_urls = {
	"www.example.com",
	"www.google.com"
}
ptr<http::transfers> requests = http::make_requests(request_url, params);
ptr<http::responses> responses = http::execute(std::move(requests));
for(const auto& r : *responses){
	if(r.code == http::OK){
		// ...do something
	}
}

strings download_urls = {
	""
}
strings storage_paths = {
	"./download1"
}
ptr<http::transfers> downloads = http::make_downloads(download_url, storage_paths, params);
ptr<http::responses> responses = http::execute(std::move(downloads));
for(const auto& r : *responses){
	if(r.code == http::OK){
		// ...do something
	}
}
```