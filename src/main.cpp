
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_opengl.h>
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

static std::string ConfigPath() {
    const char* home = getenv(
#ifdef _WIN32
        "USERPROFILE"
#else
        "HOME"
#endif
    );
    return home ? std::string(home) + "/.seaweed_size" : "";
}

static void SaveWindowSize(int w, int h, int x, int y) {
    std::string path = ConfigPath();
    if (path.empty()) return;
    FILE* f = fopen(path.c_str(), "w");
    if (f) { fprintf(f, "%d\n%d\n%d\n%d", w, h, x, y); fclose(f); }
}

static bool LoadWindowSize(int& w, int& h, int& x, int& y) {
    std::string path = ConfigPath();
    if (path.empty()) return false;
    FILE* f = fopen(path.c_str(), "r");
    if (!f) return false;
    int r = fscanf(f, "%d\n%d\n%d\n%d", &w, &h, &x, &y);
    fclose(f);
    return r == 4 && w > 0 && h > 0;
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

static void RestartApp(int argc, char* argv[], SDL_Window* window, SDL_GLContext gl_context) {
    int w, h, x, y;
    SDL_GetWindowSize(window, &w, &h);
    SDL_GetWindowPosition(window, &x, &y);
    SaveWindowSize(w, h, x, y);
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

        int winW = 1280, winH = 720, winX = SDL_WINDOWPOS_CENTERED, winY = SDL_WINDOWPOS_CENTERED;
        LoadWindowSize(winW, winH, winX, winY);

        SDL_Window* window = SDL_CreateWindow("ADBKing", 
            winX, winY,
            winW, winH, 
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
        
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
        ImGui::GetIO();

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
        bool show_demo_window = false;
        bool show_about = false;
        bool show_theme = false;
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

        std::string adbPath;
        const char* adbCandidates[] = {
            "/opt/homebrew/bin/adb",
            "/usr/local/bin/adb",
            "/usr/bin/adb",
            "/opt/homebrew/bin/adb",
        };
        for (auto p : adbCandidates) {
            FILE* f = fopen(p, "r");
            if (f) { fclose(f); adbPath = p; break; }
        }
        if (adbPath.empty()) {
#ifdef _WIN32
            const char* pf = getenv("LOCALAPPDATA");
            if (pf) adbPath = std::string(pf) + "\\Android\\Sdk\\platform-tools\\adb.exe";
#else
            const char* home = getenv("HOME");
            if (home) {
                std::string p = std::string(home) + "/Library/Android/sdk/platform-tools/adb";
                FILE* f = fopen(p.c_str(), "r");
                if (f) { fclose(f); adbPath = p; }
                else {
                    p = std::string(home) + "/Android/Sdk/platform-tools/adb";
                    f = fopen(p.c_str(), "r");
                    if (f) { fclose(f); adbPath = p; }
                }
            }
#endif
        }

        std::string adbVersion;
        if (!adbPath.empty()) {
            adbVersion = ExecCmd((adbPath + " version").c_str());
        }
        if (adbVersion.empty()) adbVersion = "Not found";
        
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

            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("Seaweed"))
                {
                    if (ImGui::MenuItem("About")) { show_about = true; }
                    if (ImGui::MenuItem("Theme")) { show_theme = true; }
                    if (ImGui::MenuItem("Restart")) { RestartApp(argc, argv, window, gl_context); }
                    if (ImGui::MenuItem("Close")) { running = false; }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Demo Window", nullptr, &show_demo_window)) {}
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

            if (show_about) {
                if (ImGui::Begin("About Seaweed", &show_about)) {
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
                    snprintf(buf, sizeof(buf), "%s", adbPath.empty() ? "Not found" : adbPath.c_str());
                    ImGui::InputText("##adbpath", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);

                    ImGui::Text("ADB Version"); ImGui::SameLine(120);
                    ImGui::InputTextMultiline("##adbver", &adbVersion[0], adbVersion.size() + 1,
                        ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly);
                }
                ImGui::End();
            }

            if (show_theme) {
                if (ImGui::Begin("Theme", &show_theme)) {
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

            if (show_demo_window)
                ImGui::ShowDemoWindow(&show_demo_window);

            ImGui::Render();
            int w, h;
            SDL_GL_GetDrawableSize(window, &w, &h);
            glViewport(0, 0, w, h);
            glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(window);
        }

        int curW, curH, curX, curY;
        SDL_GetWindowSize(window, &curW, &curH);
        SDL_GetWindowPosition(window, &curX, &curY);
        SaveWindowSize(curW, curH, curX, curY);

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

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception caught" << std::endl;
        return 1;
    }
}
