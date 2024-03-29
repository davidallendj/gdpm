cmake_minimum_required(VERSION 3.12)
project(gdpm LANGUAGES CXX VERSION 0.1.0)


# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
	message(
	FATAL_ERROR
		"In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
	)
endif()

# Get source files except for main.cpp
file(GLOB SRC CONFIG_DEPENDS "src/[!main]*.cpp")
file(GLOB TESTS CONFIG_DEPENDS "tests/*.cpp")
set(SRC_HTTP "src/http.cpp")
set(SRC_RESTAPI "src/rest_api.cpp")

# Find all the packages required to build
find_package(Threads REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(cxxopts CONFIG REQUIRED)
#find_package(Poco CONFIG REQUIRED COMPONENTS Net JSON Util)
find_package(libzip CONFIG REQUIRED)
find_package(SQLiteCpp CONFIG REQUIRED)

set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_BUILD_RPATH "build/cmake")
set(CMAKE_CXX_FLAGS 
	"${CMAKE_CXX_FLAGS} -std=c++20 -g -Ofast -fPIC -fPIE -fpermissive -Wall -Wno-switch -Wno-unused-variable -Wno-unused-function -Wno-sign-conversion -Wno-deprecated-declarations -pedantic-errors"
)
set(INCLUDE_DIRS
	"include"
	"/usr/include/doctest"
	${RAPIDJSON_INCLUDE_DIRS}
  #  Poco::Net
  #  Poco::JSON
  #Poco::Util
)
set(LINK_LIBS 
	fmt::fmt 
	Threads::Threads 
	Catch2::Catch2 
	cxxopts::cxxopts
	SQLiteCpp
  #Poco::Net
  #Poco::JSON
  #Poco::Util
	-lcurlpp
	-lzip
	-lsqlite3
	-lcurl
)

# Set library and executable targets
add_library(${PROJECT_NAME}-shared SHARED "${SRC}")
add_library(${PROJECT_NAME}-static STATIC "${SRC}")
add_library(${PROJECT_NAME}-http SHARED "${SRC_HTTP}")
add_library(${PROJECT_NAME}-restapi SHARED "${SRC_RESTAPI}")
add_executable(${PROJECT_NAME} "src/main.cpp")
add_executable(${PROJECT_NAME}.static "src/main.cpp")
add_executable(${PROJECT_NAME}.tests "${TESTS}")

# Set include directories for targets
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME}.static PRIVATE ${INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME}.tests PRIVATE ${INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME}-shared PRIVATE ${INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME}-static PRIVATE ${INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME}-http PRIVATE ${INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME}-restapi PRIVATE ${INCLUDE_DIRS})

# Set link libraries for targets
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}-shared ${LINK_LIBS})
target_link_libraries(${PROJECT_NAME}.static PRIVATE ${PROJECT_NAME}-static ${LINK_LIBS})
target_link_libraries(${PROJECT_NAME}.tests PRIVATE ${PROJECT_NAME}-shared ${LINK_LIBS})
target_link_libraries(${PROJECT_NAME}-shared PRIVATE ${LINK_LIBS})
target_link_libraries(${PROJECT_NAME}-static PRIVATE ${LINK_LIBS})
target_link_libraries(${PROJECT_NAME}-http PRIVATE ${LINK_LIBS})
target_link_libraries(${PROJECT_NAME}-restapi PRIVATE ${LINK_LIBS})

# Add project unit tests
# add_custom_target("${PROJECT_NAME}-tests" SOURCE ${TESTS})
