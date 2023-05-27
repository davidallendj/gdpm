# GDPM Rest API Example

This is an example showing how to use the REST API designed to query the Godot Asset library in C++. It is built using the `libcurl` library.

Here is a snippet making a HTTP get and post request.

```c++
using string 	= std::string;
using headers_t = std::unordered_map<string, string>;

std::string url = "www.example.com";
http::response r_get = http::request_get(url)
if(r_get.response_code == http::response_code::OK){
	// ...do something...
}

http::request_params params;
params.headers = {
	{"user-agent", "firefox"},
	{"content-type", "application/json"}
}
http::response r_post = http::request_post(url, params);
if(r_post.response_code == http::response_code::OK){
	// ...do something...
}
```