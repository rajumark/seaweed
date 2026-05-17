
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_opengl.h>
#include <iostream>
#include <stdexcept>

#ifdef DISABLE_DOWNLOADS
#else
#include <curl/curl.h>
#endif

int main() {
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
            800, 600, 
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
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;

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
        bool show_demo_window = true;
        
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

            if (show_demo_window)
                ImGui::ShowDemoWindow(&show_demo_window);

            {
                ImGui::Begin("Hello, world!");
                ImGui::Text("This is the default ImGui window.");
                ImGui::Checkbox("Demo Window", &show_demo_window);
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
                ImGui::End();
            }

            ImGui::Render();
            int w, h;
            SDL_GL_GetDrawableSize(window, &w, &h);
            glViewport(0, 0, w, h);
            glClearColor(1, 1, 1, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(window);
        }

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
