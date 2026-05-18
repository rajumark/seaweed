
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_opengl.h>
#include "spotlight.h"
#include "device_manager.h"
#include "global_config.h"
#include "emulator_manager.h"
#include "setup.h"
#include "core/registry/panel_registry.h"
#include "core/registry/command_registry.h"
#include "panels/register_all_panels.h"
#include <iostream>
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <array>
#include <cstdlib>
#include <string>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

#ifdef DISABLE_DOWNLOADS
#else
#include <curl/curl.h>
#endif

static bool running = true;
static bool s_showDemo = false;

static void RestartApp(int argc, char* argv[], SDL_Window* window, SDL_GLContext gl_context)
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
#ifdef DISABLE_DOWNLOADS
#else
    curl_global_cleanup();
#endif
#ifdef _WIN32
    _execvp(argv[0], argv);
#else
    execvp(argv[0], argv);
#endif
}

static void RegisterCommands(int argc, char* argv[], SDL_Window* window, SDL_GLContext gl_context) {
    CommandRegistry::Get().Register({
        "restart", "Restart", "Seaweed", {},
        [argc, argv, window, gl_context]() { RestartApp(argc, argv, window, gl_context); },
        nullptr
    });
    CommandRegistry::Get().Register({
        "close_app", "Close App", "Seaweed", {},
        []() { running = false; },
        nullptr
    });
    CommandRegistry::Get().Register({
        "close_all", "Close All", "Seaweed", {"clean", "hide", "windows"},
        []() {
            PanelRegistry::Get().CloseAll();
            s_showDemo = false;
        },
        nullptr
    });
    CommandRegistry::Get().Register({
        "demo_window", "Demo Window", "Seaweed", {"debug", "test", "example"},
        []() { s_showDemo = !s_showDemo; },
        []() { return s_showDemo; }
    });
}

static void RegisterSpotlightItems() {
    for (auto& panel : PanelRegistry::Get().GetAll()) {
        bool* showPtr = panel.show;
        std::string title = panel.title;
        std::string category = panel.category;
        std::vector<std::string> keywords = panel.keywords;

        RegisterSpotlightItem({
            title, category,
            [showPtr]() { if (showPtr) *showPtr = !*showPtr; },
            [showPtr]() { return showPtr && *showPtr; },
            keywords
        });
    }

    for (const auto& cmd : CommandRegistry::Get().GetAll()) {
        auto exec = cmd.execute;
        auto toggle = cmd.isToggled;

        RegisterSpotlightItem({
            cmd.label, cmd.category,
            exec,
            toggle,
            cmd.keywords,
            toggle != nullptr
        });
    }
}

static void DrawMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Seaweed")) {
            for (auto& panel : PanelRegistry::Get().GetAll())
                if (panel.category == "Seaweed")
                    if (ImGui::MenuItem(panel.title.c_str(), nullptr, panel.show ? *panel.show : false))
                        if (panel.show) *panel.show = !*panel.show;

            ImGui::Separator();

            for (const auto& cmd : CommandRegistry::Get().GetAll())
                if (cmd.category == "Seaweed") {
                    if (cmd.isToggled) {
                        bool toggled = cmd.isToggled();
                        if (ImGui::MenuItem(cmd.label.c_str(), nullptr, &toggled))
                            cmd.execute();
                    } else {
                        if (ImGui::MenuItem(cmd.label.c_str()))
                            cmd.execute();
                    }
                }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Devices")) {
            for (auto& panel : PanelRegistry::Get().GetAll())
                if (panel.category == "Devices")
                    if (ImGui::MenuItem(panel.title.c_str(), nullptr, panel.show ? *panel.show : false))
                        if (panel.show) *panel.show = !*panel.show;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Data")) {
            for (auto& panel : PanelRegistry::Get().GetAll())
                if (panel.category == "Data")
                    if (ImGui::MenuItem(panel.title.c_str(), nullptr, panel.show ? *panel.show : false))
                        if (panel.show) *panel.show = !*panel.show;
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

int main(int argc, char* argv[]) {
    try {
        
#ifdef DISABLE_DOWNLOADS
#else
        if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
            std::cerr << "CURL initialization failed" << std::endl;
            return 1;
        }
#endif
        
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
            std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
#ifdef DISABLE_DOWNLOADS
#else
            curl_global_cleanup();
#endif
            return 1;
        }
        
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

        SDL_Window* window = SDL_CreateWindow("ADBKing", 
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            1280, 720, 
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
        
        if (!window) {
            std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return 1;
        }

        SDL_GLContext gl_context = SDL_GL_CreateContext(window);
        if (!gl_context) {
            std::cerr << "OpenGL context creation failed: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }
        
        SDL_GL_MakeCurrent(window, gl_context);
        SDL_GL_SetSwapInterval(1);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigWindowsMoveFromTitleBarOnly = true;

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

        RegisterAllPanels();
        RegisterCommands(argc, argv, window, gl_context);
        RegisterSpotlightItems();

        bool setupComplete = PlatformToolsExist();
        if (setupComplete) {
            GlobalConfig::InitializeADBPath();
            std::string adbPath = GlobalConfig::GetADBPath();
            GlobalConfig::InitializeDeviceManager();
            GlobalConfig::StartDeviceMonitoring();
            EmulatorManager::GetInstance().SetAdbPath(adbPath);
        }
        while (running) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (!SpotlightHandleSDLEvent(event))
                    ImGui_ImplSDL2_ProcessEvent(&event);
                if (event.type == SDL_QUIT)
                    running = false;
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();

            if (!setupComplete) {
                ShowSetup(setupComplete);
                if (setupComplete) {
                    std::string adbPath = GlobalConfig::GetADBPath();
                    GlobalConfig::InitializeDeviceManager();
                    GlobalConfig::StartDeviceMonitoring();
                    EmulatorManager::GetInstance().SetAdbPath(adbPath);
                }
            } else {
                ImGuiViewport* viewport = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(viewport->WorkPos);
                ImGui::SetNextWindowSize(viewport->WorkSize);
                ImGui::SetNextWindowViewport(viewport->ID);

                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

                ImGuiWindowFlags dockspace_flags =
                    ImGuiWindowFlags_NoDocking |
                    ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoBringToFrontOnFocus |
                    ImGuiWindowFlags_NoNavFocus;

                ImGui::Begin("MainDockSpace", nullptr, dockspace_flags | ImGuiWindowFlags_MenuBar);
                ImGui::PopStyleVar(2);

                DrawMenuBar();

                ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
                ImGui::DockSpace(dockspace_id, ImVec2(0, 0));
                ImGui::End();

                RenderSpotlight();
                PanelRegistry::Get().DrawAll();

                if (s_showDemo)
                    ImGui::ShowDemoWindow(&s_showDemo);
            }
            ImGui::Render();
            int display_w, display_h;
            SDL_GL_GetDrawableSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            SDL_GL_SwapWindow(window);
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        SDL_GL_DeleteContext(gl_context);
        GlobalConfig::StopDeviceMonitoring();
        SDL_DestroyWindow(window);
        SDL_Quit();
#ifdef DISABLE_DOWNLOADS
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
