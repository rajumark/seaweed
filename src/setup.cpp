#include "setup.h"
#include "global_config.h"
#include "imgui.h"
#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>
#ifdef DISABLE_DOWNLOADS
// CURL not available - download functionality disabled
#else
#include <curl/curl.h>
#endif
#include <thread>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#elif defined(__APPLE__)
#include <sys/stat.h>
#include <unistd.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#endif

// Platform tool download URLs
const char* PLATFORM_TOOLS_WINDOWS_URL = "https://raw.githubusercontent.com/rajumark/adbcontent/main/platform-tools-windows.zip";
const char* PLATFORM_TOOLS_MACOS_URL = "https://raw.githubusercontent.com/rajumark/adbcontent/main/platform-tools-macos.zip";
const char* PLATFORM_TOOLS_LINUX_URL = "https://raw.githubusercontent.com/rajumark/adbcontent/main/platform-tools-linux.zip";

// Global download progress
static std::atomic<float> downloadProgress(0.0f);
static std::atomic<bool> downloadComplete(false);
static std::atomic<bool> downloadError(false);
static std::string downloadStatus = "Ready to download";
static std::atomic<bool> extractionComplete(false);
static std::atomic<bool> extractionError(false);
static std::atomic<bool> extractionInProgress(false);

// Cache for platform tools existence check
static bool platformToolsExistCached = false;
static bool platformToolsChecked = false;

#ifdef DISABLE_DOWNLOADS
// Stub functions when downloads are disabled
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    return size * nmemb;
}

int ProgressCallback(void* clientp, long long dltotal, long long dlnow, long long ultotal, long long ulnow) {
    return 0;
}
#else
// Callback function for curl to track download progress
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    FILE* file = static_cast<FILE*>(userp);
    if (file) {
        return fwrite(contents, size, nmemb, file);
    }
    return size * nmemb;
}

int ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    if (dltotal > 0) {
        float progress = static_cast<float>(dlnow) / static_cast<float>(dltotal);
        downloadProgress.store(progress);
    }
    return 0;
}
#endif

std::string GetPlatformToolsURL() {
#ifdef _WIN32
    return PLATFORM_TOOLS_WINDOWS_URL;
#elif defined(__APPLE__)
    return PLATFORM_TOOLS_MACOS_URL;
#else
    return PLATFORM_TOOLS_LINUX_URL;
#endif
}

bool UnzipFile(const std::string& zipPath, const std::string& extractPath) {
    try {
        // Create extraction directory if it doesn't exist
        if (!std::filesystem::exists(extractPath)) {
            std::filesystem::create_directories(extractPath);
        }
        
        // Use system command to unzip
        std::string command;
        
#ifdef _WIN32
        // Windows: Use PowerShell's Expand-Archive
        command = "powershell -command \"Expand-Archive -Path '" + zipPath + "' -DestinationPath '" + extractPath + "' -Force\"";
#else
        // macOS and Linux: Use unzip command
        command = "unzip -o '" + zipPath + "' -d '" + extractPath + "'";
#endif
        
        int result = system(command.c_str());
        
        if (result == 0) {
            // Clean up the zip file after successful extraction
            std::filesystem::remove(zipPath);
            return true;
        } else {
            std::cerr << "Unzip command failed with exit code: " << result << std::endl;
            return false;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Exception during unzip: " << e.what() << std::endl;
        return false;
    }
}

bool PlatformToolsExist() {
    // Return cached result if already checked
    if (platformToolsChecked) {
        return platformToolsExistCached;
    }
    
    try {
        std::string adbKingPath = GetADBKingPath();
#ifdef _WIN32
        std::string platformToolsPath = adbKingPath + "\\platform-tools";
        std::string adbPath = platformToolsPath + "\\adb.exe";
#else
        std::string platformToolsPath = adbKingPath + "/platform-tools";
        std::string adbPath = platformToolsPath + "/adb";
#endif
        
        // Check if platform-tools folder exists and contains expected files
        if (std::filesystem::exists(platformToolsPath)) {
            // Check for some key files that should exist in platform-tools
            
            if (std::filesystem::exists(adbPath)) {
                platformToolsExistCached = true;
                platformToolsChecked = true;
                return true;
            }
        }
        
        platformToolsExistCached = false;
        platformToolsChecked = true;
        return false;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception checking platform tools: " << e.what() << std::endl;
        platformToolsExistCached = false;
        platformToolsChecked = true;
        return false;
    }
}

bool DownloadPlatformTools() {
#ifdef DISABLE_DOWNLOADS
    downloadStatus = "Download functionality not available - CURL not found";
    downloadError.store(true);
    return false;
#else
    try {
        // Check if platform tools already exist
        if (PlatformToolsExist()) {
            downloadStatus = "Platform tools already exist - skipping download";
            downloadComplete.store(true);
            extractionComplete.store(true);
            return true;
        }
        
        std::string url = GetPlatformToolsURL();
        std::string adbKingPath = GetADBKingPath();
#ifdef _WIN32
        std::string zipPath = adbKingPath + "\\platform-tools.zip";
#else
        std::string zipPath = adbKingPath + "/platform-tools.zip";
#endif
        
        // Initialize curl
        CURL* curl = curl_easy_init();
        if (!curl) {
            downloadStatus = "Failed to initialize curl";
            downloadError.store(true);
            return false;
        }
        
        // Open file for writing
        FILE* file = fopen(zipPath.c_str(), "wb");
        if (!file) {
            downloadStatus = "Failed to create output file";
            downloadError.store(true);
            curl_easy_cleanup(curl);
            return false;
        }
        
        // Set up curl options
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, nullptr);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        
        downloadStatus = "Downloading platform tools...";
        downloadProgress.store(0.0f);
        downloadError.store(false);
        
        // Perform the download
        CURLcode res = curl_easy_perform(curl);
        fclose(file);
        
        if (res != CURLE_OK) {
            downloadStatus = "Download failed: " + std::string(curl_easy_strerror(res));
            downloadError.store(true);
            curl_easy_cleanup(curl);
            return false;
        }
        
        curl_easy_cleanup(curl);
        downloadProgress.store(1.0f);
        downloadStatus = "Download completed successfully!";
        downloadComplete.store(true);
        
        // Extract the zip file
        downloadStatus = "Extracting platform tools...";
        extractionInProgress.store(true);
        if (UnzipFile(zipPath, adbKingPath)) {
            downloadStatus = "Download and extraction completed successfully!";
            extractionComplete.store(true);
            extractionInProgress.store(false);
            return true;
        } else {
            downloadStatus = "Download completed but extraction failed!";
            extractionError.store(true);
            extractionInProgress.store(false);
            return false;
        }
        
    } catch (const std::exception& e) {
        downloadStatus = "Exception during download: " + std::string(e.what());
        downloadError.store(true);
        extractionInProgress.store(false);
        return false;
    }
#endif
}

