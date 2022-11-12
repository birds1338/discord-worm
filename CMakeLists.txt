include(FetchContent)
cmake_minimum_required (VERSION 3.8)
project ("Worm++")
set(CMAKE_CXX_STANDARD 17)
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_FIND_LIBRARY_SUFFIXES .a .lib)

FetchContent_Declare(json
	GIT_REPOSITORY https://github.com/nlohmann/json.git
	GIT_TAG v3.7.3)

FetchContent_GetProperties(json)
if(NOT json_POPULATED)
	FetchContent_Populate(json)
	add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git GIT_TAG 67e12da316754ff3c6c91b50aafb2658438f3c1e)
FetchContent_MakeAvailable(cpr)

add_executable (Worm WIN32 "main.cpp")
target_link_libraries(Worm PRIVATE nlohmann_json::nlohmann_json cpr::cpr)