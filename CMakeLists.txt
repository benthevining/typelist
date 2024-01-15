cmake_minimum_required (VERSION 3.26.0 FATAL_ERROR)

project (
	typelist
	VERSION 0.0.1
	LANGUAGES CXX 
	DESCRIPTION "Compile-time typelist for C++"
	HOMEPAGE_URL "https://github.com/benthevining/typelist"
)

add_library (typelist INTERFACE)
add_library (typelist::typelist ALIAS typelist)

target_compile_features (typelist INTERFACE cxx_std_20)

set_target_properties (typelist PROPERTIES CXX_STANDARD_REQUIRED ON)

target_include_directories (typelist INTERFACE
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
	"$<INSTALL_INTERFACE:include>"
)

target_sources (typelist INTERFACE 
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/typelist/typelist.h>"
	"$<INSTALL_INTERFACE:include/typelist/typelist.h>"
)

install (FILES "${CMAKE_CURRENT_LIST_DIR}/include/typelist/typelist.h"
		 DESTINATION include
		 COMPONENT typelist
)

#

find_package (Doxygen)

if(DOXYGEN_FOUND)
	doxygen_add_docs (
		typelist_docs
		"${CMAKE_CURRENT_LIST_DIR}/include/typelist/typelist.h"
		USE_STAMP_FILE
		COMMENT "Building typelist docs..."
	)
endif()

#

enable_testing()

add_library (typelist_test STATIC EXCLUDE_FROM_ALL)

target_sources (typelist_test PRIVATE test/test.cpp)

target_link_libraries (typelist_test PRIVATE typelist::typelist)

add_test (NAME typelist.test
		  COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}"
									 --target typelist_test 
									 --config "$<CONFIG>"
)
