cmake_minimum_required(VERSION 3.15)
project(ADBKing VERSION 1.0.1)

# ✅ Set macOS minimum deployment target for backward compatibility
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS deployment version")
    # Add macOS-specific compiler flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ImGui source files
file(GLOB IMGUI_SRC
    imgui/*.cpp
    imgui/backends/imgui_impl_sdl2.cpp
    imgui/backends/imgui_impl_opengl3.cpp
)

# App source files
file(GLOB APP_SRC
    src/*.cpp
    src/core/registry/*.cpp
    src/core/capability/*.cpp
    src/core/ui/*.cpp
    src/panels/*.cpp
)

# Add executable
add_executable(imgui_app
    ${APP_SRC}
    ${IMGUI_SRC}
)



# ✅ Find OpenGL on Linux
if(NOT WIN32 AND NOT APPLE)
    find_package(OpenGL REQUIRED)
endif()

# Set static runtime for the target on Windows
if(WIN32)
    set_target_properties(imgui_app PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endif()

# Include directories
target_include_directories(imgui_app PRIVATE
    imgui
    imgui/backends
    src
)

# ✅ Find SDL2 - use pkg-config on Unix systems, vcpkg/manual on Windows
if(WIN32)
    find_package(SDL2 CONFIG REQUIRED)
    find_package(CURL CONFIG REQUIRED)
else()
    # Unix/macOS: Use pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(CURL REQUIRED libcurl)
endif()

# ✅ Link SDL2 with static linking for macOS
if(SDL2_FOUND)
    target_include_directories(imgui_app PRIVATE ${SDL2_INCLUDE_DIRS})
    
    if(APPLE)
        # For macOS, use static linking to avoid code signing issues
        # Find static SDL2 library
        find_library(SDL2_STATIC_LIB
            NAMES libSDL2.a SDL2
            PATHS
            /usr/local/lib
            /opt/homebrew/lib
            /usr/lib
            ${SDL2_LIBRARY_DIRS}
        )
        
        if(SDL2_STATIC_LIB)
            message(STATUS "Found static SDL2 library: ${SDL2_STATIC_LIB}")
            target_link_libraries(imgui_app PRIVATE ${SDL2_STATIC_LIB})
        else()
            # Fallback to dynamic linking but with proper bundling
            target_link_libraries(imgui_app PRIVATE ${SDL2_LIBRARIES})
            
            # Set RPATH to @executable_path/../Frameworks for macOS
            set_target_properties(imgui_app PROPERTIES
                INSTALL_RPATH "@executable_path/../Frameworks"
                BUILD_WITH_INSTALL_RPATH TRUE
            )
            
            # Store SDL2 library path for bundling later
            get_filename_component(SDL2_LIBRARY_PATH ${SDL2_LIBRARIES} REALPATH)
            set_target_properties(imgui_app PROPERTIES
                SDL2_LIBRARY_PATH "${SDL2_LIBRARY_PATH}"
            )
        endif()
        
        # Add all necessary macOS frameworks
        target_link_libraries(imgui_app PRIVATE 
            "-framework OpenGL" 
            "-framework Cocoa" 
            "-framework IOKit" 
            "-framework CoreVideo" 
            "-framework Carbon"
            "-framework CoreGraphics"
            "-framework CoreFoundation"
            "-framework AudioToolbox"
            "-framework CoreAudio"
            "-framework ForceFeedback"
            "-framework AVFoundation"
            "-framework CoreHaptics"
            "-framework GameController"
            "-framework Metal"
            "-framework MetalKit"
            "-framework QuartzCore"
        )
        
        # Set macOS-specific properties
        set_target_properties(imgui_app PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
        )
        
        # Create Info.plist for macOS app bundle
        if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")
            file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">
<plist version=\"1.0\">
<dict>
    <key>CFBundleExecutable</key>
    <string>imgui_app</string>
    <key>CFBundleIdentifier</key>
    <string>com.adbking.app</string>
    <key>CFBundleName</key>
    <string>ADBKing</string>
    <key>CFBundleDisplayName</key>
    <string>ADBKing</string>
    <key>CFBundleVersion</key>
    <string>1.0.1</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0.1</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleSignature</key>
    <string>????</string>
    <key>LSMinimumSystemVersion</key>
    <string>12.0</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSPrincipalClass</key>
    <string>NSApplication</string>
</dict>
</plist>")
        endif()
    elseif(WIN32)
        target_link_libraries(imgui_app PRIVATE SDL2::SDL2main SDL2::SDL2-static)
    else()
        # For other platforms, use dynamic linking
        target_link_libraries(imgui_app PRIVATE ${SDL2_LIBRARIES})
    endif()
    
    if(NOT WIN32 AND NOT APPLE)
        target_compile_options(imgui_app PRIVATE ${SDL2_CFLAGS_OTHER})
        target_link_options(imgui_app PRIVATE ${SDL2_LDFLAGS_OTHER})
    endif()

    if(WIN32)
        target_link_libraries(imgui_app PRIVATE opengl32)
    endif()
    
    if(NOT WIN32 AND NOT APPLE)
        target_link_libraries(imgui_app PRIVATE OpenGL::GL)
    endif()
    
    # Link CURL
    if(WIN32)
        target_link_libraries(imgui_app PRIVATE CURL::libcurl)
        message(STATUS "CURL linked via vcpkg")
    elseif(CURL_FOUND)
        target_include_directories(imgui_app PRIVATE ${CURL_INCLUDE_DIRS})
        if(TARGET CURL::libcurl)
            target_link_libraries(imgui_app PRIVATE CURL::libcurl)
        else()
            target_link_libraries(imgui_app PRIVATE ${CURL_LIBRARIES})
        endif()
        message(STATUS "CURL found and linked successfully")
    else()
        message(WARNING "CURL not found - download functionality will be disabled")
        target_compile_definitions(imgui_app PRIVATE DISABLE_DOWNLOADS)
    endif()
else()
    message(FATAL_ERROR "SDL2 not found!")
endif()

# Include local overrides if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.local.txt")
    include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.local.txt")
endif()
