#pragma once

#include <string>

namespace gdpm::constants{
	constexpr const char *ConfigPath = "config.ini";
	constexpr const char *LocalPackagesDir = "$HOME/.config/gdpm";
	constexpr const char *UserAgent = "libcurl-agent/1.0";
	constexpr const char *AssetRepo = "https://godotengine.org/asset-library/api/asset";
	constexpr const char *HostUrl = "https://godotengine.org/asset-library/api";
	constexpr const char *LockfilePath = "$HOME/.config/gdpm/gdpm.lck";
	constexpr const char *TmpPath = "$HOME/.config/gdpm/tmp";
}

/* Defines to set when building with -DGPM_* */
#define GDPM_CONFIG_USERNAME ""
#define GDPM_CONFIG_PASSWORD ""
#define GDPM_CONFIG_PATH "config.json"
#define GDPM_CONFIG_TOKEN ""
#define GDPM_CONFIG_GODOT_VERSION "3.4"
#define GDPM_CONFIG_LOCAL_PACKAGES_DIR "tests/gdpm/packages"
#define GDPM_CONFIG_LOCAL_TMP_DIR "tests/gdpm/.tmp"
#define GDPM_CONFIG_REMOTE_SOURCES constants::HostUrl 
#define GDPM_CONFIG_THREADS 1
#define GDPM_CONFIG_TIMEOUT_MS 30000
#define GDPM_CONFIG_ENABLE_SYNC 1
#define GDPM_CONFIG_ENABLE_FILE_LOGGING 0
#define GDPM_CONFIG_VERBOSE 0

/* Defines package cache for local storage */
#define GDPM_PACKAGE_CACHE_ENABLE 1
#define GDPM_PACKAGE_CACHE_PATH "tests/gdpm/packages.db"
#define GDPM_PACKAGE_CACHE_TABLENAME "cache"
#define GDPM_PACKAGE_CACHE_COLNAMES "asset_id, type, title, author, author_id, version, godot_version, cost, description, modify_date, support_level, category, remote_source, download_url, download_hash, is_installed, install_path"

/* Defines to set default assets API params */
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