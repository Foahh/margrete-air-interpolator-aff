cmake_minimum_required(VERSION 3.30)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DUNICODE -D_UNICODE)
project(MargreteAirInterpolator VERSION 1.0.0)

include_directories("src")

function(setup_metadata)
    set(DLL_NAME "air_intp_aff" PARENT_SCOPE)

    set(JA_TITLE "Air 補間ツール (*.aff)" PARENT_SCOPE)
    set(JA_DESC "ArcCreate 譜面 (*.aff) のアークノートから AirCrush / AirSlide を補間します\\nhttps://github.com/Foahh/margrete-air-interpolator-aff" PARENT_SCOPE)

    set(EN_TITLE "Air Interpolator (*.aff)" PARENT_SCOPE)
    set(EN_DESC "Interpolate AirCrush / AirSlide from arc notes in ArcCreate Chart (*.aff)\\nhttps://github.com/Foahh/margrete-air-interpolator-aff" PARENT_SCOPE)

    set(ZH_CN_TITLE "Air 转换器 (*.aff)" PARENT_SCOPE)
    set(ZH_CN_DESC "将 ArcCreate 谱面 (*.aff) 中的蛇转换为 AirCrush / AirSlide\\nhttps://github.com/Foahh/margrete-air-interpolator-aff" PARENT_SCOPE)

    set(DEVELOPER "Foahh" PARENT_SCOPE)
endfunction()

function(generate_configurations)
    configure_file(config/config.ini.in ${CMAKE_CURRENT_BINARY_DIR}/${DLL_NAME}.ini @ONLY)
    configure_file(config/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/config.h @ONLY)
    configure_file(config/version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/include/version.rc @ONLY)

    include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
    message(STATUS "CONFIGURATION_PATH: ${CMAKE_CURRENT_BINARY_DIR}")
endfunction()

function(add_include_paths ENV_VAR_NAME)
    set(ENV_PATH $ENV{${ENV_VAR_NAME}})
    if (ENV_PATH)
        if (WIN32)
            string(REPLACE ";" "\\;" INCLUDE_PATHS "${ENV_PATH}")
        else ()
            string(REPLACE ":" "\\;" INCLUDE_PATHS "${ENV_PATH}")
        endif ()
        foreach (PATH ${INCLUDE_PATHS})
            if (EXISTS ${PATH})
                include_directories(${PATH})
                message(STATUS "${ENV_VAR_NAME}: ${PATH}")
            endif ()
        endforeach ()
    endif ()
endfunction()

function(setup_common_interface)
    find_package(imgui CONFIG REQUIRED)
    find_path(WTL_INCLUDE_DIRS "atlapp.h")

    add_library(common INTERFACE)
    target_include_directories(common INTERFACE ${WTL_INCLUDE_DIRS})
    target_link_libraries(common INTERFACE comctl32 d3d11 imgui::imgui)

    if (MSVC)
        target_compile_options(common INTERFACE /EHsc /utf-8)
    endif ()
endfunction()

function(build_main_library)
    add_library(main SHARED
            src/DLLMain.cpp
            src/ArcNote.cpp
            src/ArcParser.cpp
            src/Dialog.cpp
            src/Interpolator.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/include/version.rc
    )
    target_link_libraries(main PRIVATE common)
    set_target_properties(main PROPERTIES OUTPUT_NAME ${DLL_NAME})
endfunction()

function(build_tests)
    find_package(Catch2 3 REQUIRED)
    add_executable(tests
            tests/test.cpp
            src/ArcNote.cpp
            src/ArcParser.cpp
            src/Dialog.cpp
            src/Interpolator.cpp
    )

    find_package(Catch2 CONFIG REQUIRED)
    target_link_libraries(tests PRIVATE common Catch2::Catch2 Catch2::Catch2WithMain)
endfunction()

setup_metadata()
generate_configurations()
add_include_paths(CPLUS_INCLUDE_PATH)
setup_common_interface()
build_main_library()
build_tests()