
project(
	'gdpm', 
	'cpp',
	default_options: ['cpp_std=c++20', 'build.cpp_std=c++20']
)
deps = [
	dependency('Threads'),
	dependency('RapidJSON'),
	dependency('fmt'),
	dependency('Catch2'),
	dependency('cxxopts'),
	# dependency('curl'),
	dependency('curlpp'),
	dependency('libzip'),
	dependency('sqlite3')
]
includes = include_directories('include')
src = [
	'src/config.cpp',
	'src/package_manager.cpp',
	'src/rest_api.cpp',
	'src/utils.cpp',
	'src/http.cpp',
	'src/cache.cpp'
]
# cpp = meson.get_compiler('cpp')
# cpp.find_library()
cpp_args = [
	'-Ofast',
	# '-fPIC',
	# '-fPIE',
	# '-fconcepts',
	# '-fsanitize=address',
	'-fpermissive',
	'-Wall',
	'-Wno-switch',
	'-Wno-unused-variable',
	'-Wno-sign-conversion',
	'-Wno-unused-function',
	'-pedantic-errors',
	# '-lcurl',
	# '-lzip'
	'-DGDPM_LOG_LEVEL=2',
	'-DGDPM_REQUEST_DELAY=200ms',
	'-DGDPM_ENABLE_COLORS=1',
	'-DGDPM_ENABLE_TIMESTAMPS=1',
]
lib = shared_library(
	meson.project_name(), 
	src, 
	dependencies: deps,
	version: '1.0', 
	soversion: '0',
	include_directories: includes,
	cpp_args: cpp_args
)
exe = executable(
	meson.project_name(),
	'src/main.cpp',
	dependencies: deps,
	include_directories: includes,
	link_with: lib,
	cpp_args: cpp_args
)
test('unittests', exe)