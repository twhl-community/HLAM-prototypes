if (MSVC)
	target_sources(MultiAsset PRIVATE multiasset.rc version.rc)
endif()

target_sources(MultiAsset
	PRIVATE
		Main.cpp)

add_subdirectory(application)
add_subdirectory(assets)
add_subdirectory(assetsystems)
add_subdirectory(formats)
add_subdirectory(ui)
add_subdirectory(utils)
