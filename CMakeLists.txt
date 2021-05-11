################################################################################
# Protoshop CMake build script
#
# WORK IN PROGRESS
# Still To Do:
# - This build only works out-of-tree.
# - HLSL compiles aren't represented in here yet (most things won't work).
################################################################################

cmake_minimum_required(VERSION 3.20.2)
project(Protoshop)

################################################################################
# Fetch all external dependencies from Git repositories
################################################################################

include(FetchContent)

################################################################################
# Fetch FreeType from GitHub
################################################################################

FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype
    GIT_TAG master
)

FetchContent_MakeAvailable(freetype)

################################################################################
# Fetch GoogleTest from GitHub
################################################################################

# TODO: Write tests; shame on you.

#FetchContent_Declare(
#    googletest
#    GIT_REPOSITORY https://github.com/google/googletest.git
#    GIT_TAG master
#)

#FetchContent_MakeAvailable(googletest)

################################################################################
# Fetch OpenVR
################################################################################

FetchContent_Declare(
    openvr
    GIT_REPOSITORY https://github.com/ValveSoftware/openvr
    GIT_TAG master
)

FetchContent_GetProperties(openvr)
if(NOT openvr_POPULATED)
    FetchContent_Populate(openvr)
endif()

################################################################################
# Build Defaults
################################################################################

# Use Unicode
add_definitions(-DUNICODE -D_UNICODE)

# Use C++17 standard
set(CMAKE_CXX_STANDARD 17)

################################################################################
# Protoshop Project
################################################################################

file(GLOB ALLFILES Source/*.cpp)		# Collect ALL CPP files in Source [Bad Practice]
add_executable(Protoshop WIN32 ${ALLFILES})	#Define the Protoshop executable target

# We have to include Vulkan with a bare compiler option.
# include_directories(...) interprets $(VULKAN_SDK) as a relative path and prefixes it :(
target_compile_options(Protoshop PUBLIC  /I\"$(VULKAN_SDK)/Include\")

include_directories(Protoshop ${openvr_SOURCE_DIR}/headers)

target_link_libraries(Protoshop freetype)
target_link_libraries(Protoshop ${openvr_SOURCE_DIR}/lib/win64/openvr_api.lib)
target_link_libraries(Protoshop $(VULKAN_SDK)/Lib/vulkan-1.lib)

add_custom_command(TARGET Protoshop COMMAND  copy /Y \"${openvr_SOURCE_DIR}/bin/win64\\openvr_api.dll\" \"$(ProjectDir)\")