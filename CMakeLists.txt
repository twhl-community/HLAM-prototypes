cmake_minimum_required(VERSION 3.27)

project(MultiAsset
	VERSION 0.1.0
	DESCRIPTION "MultiAsset"
	LANGUAGES CXX)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Find the QtWidgets library
find_package(Qt6 COMPONENTS Widgets OpenGLWidgets REQUIRED)
find_package(glm REQUIRED)

qt_standard_project_setup()

if (MSVC)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/version_generated.rc @ONLY)
endif()

qt_add_executable(MultiAsset)

set_target_properties(MultiAsset PROPERTIES
    WIN32_EXECUTABLE ON)

target_compile_features(MultiAsset
	PRIVATE
		cxx_std_20)

target_include_directories(MultiAsset
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
		${CMAKE_BINARY_DIR})

target_compile_definitions(MultiAsset
	PRIVATE
		QT_MESSAGELOGCONTEXT
		$<$<CXX_COMPILER_ID:MSVC>:UNICODE _UNICODE>
		$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:FILE_OFFSET_BITS=64>)

target_link_libraries(MultiAsset
	PRIVATE
		Qt6::Widgets
		Qt6::OpenGLWidgets
		${CMAKE_DL_LIBS}
		glm::glm)

target_compile_options(MultiAsset
	PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>:/MP /fp:strict>
		$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fPIC>)

target_link_options(MultiAsset
	PRIVATE
		$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wl,--exclude-libs,ALL>)

add_subdirectory(src)

# Create filters
get_target_property(SOURCE_FILES MultiAsset SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${SOURCE_FILES})

# Must be done after generating source groups
qt_add_resources(MultiAsset "resources"
	BASE "src"
	PREFIX "/"
	FILES
		src/multiasset.ico)

if (WIN32)
	# Use windeployqt to set up Qt dependencies
	add_custom_command(TARGET MultiAsset POST_BUILD
		COMMAND ${WINDEPLOYQT_EXECUTABLE}
			#--verbose 1
			--no-svg
			--no-opengl-sw
			--no-compiler-runtime
			--no-system-d3d-compiler
			\"$<TARGET_FILE:MultiAsset>\")
endif()

set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT MultiAsset)

install(TARGETS MultiAsset
	RUNTIME DESTINATION .)
