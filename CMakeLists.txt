cmake_minimum_required(VERSION 3.16)
project(SBSWars VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(SBS_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

if(MSVC)
    set(SBS_WARNINGS /W3)
    if(SBS_WARNINGS_AS_ERRORS)
        list(APPEND SBS_WARNINGS /WX)
    endif()
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    set(SBS_WARNINGS -Wall -Wextra -Wpedantic -Wno-unused-parameter)
    if(SBS_WARNINGS_AS_ERRORS)
        list(APPEND SBS_WARNINGS -Werror)
    endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(SDL2 CONFIG QUIET)
if(NOT SDL2_FOUND)
    find_package(SDL2 REQUIRED)
endif()

find_package(SDL2_mixer CONFIG QUIET)
find_package(SDL2_ttf CONFIG QUIET)
if(NOT SDL2_mixer_FOUND OR NOT SDL2_ttf_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL2_MIXER QUIET SDL2_mixer)
        pkg_check_modules(SDL2_TTF QUIET SDL2_ttf)
    endif()
endif()

if(NOT SDL2_mixer_FOUND AND NOT SDL2_MIXER_FOUND AND NOT TARGET SDL2_mixer::SDL2_mixer)
    find_library(SDL2_MIXER_LIBRARIES NAMES SDL2_mixer SDL2_mixer.dll)
    find_path(SDL2_MIXER_INCLUDE_DIRS SDL_mixer.h PATH_SUFFIXES SDL2)
endif()
if(NOT SDL2_ttf_FOUND AND NOT SDL2_TTF_FOUND AND NOT TARGET SDL2_ttf::SDL2_ttf)
    find_library(SDL2_TTF_LIBRARIES NAMES SDL2_ttf SDL2_ttf.dll)
    find_path(SDL2_TTF_INCLUDE_DIRS SDL_ttf.h PATH_SUFFIXES SDL2)
endif()

include_directories(${CMAKE_SOURCE_DIR}/Source)

set(SBS_CORE_SOURCES
    Source/Engine/SBSLog.cpp
    Source/Engine/NetSocket.cpp
    Source/Engine/Protocol.cpp
    Source/Game/MapData.cpp
    Source/Game/WeaponSystem.cpp
    Source/Game/BotAI.cpp
    Source/Game/GameModes.cpp
    Source/Game/World.cpp
    Source/Game/SaveSystem.cpp
    Source/Game/ServerHost.cpp
    Source/Game/LaunchOptions.cpp
)

set(SBS_CLIENT_SOURCES
    Source/Engine/Renderer.cpp
    Source/Engine/AudioEngine.cpp
    Source/Engine/TextureGen.cpp
    Source/Engine/Bitmap.cpp
    Source/Engine/AssetPath.cpp
    Source/Game/UISystem.cpp
    Source/Game/ClientApp.cpp
)

function(sbs_link_sdl target)
    if(TARGET SDL2::SDL2)
        target_link_libraries(${target} PUBLIC SDL2::SDL2)
    elseif(TARGET SDL2::SDL2-static)
        target_link_libraries(${target} PUBLIC SDL2::SDL2-static)
    else()
        target_link_libraries(${target} PUBLIC ${SDL2_LIBRARIES})
        target_include_directories(${target} PUBLIC ${SDL2_INCLUDE_DIRS})
    endif()
    if(TARGET SDL2_mixer::SDL2_mixer)
        target_link_libraries(${target} PUBLIC SDL2_mixer::SDL2_mixer)
    elseif(SDL2_MIXER_LIBRARIES)
        target_link_libraries(${target} PUBLIC ${SDL2_MIXER_LIBRARIES})
        target_include_directories(${target} PUBLIC ${SDL2_MIXER_INCLUDE_DIRS})
    endif()
    if(TARGET SDL2_ttf::SDL2_ttf)
        target_link_libraries(${target} PUBLIC SDL2_ttf::SDL2_ttf)
    elseif(SDL2_TTF_LIBRARIES)
        target_link_libraries(${target} PUBLIC ${SDL2_TTF_LIBRARIES})
        target_include_directories(${target} PUBLIC ${SDL2_TTF_INCLUDE_DIRS})
    endif()
    target_compile_definitions(${target} PUBLIC SDL_MAIN_HANDLED)
endfunction()

add_library(sbs_core STATIC ${SBS_CORE_SOURCES})
target_compile_options(sbs_core PRIVATE ${SBS_WARNINGS})
target_compile_definitions(sbs_core PRIVATE SBS_VERSION="${PROJECT_VERSION}")
if(WIN32)
    target_link_libraries(sbs_core PUBLIC ws2_32)
endif()

add_library(sbs_client STATIC ${SBS_CLIENT_SOURCES})
target_compile_options(sbs_client PRIVATE ${SBS_WARNINGS})
target_compile_definitions(sbs_client PRIVATE SBS_VERSION="${PROJECT_VERSION}")
target_link_libraries(sbs_client PUBLIC sbs_core)
sbs_link_sdl(sbs_client)

add_executable(sbswars Source/Game/main_client.cpp)
target_compile_options(sbswars PRIVATE ${SBS_WARNINGS})
target_link_libraries(sbswars PRIVATE sbs_client)
target_compile_definitions(sbswars PRIVATE SBS_VERSION="${PROJECT_VERSION}")

add_executable(sbswars-server Source/Game/main_server.cpp)
target_compile_options(sbswars-server PRIVATE ${SBS_WARNINGS})
target_link_libraries(sbswars-server PRIVATE sbs_core)
target_compile_definitions(sbswars-server PRIVATE SBS_VERSION="${PROJECT_VERSION}" SBS_HEADLESS=1)

add_executable(sbswars-launcher Source/Launcher/Launcher.cpp)
target_compile_options(sbswars-launcher PRIVATE ${SBS_WARNINGS})
target_link_libraries(sbswars-launcher PRIVATE sbs_client)
target_compile_definitions(sbswars-launcher PRIVATE SBS_VERSION="${PROJECT_VERSION}")

add_executable(sbswars-installer Source/Installer/Installer.cpp)
target_compile_options(sbswars-installer PRIVATE ${SBS_WARNINGS})
target_link_libraries(sbswars-installer PRIVATE sbs_client)
target_compile_definitions(sbswars-installer PRIVATE SBS_VERSION="${PROJECT_VERSION}")

add_executable(sbswars-tests Tests/test_all.cpp)
target_compile_options(sbswars-tests PRIVATE ${SBS_WARNINGS})
target_link_libraries(sbswars-tests PRIVATE sbs_core)
target_compile_definitions(sbswars-tests PRIVATE SBS_VERSION="${PROJECT_VERSION}" SBS_HEADLESS=1 SBS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

set(SBS_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/Binaries)
if(WIN32)
    set(SBS_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/Binaries/windows)
endif()
set_target_properties(sbswars sbswars-server sbswars-launcher sbswars-installer sbswars-tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SBS_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${SBS_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${SBS_OUTPUT_DIR}
)

if(WIN32)
    set_target_properties(sbswars-launcher PROPERTIES OUTPUT_NAME "SBSWarsLauncher")
    set_target_properties(sbswars-installer PROPERTIES OUTPUT_NAME "SBSWarsInstaller")
    set_target_properties(sbswars PROPERTIES OUTPUT_NAME "SBSWars")
    set_target_properties(sbswars-server PROPERTIES OUTPUT_NAME "SBSWarsServer")
endif()

if(MINGW)
    foreach(tgt IN ITEMS sbswars sbswars-server sbswars-launcher sbswars-installer sbswars-tests)
        target_link_options(${tgt} PRIVATE -static-libgcc -static-libstdc++)
    endforeach()
endif()

enable_testing()
add_test(NAME SBSWarsUnitTests COMMAND sbswars-tests)

install(TARGETS sbswars sbswars-server sbswars-launcher sbswars-installer
    RUNTIME DESTINATION bin)
install(DIRECTORY Maps DESTINATION share/sbswars)
install(DIRECTORY Content DESTINATION share/sbswars)
install(DIRECTORY Audio DESTINATION share/sbswars)
install(DIRECTORY UI DESTINATION share/sbswars)
