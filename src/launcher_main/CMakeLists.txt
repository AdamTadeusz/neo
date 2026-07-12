add_library(launcher SHARED)

set_target_properties(launcher PROPERTIES PREFIX "")

if(NEO_USE_SEPARATE_BUILD_INFO)
	split_debug_information(TARGET launcher)
endif()

add_library_copy_target(TARGET launcher)

if(NEO_USE_SEPARATE_BUILD_INFO AND NOT COMPILER_MSVC)
	add_dependencies(launcher_copy_lib launcher_split_debug_information)
endif()

target_include_directories(launcher
	PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/common
		${CMAKE_SOURCE_DIR}/launcher_main
        ${CMAKE_SOURCE_DIR}/utils/common
        ${CMAKE_SOURCE_DIR}/public
)

#target_compile_definitions(launcher
#    LAUNCHER
#   APSTUDIO_INVOKED
#	LAUNCHER_DLL_EXPORT
#)

if(OS_WINDOWS)
    target_compile_definitions(launcher
        PRIVATE
        fopen=dont_use_fopen
    )

    target_link_libraries(launcher
        PRIVATE
        winmm.lib
    )
elseif(OS_LINUX)
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS})

elseif(OS_MACOS)

endif()

target_link_libraries(launcher
    PRIVATE
    mathlib::mathlib
    tier0::tier0
    tier1::tier1
    tier2::tier2
    tier3::tier3
    vstdlib::vstdlib
    steam_api::steam_api
)

target_link_directories(launcher
    PRIVATE
    ${LIBCOMMON}
    ${LIBPUBLIC}
)

target_sources_grouped(
    TARGET launcher
    NAME "Source Files"
    FILES
    main.cpp
)

target_sources_grouped(
    TARGET launcher
    NAME "Header Files"
    FILES
    resource.h
    ${CMAKE_SOURCE_DIR}/public/tier1/strtools.h
)