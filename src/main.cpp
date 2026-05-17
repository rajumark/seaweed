
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_opengl.h>
#include <iostream>
#include <stdexcept>

#include "home.h"
#include "setup.h"
#include "force_update.h"
#include "global_config.h"
#include "device_manager.h"
#include "key_value_store.h"
#include "apps_action_cook_helper.h"
#include "date_expiration.h"
#include "font_manager.h"
#include <chrono>
#include <thread>
#ifdef DISABLE_DOWNLOADS
// CURL not available - download functionality disabled
#else
#include <curl/curl.h>
#endif

int main() {
    try {
        
#ifdef DISABLE_DOWNLOADS
        // CURL not available - skip initialization
#else
        // Initialize CURL
        if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
            std::cerr << "CURL initialization failed" << std::endl;
            return 1;
        }
#endif
        
        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
            std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
#ifdef DISABLE_DOWNLOADS
            // No CURL cleanup needed
#else
            curl_global_cleanup();
#endif
            return 1;
        }
        
        // Set OpenGL attributes
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

        // Create window
        SDL_Window* window = SDL_CreateWindow("ADBKing", 
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
            800, 600, 
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
        
        if (!window) {
            std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return 1;
        }

        // Create OpenGL context
        SDL_GLContext gl_context = SDL_GL_CreateContext(window);
        if (!gl_context) {
            std::cerr << "OpenGL context creation failed: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }
        
        SDL_GL_MakeCurrent(window, gl_context);
        SDL_GL_SetSwapInterval(1);

        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        
        // Configure ImGui flags
        io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard; // Disable keyboard navigation for now
        io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;  // Disable gamepad navigation for now

        // Initialize global configuration and device manager
        GlobalConfig::InitializeADBPath();
        GlobalConfig::InitializeDeviceManager();
        GlobalConfig::StartDeviceMonitoring();
        
        // Initialize background task manager for async ADB operations
        AppsActionCookHelper::InitializeBackgroundTaskManager();
        
        // Load preferences
        {
            // Theme (default: dark)
            const std::string themeStr = KeyValueStore::GetString("theme", "dark");
            if (themeStr == "light") {
                GlobalConfig::SetTheme(Theme::LIGHT);
            } else {
                GlobalConfig::SetTheme(Theme::DARK);
            }

            // UI global scale: keep at 1.0 for sharpest text
            ImGuiIO& ioInit = ImGui::GetIO();
            ioInit.FontGlobalScale = 1.0f;

            // Load UI style preferences (rounding and borders)
            ImGuiStyle& style = ImGui::GetStyle();
            const float savedFrameRounding = KeyValueStore::GetFloat("style_frame_rounding", style.FrameRounding);
            style.FrameRounding = savedFrameRounding;
            style.GrabRounding = savedFrameRounding;
            style.WindowBorderSize = KeyValueStore::GetFloat("style_window_border", style.WindowBorderSize);
            style.FrameBorderSize = KeyValueStore::GetFloat("style_frame_border", style.FrameBorderSize);
            style.PopupBorderSize = KeyValueStore::GetFloat("style_popup_border", style.PopupBorderSize);

            // Load preferred font size (default 18) and persist if not present
            int savedFontPx = KeyValueStore::GetInt("font_px", 18);
            GlobalConfig::SetFontSize(savedFontPx);
        }

        // Apply the current theme (already set above) and initialize fonts
        GlobalConfig::ApplyTheme(GlobalConfig::GetCurrentTheme());
        GlobalConfig::LoadFont(GlobalConfig::GetFontSize());
        
        // Check software expiration before proceeding
        bool softwareExpired = IsSoftwareExpired();
        
        if (!ImGui_ImplSDL2_InitForOpenGL(window, gl_context)) {
            std::cerr << "ImGui SDL2 implementation initialization failed" << std::endl;
            ImGui::DestroyContext();
            SDL_GL_DeleteContext(gl_context);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }
        
        if (!ImGui_ImplOpenGL3_Init("#version 150")) {
            std::cerr << "ImGui OpenGL3 implementation initialization failed" << std::endl;
            ImGui_ImplSDL2_Shutdown();
            ImGui::DestroyContext();
            SDL_GL_DeleteContext(gl_context);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }
        
        bool running = true;
        bool setupComplete = false;
        
        // Check if software is expired and show force update screen
        if (softwareExpired) {
            while (running) {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    ImGui_ImplSDL2_ProcessEvent(&event);
                    if (event.type == SDL_QUIT)
                        running = false;
                }

                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplSDL2_NewFrame();
                ImGui::NewFrame();

                ShowForceUpdate();

                ImGui::Render();
                int w, h;
                SDL_GL_GetDrawableSize(window, &w, &h);
                glViewport(0, 0, w, h);
                glClearColor(1, 1, 1, 1);
                glClear(GL_COLOR_BUFFER_BIT);
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                SDL_GL_SwapWindow(window);
            }
            // If we reach here, the user closed the force update window
            return 0;
        }
        
        // Show setup page until user manually completes it
        // auto startTime = std::chrono::steady_clock::now(); // Disabled for now
        
        while (running && !setupComplete) {
            // auto currentTime = std::chrono::steady_clock::now();
            // auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime);
            
            // Check if 3 seconds have passed - DISABLED
            // if (elapsed.count() >= 3000) {
            //     setupComplete = true;
            //     break;
            // }
            
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL2_ProcessEvent(&event);
                if (event.type == SDL_QUIT)
                    running = false;
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            ShowSetup(setupComplete);

            ImGui::Render();
            int w, h;
            SDL_GL_GetDrawableSize(window, &w, &h);
            glViewport(0, 0, w, h);
            glClearColor(1, 1, 1, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(window);
        }
        
        // Main application loop (Home page)
        while (running) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL2_ProcessEvent(&event);
                if (event.type == SDL_QUIT)
                    running = false;
            }

            // Check if font needs to be reloaded
            if (GlobalConfig::FontNeedsReload()) {
                GlobalConfig::LoadFont(GlobalConfig::GetFontSize());
            }
            
            // Also check if FontManager needs to reload fonts
            if (FontManager::GetInstance().FontsNeedReload()) {
                FontManager::GetInstance().ReloadFonts();
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            ShowHome();

            ImGui::Render();
            int w, h;
            SDL_GL_GetDrawableSize(window, &w, &h);
            glViewport(0, 0, w, h);
            glClearColor(1, 1, 1, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(window);
        }

        // Stop device monitoring before shutting down
        GlobalConfig::StopDeviceMonitoring();
        
        // Shutdown background task manager
        AppsActionCookHelper::ShutdownBackgroundTaskManager();
        
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        SDL_GL_DeleteContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
#ifdef DISABLE_DOWNLOADS
        // No CURL cleanup needed
#else
        curl_global_cleanup();
#endif

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception caught" << std::endl;
        return 1;
    }
}
