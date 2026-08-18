cmake_minimum_required(VERSION 3.16)
project(SBSWars VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(SBS_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

set(SBS_WARNINGS -Wall -Wextra -Wpedantic -Wno-unused-parameter)
if(SBS_WARNINGS_AS_ERRORS)
    list(APPEND SBS_WARNINGS -Werror)
endif()

find_package(SDL2 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

include_directories(
    ${CMAKE_SOURCE_DIR}/Source
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_MIXER_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
)

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
    Source/Game/UISystem.cpp
    Source/Game/ClientApp.cpp
)

add_library(sbs_core STATIC ${SBS_CORE_SOURCES})
target_compile_options(sbs_core PRIVATE ${SBS_WARNINGS})
target_compile_definitions(sbs_core PRIVATE SBS_VERSION="${PROJECT_VERSION}")

add_library(sbs_client STATIC ${SBS_CLIENT_SOURCES})
target_compile_options(sbs_client PRIVATE ${SBS_WARNINGS})
target_compile_definitions(sbs_client PRIVATE SBS_VERSION="${PROJECT_VERSION}")
target_link_libraries(sbs_client PUBLIC sbs_core SDL2::SDL2 ${SDL2_MIXER_LIBRARIES} ${SDL2_TTF_LIBRARIES})

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
target_compile_definitions(sbswars-tests PRIVATE SBS_VERSION="${PROJECT_VERSION}" SBS_HEADLESS=1)

set(SBS_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/Binaries)
set_target_properties(sbswars sbswars-server sbswars-launcher sbswars-installer sbswars-tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SBS_OUTPUT_DIR}
)

enable_testing()
add_test(NAME SBSWarsUnitTests COMMAND sbswars-tests)

install(TARGETS sbswars sbswars-server sbswars-launcher sbswars-installer
    RUNTIME DESTINATION bin)
install(DIRECTORY Maps DESTINATION share/sbswars)
install(DIRECTORY Content DESTINATION share/sbswars)
install(DIRECTORY Audio DESTINATION share/sbswars)
install(DIRECTORY UI DESTINATION share/sbswars)