void StartDownloadThread() {
    std::thread downloadThread([]() {
        DownloadPlatformTools();
    });
    downloadThread.detach();
}

std::string GetADBKingPath() {
    std::string appDataPath;
    
#ifdef _WIN32
    // Windows: Use AppData\Local
    PWSTR localAppData = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData))) {
        // Convert wide string to narrow string
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, localAppData, -1, nullptr, 0, nullptr, nullptr);
        if (size_needed > 0) {
            char* buffer = new char[size_needed];
            WideCharToMultiByte(CP_UTF8, 0, localAppData, -1, buffer, size_needed, nullptr, nullptr);
            appDataPath = std::string(buffer);
            delete[] buffer;
        }
        CoTaskMemFree(localAppData);
    } else {
        // Fallback to environment variable
        char* envPath = nullptr;
        size_t len;
        _dupenv_s(&envPath, &len, "LOCALAPPDATA");
        if (envPath) {
            appDataPath = std::string(envPath);
            free(envPath);
        }
    }
    
#elif defined(__APPLE__)
    // macOS: Use ~/Library/Application Support 
    const char* homeDir = getenv("HOME");
    if (homeDir) {
        appDataPath = std::string(homeDir) + "/Library/Application Support";
    }
    
#else
    // Linux: Use ~/.local/share 
    const char* homeDir = getenv("HOME");
    if (homeDir) {
        appDataPath = std::string(homeDir) + "/.local/share";
    }
#endif
    
    if (appDataPath.empty()) {
        // Fallback for all platforms
        const char* homeDir = getenv("HOME");
        if (!homeDir) homeDir = getenv("USERPROFILE");
        if (homeDir) {
            appDataPath = std::string(homeDir);
        }
    }
    
#ifdef _WIN32
    return appDataPath + "\\ADBKing";
#else
    return appDataPath + "/ADBKing";
#endif
}

