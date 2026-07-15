cmake_minimum_required(VERSION 3.15)
project(UVTransform VERSION 1.0 LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

#
# ---- Platform specific settings ----
#

if (WIN32)
    message(STATUS "Configuring for Windows")

    set(LXSDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../LXSDK-000025")

elseif (APPLE)
    message(STATUS "Configuring for macOS")
    message("Current directory: ${CMAKE_CURRENT_SOURCE_DIR}")

    # Overridable default paths
    set(LXSDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../LXSDK-000025")
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")

else()
    message(FATAL_ERROR "Unknown or unsupported platform")
endif()

#
# ---- LXSDK ----
#
if (NOT LXSDK_PATH)
    FetchContent_Declare( 
        lxsdk
        URL https://thefoundry.s3.us-east-1.amazonaws.com/products/modo/16.1v5/lxsdk16.1v5_692025.zip
    )
    FetchContent_Populate(lxsdk)
    message(STATUS "FetchContent lxsdk directory: ${lxsdk_SOURCE_DIR}")
    set(LXSDK_PATH ${lxsdk_SOURCE_DIR})
endif()

if (NOT EXISTS "${LXSDK_PATH}/include")
    message(FATAL_ERROR "LXSDK not found at: ${LXSDK_PATH}")
endif()
file(GLOB LXSDK_SOURCES "${LXSDK_PATH}/common/*.cpp")
message("LXSDK PATH: ${LXSDK_PATH}")
add_library(lxsdk STATIC ${LXSDK_SOURCES})
target_include_directories(lxsdk PRIVATE "${LXSDK_PATH}/include")


#
# ---- Plugin build ----
#

add_library(UVTransform SHARED
    source/tool.cpp
    source/space.cpp
)

target_include_directories(UVTransform PRIVATE
    source
    "${LXSDK_PATH}/include"
)

target_compile_definitions(UVTransform PRIVATE
    GL_SILENCE_DEPRECATION
)

target_link_libraries(UVTransform PRIVATE
    lxsdk
)

#
# ---- Install ----
#

install(FILES "index.cfg" DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES "README.html" DESTINATION ${CMAKE_INSTALL_PREFIX})
install(DIRECTORY "images" DESTINATION ${CMAKE_INSTALL_PREFIX})
install(TARGETS UVTransform DESTINATION ${CMAKE_INSTALL_PREFIX}/extra)

