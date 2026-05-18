
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

struct WindowState {
    bool showAbout, showTheme, showDemo;
    bool showDeviceList, showEmulator, showWireless;
};

static std::string ConfigPath() {
    const char* home = getenv(
#ifdef _WIN32
        "USERPROFILE"
#else
        "HOME"
#endif
    );
    return home ? std::string(home) + "/.seaweed_state" : "";
}

static void SaveState(const WindowState& s) {
    std::string path = ConfigPath();
    if (path.empty()) return;
    FILE* f = fopen(path.c_str(), "w");
    if (f) {
        fprintf(f, "%d\n%d\n%d\n%d\n%d\n%d",
            (int)s.showAbout, (int)s.showTheme, (int)s.showDemo,
            (int)s.showDeviceList, (int)s.showEmulator, (int)s.showWireless);
        fclose(f);
    }
}

static bool LoadState(WindowState& s) {
    std::string path = ConfigPath();
    if (path.empty()) return false;
    FILE* f = fopen(path.c_str(), "r");
    if (!f) return false;
    int a, t, d, dl, e, wl;
    int r = fscanf(f, "%d\n%d\n%d\n%d\n%d\n%d", &a, &t, &d, &dl, &e, &wl);
    fclose(f);
    if (r >= 6) {
        s.showAbout = a; s.showTheme = t; s.showDemo = d;
        s.showDeviceList = dl; s.showEmulator = e; s.showWireless = wl;
        return true;
    }
    return false;
}

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

#ifdef DISABLE_DOWNLOADS
#else
#include <curl/curl.h>
#endif