bool CreateADBKingFolder() {
    try {
        std::string adbKingPath = GetADBKingPath();
        
        // Check if folder already exists
        if (std::filesystem::exists(adbKingPath)) {
            return true;
        }
        
        // Create the folder
        if (std::filesystem::create_directories(adbKingPath)) {
            return true;
        } else {
            std::cerr << "Failed to create ADBKing folder at: " << adbKingPath << std::endl;
            return false;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Exception creating ADBKing folder: " << e.what() << std::endl;
        return false;
    }
}

void ShowSetup(bool& setupComplete) {
    try {
        // Create ADBKing folder only once when setup page shows
        static bool folderCreated = false;
        static bool downloadStarted = false;
        static bool autoDownloadTriggered = false;
        
        if (!folderCreated) {
            CreateADBKingFolder();
            folderCreated = true;
        }
        
        // Check if platform tools already exist (only once)
        if (!platformToolsChecked) {
            if (PlatformToolsExist()) {
                downloadComplete.store(true);
                extractionComplete.store(true);
                downloadStatus = "Platform tools already exist - ready to continue";
            }
        }
        
        // Auto-start download if platform tools don't exist and we haven't started yet
        if (!platformToolsExistCached && !autoDownloadTriggered && !downloadStarted) {
            autoDownloadTriggered = true;
            downloadStarted = true;
            StartDownloadThread();
        }
        
        // Auto-navigate when download and extraction are complete
        if (extractionComplete.load()) {
            // Initialize the global ADB path before completing setup
            GlobalConfig::InitializeADBPath();
            setupComplete = true;
            return;
        }
        
        // Get the window size
        ImVec2 windowSize = ImGui::GetIO().DisplaySize;
        
        // Create a full-window, non-draggable window
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(windowSize);
        
        // Window flags to make it non-draggable and full-window
        ImGuiWindowFlags windowFlags = 
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse;
        
        // Begin the window
        if (ImGui::Begin("Setup", nullptr, windowFlags)) {
            // Calculate center position for the content
            float contentWidth = 400.0f;
            float contentHeight = 200.0f;
            float centerX = (windowSize.x - contentWidth) * 0.5f;
            float centerY = (windowSize.y - contentHeight) * 0.5f;
            
            ImGui::SetCursorPos(ImVec2(centerX, centerY));
            
            // Setup title
            ImGui::SetCursorPos(ImVec2(centerX + 150.0f - 100.0f, centerY));
            ImGui::Text("Setting up platform tools");
            ImGui::Spacing();
            
            // Check if platform tools already exist (use cached result)
            if (platformToolsExistCached) {
                            // Auto-navigate when platform tools exist
            GlobalConfig::InitializeADBPath();
            setupComplete = true;
            } else {
                // Download section
                ImGui::Spacing();
                
                // Progress bar and status
                if (downloadStarted) {
                    float progress = downloadProgress.load();
                    ImGui::SetCursorPos(ImVec2(centerX, centerY + 100.0f));
                    ImGui::ProgressBar(progress, ImVec2(300, 20));
                    
                    // Show extraction loading indicator
                    if (extractionInProgress.load()) {
                        ImGui::SetCursorPos(ImVec2(centerX + 150.0f - 40.0f, centerY + 80.0f));
                        ImGui::Text("Extracting...");
                        
                        // Indeterminate progress bar with animated value
                        ImGui::SetCursorPos(ImVec2(centerX, centerY + 100.0f));
                        ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(300, 20), "Extracting...");
                    }
                    
                    if (downloadError.load()) {
                        ImGui::SetCursorPos(ImVec2(centerX + 150.0f - 80.0f, centerY + 130.0f));
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                        ImGui::Text("✗ Download failed!");
                        ImGui::PopStyleColor();
                        
                        ImGui::SetCursorPos(ImVec2(centerX + 150.0f - 100.0f, centerY + 160.0f));
                        if (ImGui::Button("Retry Download", ImVec2(200, 30))) {
                            downloadStarted = false;
                            downloadComplete.store(false);
                            downloadError.store(false);
                            extractionComplete.store(false);
                            extractionError.store(false);
                            downloadProgress.store(0.0f);
                            downloadStatus = "Ready to download";
                            autoDownloadTriggered = false;
                        }
                    }
                    
                    if (extractionError.load()) {
                        ImGui::SetCursorPos(ImVec2(centerX + 150.0f - 120.0f, centerY + 130.0f));
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
                        ImGui::Text("⚠ Download completed but extraction failed!");
                        ImGui::PopStyleColor();
                        
                        ImGui::SetCursorPos(ImVec2(centerX + 150.0f - 100.0f, centerY + 160.0f));
                        if (ImGui::Button("Retry Download", ImVec2(200, 30))) {
                            downloadStarted = false;
                            downloadComplete.store(false);
                            downloadError.store(false);
                            extractionComplete.store(false);
                            extractionError.store(false);
                            downloadProgress.store(0.0f);
                            downloadStatus = "Ready to download";
                            autoDownloadTriggered = false;
                        }
                    }
                }
            }
        }
        ImGui::End();
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in ShowSetup: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in ShowSetup" << std::endl;
    }
} 