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
)

# Add executable
add_executable(imgui_app
    ${APP_SRC}
    ${IMGUI_SRC}
)



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
    # Set static runtime for Windows to avoid MSVCP140.dll and VCRUNTIME140.dll dependencies
    # This links both C and C++ runtime libraries statically, eliminating the need for
    # Visual C++ Redistributable on target systems
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    
    # Force static runtime linking with explicit compiler flags
    set(CMAKE_CXX_FLAGS_RELEASE "/MT /EHsc")
    set(CMAKE_C_FLAGS_RELEASE "/MT")
    set(CMAKE_CXX_FLAGS_DEBUG "/MTd /EHsc")
    set(CMAKE_C_FLAGS_DEBUG "/MTd")
    
    # Ensure these flags are applied to the target
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT /EHsc")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MT")
    
    find_package(SDL2 REQUIRED)
    
    # Find CURL - try vcpkg first, then manual paths
    find_package(CURL)
    
    if(NOT CURL_FOUND)
        find_path(CURL_INCLUDE_DIR curl/curl.h
            PATHS
            "C:/tools/curl/current/include"
            "C:/curl/include"
            "C:/Program Files/curl/include"
            "C:/Program Files (x86)/curl/include"
        )
        
        find_library(CURL_LIBRARY
            NAMES libcurl libcurl_a curl
            PATHS
            "C:/tools/curl/current/lib"
            "C:/curl/lib"
            "C:/Program Files/curl/lib"
            "C:/Program Files (x86)/curl/lib"
        )
        
        if(CURL_INCLUDE_DIR AND CURL_LIBRARY)
            set(CURL_FOUND TRUE)
            set(CURL_INCLUDE_DIRS ${CURL_INCLUDE_DIR})
            set(CURL_LIBRARIES ${CURL_LIBRARY})
        endif()
    endif()
    
    if(NOT SDL2_FOUND)
        find_path(SDL2_INCLUDE_DIR SDL.h
            PATHS
            "C:/tools/sdl2/current/include"
            "C:/SDL2/include"
            "C:/Program Files/SDL2/include"
            "C:/Program Files (x86)/SDL2/include"
        )
        
        find_library(SDL2_LIBRARY
            NAMES SDL2
            PATHS
            "C:/tools/sdl2/current/lib/x64"
            "C:/SDL2/lib/x64"
            "C:/Program Files/SDL2/lib/x64"
            "C:/Program Files (x86)/SDL2/lib/x64"
        )
        
        if(SDL2_INCLUDE_DIR AND SDL2_LIBRARY)
            set(SDL2_FOUND TRUE)
            set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIR})
            set(SDL2_LIBRARIES ${SDL2_LIBRARY})
        endif()
    endif()
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
    else()
        # For Windows, prefer static linking to avoid runtime dependencies
        if(WIN32)
            # Try to find static SDL2 library for Windows
            find_library(SDL2_STATIC_LIB_WIN
                NAMES SDL2main SDL2
                PATHS
                ${SDL2_LIBRARY_DIRS}
                "C:/tools/sdl2/current/lib/x64"
                "C:/SDL2/lib/x64"
                "C:/Program Files/SDL2/lib/x64"
                "C:/Program Files (x86)/SDL2/lib/x64"
            )
            
            if(SDL2_STATIC_LIB_WIN)
                message(STATUS "Found static SDL2 library for Windows: ${SDL2_STATIC_LIB_WIN}")
                target_link_libraries(imgui_app PRIVATE ${SDL2_STATIC_LIB_WIN})
            else()
                # Fallback to dynamic linking but ensure SDL2.dll is bundled
                target_link_libraries(imgui_app PRIVATE ${SDL2_LIBRARIES})
                message(STATUS "Using dynamic SDL2 linking - SDL2.dll must be bundled")
            endif()
        else()
            # For other platforms, use dynamic linking
            target_link_libraries(imgui_app PRIVATE ${SDL2_LIBRARIES})
        endif()
    endif()
    
    if(NOT WIN32 AND NOT APPLE)
        target_compile_options(imgui_app PRIVATE ${SDL2_CFLAGS_OTHER})
        target_link_options(imgui_app PRIVATE ${SDL2_LDFLAGS_OTHER})
    endif()

    if(WIN32)
        target_link_libraries(imgui_app PRIVATE opengl32)
    endif()
    
    # Link CURL
    if(CURL_FOUND)
        target_include_directories(imgui_app PRIVATE ${CURL_INCLUDE_DIRS})
        target_link_libraries(imgui_app PRIVATE ${CURL_LIBRARIES})
        message(STATUS "CURL found and linked successfully")
    else()
        message(WARNING "CURL not found - download functionality will be disabled")
        # Define a macro to disable download functionality
        target_compile_definitions(imgui_app PRIVATE DISABLE_DOWNLOADS)
    endif()
else()
    message(FATAL_ERROR "SDL2 not found!")
endif()

# Include local overrides if they exist
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.local.txt")
    include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.local.txt")
endif()
