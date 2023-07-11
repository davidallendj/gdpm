# GDPM HTTP Example

This is an example showing how to use the GDPM HTTP library to download files. The library uses RapidJSON to get results.


Here is a quick snippet to get started:
```c++
// Get a full list of assets
rest_api::context context = rest_api::make_context();
rapidjson::Document doc = http::get_asset_list(
	"https://godotengine.org/asset-library/api",
	context
)
// ... parse the rapidjson::Document
```