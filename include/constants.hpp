#pragma once

#include <rapidjson/rapidjson.h>
#include <string>
#include <string_view>

namespace gdpm::constants{
	const std::string RemoteName(std::string("origin"));
	const std::string HomePath(std::string(std::getenv("HOME")) + "/");
	const std::string TestPath(HomePath + ".config/gdpm/tests");
	const std::string ConfigPath(HomePath + ".config/gdpm/config.json");
	const std::string LockfilePath(HomePath + ".config/gdpm/gdpm.lck");
	const std::string LocalPackagesDir(HomePath + ".config/gdpm/packages");
	const std::string TemporaryPath(HomePath + ".config/gdpm/tmp");
	const std::string UserAgent("libcurl-agent/1.0 via GDPM (https://github.com/davidallendj/gdpm)");
	const std::string AssetRepo("https://godotengine.org/asset-library/api/asset");
	const std::string HostUrl("https://godotengine.org/asset-library/api");
	constexpr std::string WHITESPACE = " \n\r\t\f\v";
}

namespace gdpm::print{
	enum class style{
		list = 0,
		table = 1,
	};
}

/* Define default macros to set when building with -DGPM_* */
#define GDPM_CONFIG_USERNAME ""
#define GDPM_CONFIG_PASSWORD ""
#define GDPM_CONFIG_PATH gdpm::constants::ConfigPath
#define GDPM_CONFIG_TOKEN ""
#define GDPM_CONFIG_GODOT_VERSION "3.4"
#define GDPM_CONFIG_LOCAL_PACKAGES_DIR gdpm::constants::LocalPackagesDir
#define GDPM_CONFIG_LOCAL_TMP_DIR gdpm::constants::TemporaryPath
#define GDPM_CONFIG_REMOTE_SOURCES std::pair<std::string, std::string>(constants::RemoteName, constants::HostUrl)
#define GDPM_CONFIG_THREADS 1
#define GDPM_CONFIG_TIMEOUT_MS 30000
#define GDPM_CONFIG_ENABLE_SYNC true
#define GDPM_CONFIG_ENABLE_FILE_LOGGING true
#define GDPM_CONFIG_VERBOSE 0

/* Defines the default package cache for local storage */
#define GDPM_PACKAGE_CACHE_ENABLE 1
#define GDPM_PACKAGE_CACHE_PATH gdpm::constants::LocalPackagesDir + "/packages.db"
#define GDPM_PACKAGE_CACHE_TABLENAME "cache"
#define GDPM_PACKAGE_CACHE_COLNAMES "asset_id, type, title, author, author_id, version, godot_version, cost, description, modify_date, support_level, category, remote_source, download_url, download_hash, is_installed, install_path"

/* Define macros to set default assets API params */
#define GDPM_DEFAULT_ASSET_TYPE any
#define GDPM_DEFAULT_ASSET_CATEGORY 0
#define GDPM_DEFAULT_ASSET_SUPPORT all
#define GDPM_DEFAULT_ASSET_FILTER ""
#define GDPM_DEFAULT_ASSET_USER ""
#define GDPM_DEFAULT_ASSET_GODOT_VERSION "3.4"
#define GDPM_DEFAULT_ASSET_MAX_RESULTS 500
#define GDPM_DEFAULT_ASSET_PAGE 0
#define GDPM_DEFAULT_ASSET_SORT none
#define GDPM_DEFAULT_ASSET_REVERSE false
#define GDPM_DEFAULT_ASSET_VERBOSE 0

/* Define misc. macros */
#if defined(_WIN32)
	#define GDPM_DLL_EXPORT __declspec(dllexport)
	#define GDPM_DLL_IMPORT __declspec(dllimport)
#else
	#define GDPM_DLL_EXPORT
	#define GDPM_DLL_IMPORT
#endif

#define GDPM_READFILE_IMPL 1
#define GDPM_DELAY_HTTP_REQUESTS 1

#ifndef GDPM_REQUEST_DELAY
	#define GDPM_REQUEST_DELAY 200ms 
#endif

#ifndef GDPM_ENABLE_COLORS
	#define GDPM_ENABLE_COLORS 1
#endif

#ifndef GDPM_LOG_LEVEL
	#define GDPM_LOG_LEVEL 1
#endif

#ifndef GDPM_ENABLE_TIMESTAMPS
	#define GDPM_ENABLE_TIMESTAMPS 1
#endif

#ifndef GDPM_TIMESTAMP_FORMAT
	#define GDPM_TIMESTAMP_FORMAT ":%I:%M:%S %p; %Y-%m-%d"
#endif