static std::string ExecCmd(const char* cmd) {
    std::string result;
    std::array<char, 128> buf;
#ifdef _WIN32
    FILE* pipe = _popen(cmd, "r");
#else
    FILE* pipe = popen(cmd, "r");
#endif
    if (!pipe) return "";
    while (fgets(buf.data(), buf.size(), pipe)) result += buf.data();
#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) result.pop_back();
    return result;
}

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
        
        bool running = true;
        bool isDarkTheme = true;

        std::string osVersion = ExecCmd(
#ifdef __APPLE__
            "sw_vers -productVersion"
#elif defined(_WIN32)
            "ver"
#else
            "uname -sr"
#endif
        );
        if (osVersion.empty()) osVersion = "Unknown";

        WindowState st = {0, 0, 0, 0, 0, 0};
        LoadState(st);

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

                if (ImGui::BeginMenuBar())
                {
                    if (ImGui::BeginMenu("Seaweed"))
                    {
                        if (ImGui::MenuItem("About", nullptr, &st.showAbout)) {}
                        if (ImGui::MenuItem("Theme", nullptr, &st.showTheme)) {}
                        if (ImGui::MenuItem("Restart")) { RestartApp(argc, argv, window, gl_context); }
                        if (ImGui::MenuItem("Close App")) { running = false; }
                        if (ImGui::MenuItem("Close All")) {
                            st.showAbout = false;
                            st.showTheme = false;
                            st.showDemo = false;
                            st.showDeviceList = false;
                            st.showEmulator = false;
                            st.showWireless = false;
                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem("Demo Window", nullptr, &st.showDemo)) {}
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Devices"))
                    {
                        if (ImGui::MenuItem("Device List", nullptr, &st.showDeviceList)) {}
                        if (ImGui::MenuItem("Emulator", nullptr, &st.showEmulator)) {}
                        if (ImGui::MenuItem("Wireless", nullptr, &st.showWireless)) {}
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenuBar();
                }

                ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
                ImGui::DockSpace(dockspace_id, ImVec2(0, 0));
                ImGui::End();

                {
                    static bool registered = false;
                    if (!registered) {
                        RegisterSpotlightItem({"About", "Seaweed", [&]() { st.showAbout = !st.showAbout; }, [&]() { return st.showAbout; }});
                        RegisterSpotlightItem({"Theme", "Seaweed", [&]() { st.showTheme = !st.showTheme; }, [&]() { return st.showTheme; }, {"dark", "light", "colors", "appearance", "style"}});
                        RegisterSpotlightItem({"Restart", "Seaweed", [&]() { RestartApp(argc, argv, window, gl_context); }, nullptr});
                        RegisterSpotlightItem({"Close App", "Seaweed", [&]() { running = false; }, nullptr});
                        RegisterSpotlightItem({"Close All", "Seaweed", [&]() {
                            st.showAbout = false;
                            st.showTheme = false;
                            st.showDemo = false;
                            st.showDeviceList = false;
                            st.showEmulator = false;
                            st.showWireless = false;
                        }, [&]() {
                            return st.showAbout || st.showTheme || st.showDemo ||
                                   st.showDeviceList || st.showEmulator || st.showWireless;
                        }, {"clean", "hide", "windows"}, false});
                        RegisterSpotlightItem({"Demo Window", "Seaweed", [&]() { st.showDemo = !st.showDemo; }, [&]() { return st.showDemo; }, {"debug", "test", "example"}});
                        RegisterSpotlightItem({"Device List", "Devices", [&]() { st.showDeviceList = !st.showDeviceList; }, [&]() { return st.showDeviceList; }});
                        RegisterSpotlightItem({"Emulator", "Devices", [&]() { st.showEmulator = !st.showEmulator; }, [&]() { return st.showEmulator; }});
                        RegisterSpotlightItem({"Wireless", "Devices", [&]() { st.showWireless = !st.showWireless; }, [&]() { return st.showWireless; }});
                        registered = true;
                    }
                }

                RenderSpotlight();

                if (st.showAbout) {
                    if (ImGui::Begin("About Seaweed", &st.showAbout)) {
                        char buf[1024];

                        ImGui::Text("Name"); ImGui::SameLine(120);
                        snprintf(buf, sizeof(buf), "Seaweed");
                        ImGui::InputText("##name", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);

                        ImGui::Text("Version"); ImGui::SameLine(120);
                        snprintf(buf, sizeof(buf), "1.0.0");
                        ImGui::InputText("##version", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);

                        ImGui::Text("Developer"); ImGui::SameLine(120);
                        snprintf(buf, sizeof(buf), "Raju Shingadiya");
                        ImGui::InputText("##developer", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);

                        ImGui::Separator();

                        ImGui::Text("OS"); ImGui::SameLine(120);
                        snprintf(buf, sizeof(buf), "%s", osVersion.c_str());
                        ImGui::InputText("##os", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);

                        ImGui::Text("ADB Path"); ImGui::SameLine(120);
                        std::string curAdbPath = GlobalConfig::GetADBPath();
                        snprintf(buf, sizeof(buf), "%s", curAdbPath.empty() ? "Not found" : curAdbPath.c_str());
                        ImGui::InputText("##adbpath", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);

                        ImGui::Text("Device"); ImGui::SameLine(120);
                        std::string selectedDevice = GlobalConfig::GetSelectedDeviceId();
                        snprintf(buf, sizeof(buf), "%s", selectedDevice.empty() ? "None" : selectedDevice.c_str());
                        ImGui::InputText("##device", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);

                        ImGui::Text("ADB Version"); ImGui::SameLine(120);
                        std::string adbVerStr = ExecCmd(GlobalConfig::BuildAdbCommand("version").c_str());
                        if (adbVerStr.empty()) adbVerStr = "Not found";
                        ImGui::InputTextMultiline("##adbver", &adbVerStr[0], adbVerStr.size() + 1,
                            ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly);
                    }
                    ImGui::End();
                }

                if (st.showTheme) {
                    if (ImGui::Begin("Theme", &st.showTheme)) {
                        bool wasDark = isDarkTheme;
                        ImGui::Text("Current: %s", isDarkTheme ? "Dark" : "Light");
                        ImGui::Separator();
                        if (ImGui::RadioButton("Dark Theme", isDarkTheme)) isDarkTheme = true;
                        if (ImGui::RadioButton("Light Theme", !isDarkTheme)) isDarkTheme = false;
                        if (isDarkTheme != wasDark) {
                            if (isDarkTheme) ImGui::StyleColorsDark();
                            else ImGui::StyleColorsLight();
                        }
                    }
                    ImGui::End();
                }

                if (st.showDeviceList) {
                    if (ImGui::Begin("Device List", &st.showDeviceList)) {
                        auto devices = GlobalConfig::GetDevices();
                        std::string selectedId = GlobalConfig::GetSelectedDeviceId();

                        if (devices.empty()) {
                            ImGui::TextDisabled("No devices found. Connect a device or start an emulator.");
                        } else {
                            ImGui::Text("Devices: %zu", devices.size());
                            ImGui::Separator();

                            if (ImGui::BeginTable("devices", 4,
                                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable))
                            {
                                ImGui::TableSetupColumn("Device");
                                ImGui::TableSetupColumn("ID");
                                ImGui::TableSetupColumn("OS");
                                ImGui::TableSetupColumn("State");
                                ImGui::TableHeadersRow();

                                for (int i = 0; i < (int)devices.size(); i++) {
                                    const auto& d = devices[i];
                                    bool isSelected = (d.deviceId == selectedId);

                                    ImGui::TableNextRow();
                                    ImGui::TableNextColumn();

                                    ImGui::PushID(i);
                                    std::string label = (isSelected ? "[x] " : "[ ] ") + d.deviceName;
                                    if (ImGui::Selectable(label.c_str(), isSelected,
                                        ImGuiSelectableFlags_SpanAllColumns))
                                    {
                                        GlobalConfig::SetSelectedDeviceId(d.deviceId);
                                    }
                                    ImGui::TableNextColumn();
                                    ImGui::Text("%s", d.deviceId.c_str());
                                    ImGui::TableNextColumn();
                                    ImGui::Text("%s", d.osVersion.empty() ? "-" : d.osVersion.c_str());
                                    ImGui::TableNextColumn();
                                    ImGui::Text("%s", AdbStateToString(d.state));
                                    ImGui::PopID();
                                }
                                ImGui::EndTable();
                            }
                        }
                    }
                    ImGui::End();
                }
                if (st.showEmulator) {
                    if (ImGui::Begin("Emulator", &st.showEmulator)) {
                        auto& em = EmulatorManager::GetInstance();
                        bool scanning = em.IsScanning();

                        if (ImGui::Button(scanning ? "Scanning..." : "Refresh", ImVec2(120, 0))) {
                            if (!scanning) em.ScanEmulators();
                        }
                        ImGui::SameLine();
                        std::string emPath = em.GetEmulatorPath();
                        if (!emPath.empty()) {
                            ImGui::TextDisabled("Emulator: %s", emPath.c_str());
                        } else {
                            ImGui::TextDisabled("Emulator not found in default paths");
                        }

                        ImGui::Separator();

                        auto emulators = em.GetEmulators();
                        if (emulators.empty() && !scanning) {
                            ImGui::TextDisabled("No emulators found. Press Refresh to scan.");
                        } else if (scanning) {
                            ImGui::TextDisabled("Scanning for emulators...");
                        } else {
                            if (ImGui::BeginTable("emulators", 4,
                                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable))
                            {
                                ImGui::TableSetupColumn("Name");
                                ImGui::TableSetupColumn("Status");
                                ImGui::TableSetupColumn("Device ID");
                                ImGui::TableSetupColumn("Actions");
                                ImGui::TableHeadersRow();

                                for (int i = 0; i < (int)emulators.size(); i++) {
                                    const auto& e = emulators[i];

                                    ImGui::TableNextRow();
                                    ImGui::TableNextColumn();
                                    ImGui::Text("%s", e.name.c_str());
                                    ImGui::TableNextColumn();
                                    if (e.isRunning) {
                                        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Running");
                                    } else {
                                        ImGui::Text("Stopped");
                                    }
                                    ImGui::TableNextColumn();
                                    ImGui::Text("%s", e.isRunning ? e.deviceId.c_str() : "-");
                                    ImGui::TableNextColumn();

                                    ImGui::PushID(i);
                                    if (e.isRunning) {
                                        if (ImGui::Button("Stop", ImVec2(60, 0))) {
                                            em.StopEmulator(e.deviceId);
                                        }
                                    } else {
                                        if (ImGui::Button("Start", ImVec2(60, 0))) {
                                            em.StartEmulator(e.name);
                                        }
                                    }
                                    ImGui::PopID();
                                }
                                ImGui::EndTable();
                            }
                        }
                    }
                    ImGui::End();
                }
                if (st.showWireless) {
                    if (ImGui::Begin("Wireless", &st.showWireless)) {
                        ImGui::Text("Coming soon...");
                    }
                    ImGui::End();
                }

                if (st.showDemo)
                    ImGui::ShowDemoWindow(&st.showDemo);

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

        SaveState(st);

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
