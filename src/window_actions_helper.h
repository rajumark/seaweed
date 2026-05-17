#ifndef WINDOW_ACTIONS_HELPER_H
#define WINDOW_ACTIONS_HELPER_H

#include <SDL.h>

class WindowActionsHelper {
public:
    /**
     * Closes the main application window and exits the application
     * Handles cross-platform window closing for Windows, macOS, and Linux
     * Uses SDL's global functions to access the current window and context
     */
    static void CloseMainWindow();
    
    /**
     * Performs cleanup before application exit
     * Handles platform-specific cleanup tasks
     */
    static void CleanupAndExit();
    
    /**
     * Signals the application to quit by posting an SDL_QUIT event
     * This is the preferred method for graceful shutdown
     */
    static void SignalQuit();
    
private:
    /**
     * Platform-specific window closing implementation
     */
    static void CloseWindowPlatformSpecific();
};

#endif // WINDOW_ACTIONS_HELPER_H
