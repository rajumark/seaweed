#include "window_actions_helper.h"
#include <iostream>
#include <cstdlib>

// Platform-specific includes
#ifdef _WIN32
    #include <windows.h>
    #include <process.h>
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #include <ApplicationServices/ApplicationServices.h>
    #include <unistd.h>
    #include <signal.h>
#elif defined(__linux__)
    #include <unistd.h>
    #include <signal.h>
    #include <sys/types.h>
    #include <sys/wait.h>
#endif

void WindowActionsHelper::CloseMainWindow() {
    // Use the preferred method - signal quit
    SignalQuit();
}

void WindowActionsHelper::SignalQuit() {
    // Create and post an SDL_QUIT event
    SDL_Event quitEvent;
    quitEvent.type = SDL_QUIT;
    SDL_PushEvent(&quitEvent);
}

void WindowActionsHelper::CloseWindowPlatformSpecific() {
#ifdef _WIN32
    // Windows-specific window closing
    
    // Force close all windows associated with this process
    HWND hwnd = GetForegroundWindow();
    if (hwnd) {
        PostMessage(hwnd, WM_CLOSE, 0, 0);
    }
    
#elif defined(__APPLE__)
    // macOS-specific window closing
    
    // Send quit event to the application
    // This is handled automatically by SDL on macOS
    
#elif defined(__linux__)
    // Linux-specific window closing
    
    // Send SIGTERM to the current process
    // This allows for graceful shutdown
    kill(getpid(), SIGTERM);
    
#else
    // Generic/unknown platform
#endif
}

void WindowActionsHelper::CleanupAndExit() {
#ifdef _WIN32
    // Windows-specific cleanup
    
    // Clean up any remaining Windows resources
    // Force exit if needed
    exit(0);
    
#elif defined(__APPLE__)
    // macOS-specific cleanup
    
    // Clean up any macOS-specific resources
    exit(0);
    
#elif defined(__linux__)
    // Linux-specific cleanup
    
    // Clean up any Linux-specific resources
    exit(0);
    
#else
    // Generic cleanup
    exit(0);
#endif
}
