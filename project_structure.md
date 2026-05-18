# Project Architecture — Seaweed / ADBKing

> **Stack:** C++17 · Dear ImGui 1.91+ · SDL2 · OpenGL 3.2 Core · cURL · CMake  
> **Target:** macOS (DMG), Windows (MSI), Linux (tar.gz)  
> **Version:** 1.0.1

---

## 1. Directory Layout

```
seaweed/
├── CMakeLists.txt                    # Build orchestration (C++17, SDL2, OpenGL, cURL)
├── Info.plist                        # macOS bundle metadata
├── entitlements.plist                # macOS code-signing entitlements
├── installer.wxs                     # Windows WiX MSI installer definition
├── build_dmg.sh                      # macOS DMG packaging script
├── build_debug.bat / .ps1            # Windows debug build scripts
├── run.sh                            # macOS run script
├── imgui/                            # Dear ImGui source (v1.91+, vendored)
│   ├── *.cpp / *.h                   # Core ImGui library
│   └── backends/
│       ├── imgui_impl_sdl2.cpp/h     # SDL2 platform backend
│       └── imgui_impl_opengl3.cpp/h  # OpenGL 3 renderer backend
├── desktop-icons/
│   ├── icon-mac.icns
│   ├── icon-windows.ico
│   └── icon-linux.png
├── .github/workflows/release.yml     # CI/CD: 3 parallel platform builds + GitHub Release
└── src/                              # ALL application source
    ├── main.cpp                      # ENTRY POINT — init loop, main menu, spotlight dispatch
    ├── setup.cpp/h                   # First-run: download/extract platform-tools via cURL
    ├── device_manager.cpp/h          # Singleton — ADB device discovery & monitoring
    ├── emulator_manager.cpp/h        # Singleton — AVD emulator lifecycle
    ├── global_config.cpp/h           # Namespace — ADB path, theme, font, delegates to DM
    ├── home.cpp/h                    # Home screen — alternative menu UI
    ├── apps_list.cpp/h               # Installed packages table with search/filter/pin
    ├── apps_cook_helper.cpp/h        # Namespace — async ADB package listing
    ├── apps_action_cook_helper.cpp/h # Background task queue — app actions (start/stop/etc.)
    ├── apps_pinned_store.cpp/h       # Pinned apps persistence (file-backed)
    ├── settings_screen.cpp/h         # Android settings intents list (200+ entries)
    ├── settings_helper.cpp/h         # ADB intent execution for settings
    ├── settings_pinned_store.cpp/h   # Pinned settings persistence
    ├── settings_action_helper.cpp/h  # Pin/unpin settings actions
    ├── call_logs.cpp/h               # Call log viewer (virtual scroll)
    ├── call_logs_helper.cpp/h        # ADB call log data fetching
    ├── lifecycle.cpp/h               # Activity lifecycle viewer (virtual scroll)
    ├── lifecycle_helper.cpp/h        # ADB dumpsys usagestats data fetching
    ├── adb_terminal.cpp/h            # Built-in ADB command terminal
    ├── spotlight.cpp/h               # Cmd+Shift search overlay
    ├── confirmation_dialog.cpp/h     # Reusable confirmation modal
    ├── preference_popup.cpp/h        # UI preferences (frame rounding, borders)
    ├── theme_popup.cpp/h             # Dark/Light theme toggling
    ├── no_device.cpp/h               # "No Device" help screen
    ├── window_actions_helper.cpp/h   # Cross-platform window close handler
    ├── key_value_store.cpp/h         # Generic file-based kv store
    └── calendar / contacts / install_app / keyboard_list /
        media_files / messages / notifications / processes /
        widgets_list — all *.cpp/h    # Feature stubs ("Coming soon")
```

---

## 2. Application Lifecycle (main.cpp)

```
main()
├── curl_global_init()              # One-time cURL init
├── SDL_Init(SDL_INIT_VIDEO|TIMER)
├── SDL_CreateWindow("ADBKing")     # Default 1280×720, remember position
├── SDL_GL_CreateContext()          # OpenGL 3.2 Core Profile
├── ImGui::CreateContext()
├── ImGui_ImplSDL2_InitForOpenGL()
├── ImGui_ImplOpenGL3_Init("#version 150")
│
├── Load/restore window state from ~/.seaweed_state
│
├── SETUP PHASE:
│   ├── PlatformToolsExist() ? skip → ShowSetup()
│   ├── GlobalConfig::InitializeADBPath()
│   ├── GlobalConfig::InitializeDeviceManager()   # spawns DeviceManager singleton
│   ├── GlobalConfig::StartDeviceMonitoring()     # background thread polls adb devices
│   └── EmulatorManager::GetInstance().SetAdbPath()
│
├── MAIN LOOP (every frame):
│   ├── SDL_PollEvent → SpotlightHandleSDLEvent / ImGui_ImplSDL2_ProcessEvent
│   ├── ImGui_ImplOpenGL3_NewFrame → SDL2_NewFrame → ImGui::NewFrame
│   ├── if !setupComplete → ShowSetup()
│   ├── else → BeginMainMenuBar + RenderSpotlight + conditional window draws
│   ├── ImGui::Render
│   ├── glClear → ImGui_ImplOpenGL3_RenderDrawData
│   └── SDL_GL_SwapWindow
│
└── CLEANUP:
    ├── SaveState (window geometry + dialog toggles)
    ├── ImGui_ImplOpenGL3_Shutdown → SDL2_Shutdown → ImGui::DestroyContext
    ├── GlobalConfig::StopDeviceMonitoring()     # joins background thread
    ├── SDL_Quit
    └── curl_global_cleanup
```

---

## 3. Navigation & UI Flow

There is **no router**. Dear ImGui is immediate-mode; windows are drawn conditionally each frame based on boolean flags (`WindowState` struct in `main.cpp`).

### Navigation mechanisms

| Mechanism | Trigger | Implementation |
|-----------|---------|----------------|
| **Main Menu Bar** | Always visible | `ImGui::BeginMainMenuBar()` in `main.cpp:250` |
| **Spotlight Search** | <kbd>Cmd</kbd>+<kbd>Shift</kbd> (double-tap shift) | `spotlight.cpp` — registered items with category + keywords |
| **Command Map** | Cmd+1, Cmd+2, etc. | Not yet implemented |

### Menu tree

```
Seaweed
├── About           → toggles WindowState.showAbout
├── Theme           → toggles WindowState.showTheme
├── Restart         → execvp self (full re-launch)
├── Close App       → running = false
├── Close All       → resets all WindowState booleans
├── ─────────────
└── Demo Window     → toggles WindowState.showDemo → ImGui::ShowDemoWindow()

Devices
├── Device List     → toggles WindowState.showDeviceList
├── Emulator        → toggles WindowState.showEmulator
└── Wireless        → toggles WindowState.showWireless (stub)
```

### Window visibility state machine

All 10 boolean flags in `WindowState` are simple toggle switches. The `Close All` menu item and each window's close button reset its flag to `false`. There is no stacking, layering, or dependency between windows.

### Spotlight search

- Items registered once (guard: `static bool registered`)
- Each item has: label, category, action callback, toggle callback, keywords, isToggle flag
- Search matches against label, category, and keywords
- History: last 10 interactions stored in `~/.seaweed_state`
- Keyboard-navigable (up/down/enter/escape)

---

## 4. Component Responsibilities

### 4.1 Core Infrastructure

| Component | File(s) | Responsibility |
|-----------|---------|----------------|
| **Entry Point** | `main.cpp` | SDL/OpenGL/ImGui init, main loop, cleanup, window state persistence, menu bar, spotlight registration |
| **GlobalConfig** | `global_config.cpp/h` | Namespace holding ADB path, theme, font pointer; delegates to DeviceManager; persists theme via KeyValueStore |
| **DeviceManager** | `device_manager.cpp/h` | Singleton — background thread polls `adb devices` every 2s, maintains device list with mutex, runs ADB commands via popen |
| **EmulatorManager** | `emulator_manager.cpp/h` | Singleton — scans for AVD via `emulator -list-avds`, starts/stops emulators, thread-safe |
| **KeyValueStore** | `key_value_store.cpp/h` | File-backed `key=value` store at `~/Library/Application Support/ADBKing/`; typed getters/setters |

### 4.2 Apps System

| Component | File(s) | Responsibility |
|-----------|---------|----------------|
| **AppsList** | `apps_list.cpp/h` | UI for installed apps table; search bar, filter (All/System/User), pin column |
| **AppsCookHelper** | `apps_cook_helper.cpp/h` | Namespace — spawns thread for `pm list packages`, parses output, supports filter + search |
| **AppsActionCookHelper** | `apps_action_cook_helper.cpp/h` | Background task queue → worker thread pattern; sync + async wrappers for: start, stop, uninstall, clear, enable, disable, open info, pin, etc. |
| **AppsPinnedStore** | `apps_pinned_store.cpp/h` | File-backed pinned packages set (`unordered_set` + mutex) |

### 4.3 Settings System

| Component | File(s) | Responsibility |
|-----------|---------|----------------|
| **SettingsScreen** | `settings_screen.cpp/h` | UI for 200+ Android settings shortcuts; search + pin |
| **SettingsHelper** | `settings_helper.cpp/h` | Executes ADB intent commands to open system settings panels |
| **SettingsPinnedStore** | `settings_pinned_store.cpp/h` | File-backed pinned settings set |
| **SettingsActionHelper** | `settings_action_helper.cpp/h` | Pin/unpin helpers |

### 4.4 Data Viewers

| Component | File(s) | Responsibility |
|-----------|---------|----------------|
| **CallLogs** | `call_logs.cpp/h` | Table UI with virtual scroll (`ImGuiListClipper`), async ADB fetch |
| **CallLogsHelper** | `call_logs_helper.cpp/h` | Queries `content://call_log/calls` via ADB, parses structured output |
| **Lifecycle** | `lifecycle.cpp/h` | Table UI with virtual scroll, async ADB fetch |
| **LifecycleHelper** | `lifecycle_helper.cpp/h` | Queries `dumpsys usagestats` via ADB, parses activity lifecycle data |

### 4.5 Utilities & UX

| Component | File(s) | Responsibility |
|-----------|---------|----------------|
| **Spotlight** | `spotlight.cpp/h` | Search overlay — keyboard nav, history, item registration |
| **ConfirmationDialog** | `confirmation_dialog.cpp/h` | Generic Yes/No modal with "Don't ask again" persistence |
| **PreferencePopup** | `preference_popup.cpp/h` | Slider for FrameRounding, toggles for border sizes |
| **ThemePopup** | `theme_popup.cpp/h` | Dark/Light radio selection |
| **Setup** | `setup.cpp/h` | First-run: cURL download + zip extraction of platform-tools |
| **ADBTerminal** | `adb_terminal.cpp/h` | Streaming ADB command execution in a scrollable console |
| **WindowActionsHelper** | `window_actions_helper.cpp/h` | Cross-platform window close |

### 4.6 Stub Screens

`calendar`, `contacts`, `install_app`, `keyboard_list`, `media_files`, `messages`, `notifications`, `processes`, `widgets_list` — all contain a single `Show<Name>()` function with a "Coming soon..." placeholder.

---

## 5. Data Flow

### 5.1 ADB Communication

All device interaction goes through a single pattern:

```
UI Component
  → Helper (e.g., CallLogsHelper::GetCallLogs)
    → AppsActionCookHelper::ExecuteADBCommand or DeviceManager::ExecuteCommand
      → popen("adb -s <deviceId> shell <command>", "r")   ← SYNCHRONOUS
        → stdout parsed and returned as std::string
```

Key ADB commands by feature:
| Feature | Command | Responsible |
|---------|---------|-------------|
| Device discovery | `adb devices` | `DeviceManager::UpdateDeviceListInternal()` |
| Device properties | `adb -s <id> shell getprop ro.product.brand/model` | `DeviceManager::GetHumanNameByID()` |
| Package listing | `adb -s <id> shell pm list packages [-3\|-s]` | `AppsCookHelper::LoadPackagesListInternal()` |
| App actions | `adb -s <id> shell monkey -p <pkg> 1` etc. | `AppsActionCookHelper::*` |
| Call logs | `adb -s <id> shell content query --uri content://call_log/calls` | `CallLogsHelper::GetCallLogs()` |
| Lifecycle | `adb -s <id> shell dumpsys usagestats` | `LifecycleHelper::GetLifecycleData()` |
| Settings | `adb -s <id> shell am start -a <intent>` | `SettingsHelper::ExecuteSettingsIntent()` |
| Platform tools | cURL download from GitHub | `setup.cpp::DownloadPlatformTools()` |
| Home button | `adb -s <id> shell input keyevent KEYCODE_HOME` | `AppsActionCookHelper::GoHome()` |

### 5.2 Data Persistence

```
~/.seaweed_state                              # Window geometry + dialog toggles (raw fprintf)
~/Library/Application Support/ADBKing/
├── key_value_store.txt                       # Theme, style prefs, dialog prefs (key=value)
├── pinned_apps.txt                           # Pinned package names (unordered_set)
└── pinned_settings.txt                       # Pinned setting intent actions (unordered_set)
```

All stores read full file into memory on first access, write full file on every mutation. No database.

---

## 6. Concurrency & Performance

### 6.1 Threading Model

```
MAIN THREAD (UI):
  SDL event loop → ImGui render → swap buffers
  (must never block)

BACKGROUND THREADS:
├── DeviceManager: polling thread — `adb devices` every 2s
│   └── synchronized via std::mutex on device list
│
├── AppsActionCookHelper: worker thread — task queue (std::queue + condition_variable)
│   ├── InitializeBackgroundTaskManager() spawns thread
│   └── AddBackgroundTask() pushes lambda, notifies CV
│
├── AppsCookHelper: detached thread per package list load
│   └── synchronized via std::mutex on package list
│
├── std::async (launch::async) in CallLogs / Lifecycle
│   └── each fetch spawns a future, result retrieved in next frame
│
└── Spotlight search filtering: detached thread with 300ms debounce
```

### 6.2 Heavy Operations & Mitigations

| Operation | Cost | Mitigation |
|-----------|------|------------|
| `adb devices` polling | ~100ms every 2s | Background thread, never blocks UI |
| `pm list packages` | 500ms–2s | Background thread with mutex-guarded result |
| App actions (start/stop/uninstall) | 200ms–5s | Task queue on worker thread; status polling via `GetCurrentStatus()` |
| `content query` (call logs) | 1s–10s+ | `std::async` + loading spinner + virtual scroll |
| `dumpsys usagestats` | 1s–5s | `std::async` + loading spinner + virtual scroll |
| cURL download (platform-tools ~5MB) | 5s–30s | Background download, progress bar in setup UI |
| Theme + font reload | ~50ms | Cached font pointer, `g_fontNeedsReload` flag |

### 6.3 UI Rendering Performance

- **Virtual scrolling** via `ImGuiListClipper` in `call_logs.cpp` and `lifecycle.cpp` — only visible rows are constructed each frame
- **Immediate-mode efficiency**: zero retained state — all UI rebuilt every frame; only visible windows are drawn
- **Font caching**: `g_currentFont` stored globally; only reloaded when size/theme changes
- **No textures/image assets**: all UI is glyph/text/shape-based via ImGui draw commands

---

## 7. Build System

### CMakeLists.txt structure

```
project(ADBKing CXX)
├── set(CMAKE_CXX_STANDARD 17)
├── find_package(SDL2 ... OR vcpkg on Windows)
├── find_package(CURL ...)              # optional: -DDISABLE_DOWNLOADS
├── link_libraries(SDL2::SDL2 OpenGL CURL::libcurl)
├── file(GLOB IMGUI_SRC "imgui/*.cpp" "imgui/backends/*.cpp")
├── file(GLOB APP_SRC "src/*.cpp")
├── add_executable(imgui_app ${IMGUI_SRC} ${APP_SRC})
│
├── macOS: target macOS 12.0, CREATE_BUNDLE, .app structure, Info.plist, frameworks
├── Windows: static MSVC runtime (/MT), vcpkg triplet x64-windows-static
└── Linux: standard ELF, pthreads
```

### CI/CD pipeline (`.github/workflows/release.yml`)

Three parallel jobs → unified GitHub Release:
- **macOS ARM64** → CMake + brew → `.app` bundle → ad-hoc sign → DMG
- **Windows x64** → vcpkg SDL2/curl static → CMake VS2022 → WiX MSI
- **Linux x64** → apt SDL2/curl → CMake → make → tar.gz

---

## 8. Library Dependency Graph

```
imgui_app
├── Dear ImGui (imgui/*.cpp)          # Immediate mode GUI framework
│   ├── imgui_impl_sdl2              # SDL2 platform adapter
│   └── imgui_impl_opengl3          # OpenGL3 renderer adapter
├── SDL2                              # Windowing, input, OpenGL context creation
├── OpenGL (3.2 Core Profile)         # Hardware-accelerated rendering
└── cURL (optional)                   # HTTP download for platform-tools
```

---

## 9. State Management Summary

| State | Scope | Location | Persistence |
|-------|-------|----------|-------------|
| ADB path | Global | `GlobalConfig::g_adbPath` | Derived from `GetADBKingPath()` |
| Theme | Global | `GlobalConfig::g_currentTheme` | `key_value_store.txt` |
| Font pointer | Global | `GlobalConfig::g_currentFont` | Rebuilt on launch |
| Device list | Global | `DeviceManager` singleton | Not persisted (re-fetched) |
| Selected device | Global | `DeviceManager::m_selectedDeviceId` | Not persisted |
| Window geometry | App | `main.cpp` locals | `~/.seaweed_state` |
| Dialog toggles | App | `WindowState` struct in main | `~/.seaweed_state` |
| Spotlight history | App | `spotlight.cpp` | `~/.seaweed_state` |
| Pinned apps | Feature | `AppsPinnedStore` | `pinned_apps.txt` |
| Pinned settings | Feature | `SettingsPinnedStore` | `pinned_settings.txt` |
| Action status | Feature | `AppsActionCookHelper` static | Not persisted (ephemeral) |
| "Don't ask again" | Feature | `confirmation_dialog.cpp` | `key_value_store.txt` |

---

## 10. Key Design Decisions & Trade-offs

| Decision | Rationale | Trade-off |
|----------|-----------|-----------|
| Immediate-mode GUI (Dear ImGui) | Rapid iteration, no CSS/DOM, easy to embed | No native widgets, accessibility barrier, no text selection |
| `popen` for ADB commands | Simplest cross-platform subprocess API | Blocking; requires background threads for all I/O |
| File-based persistence | Zero dependencies (no SQLite/NLite) | Full rewrite on every write, no ACID |
| Vendored ImGui source | No package manager dependency, exact version control | Increased repo size (+10MB), manual updates |
| Static runtime on Windows | No VC++ redistributable needed | Larger binary, no hotfix updates from OS |
| cURL SSL verification disabled | Quick setup for GitHub downloads | MITM risk during first-run download |
| Namespace globals (GlobalConfig) | Simpler than DI for a single-window app | Tight coupling, hard to unit test |
| Background task queue pattern | Decouples UI from ADB latency | Status polling via busy-loop instead of callbacks |
| `std::async` for data viewers | Minimal boilerplate for one-shot async | No cancellation support, no progress reporting |

---

## 11. Stub / Incomplete Features

These modules exist as placeholders with no ADB integration:

- **Calendar** — `ShowCalendar()` "Coming soon"
- **Contacts** — `ShowContacts()` "Coming soon"
- **Install App** — `ShowInstallApp()` "Coming soon"
- **Keyboard List** — `ShowKeyboardList()` "Coming soon"
- **Media Files** — `ShowMediaFiles()` "Coming soon"
- **Messages** — `ShowMessages()` "Coming soon"
- **Notifications** — `ShowNotifications()` "Coming soon"
- **Processes** — `ShowProcesses()` "Coming soon"
- **Widgets List** — `ShowWidgetsList()` "Coming soon"
- **Wireless** — `ShowWireless()` "Coming soon"
- **Download APK** — `DownloadAPK()` "TODO: implement"
- **Show More** — `ShowMore()` "TODO: implement"

---

## 12. Recommendations for AI / Automation

1. **Add a proper async/await pattern** — Replace status-polling with `std::promise`/`std::future` or callback-based completion.
2. **Add cancellation tokens** — `std::async` launches cannot be cancelled; long queries block until done.
3. **Extract ADB transport layer** — `DeviceManager::ExecuteCommand()` and `ExecCmd()` duplicate the `popen` pattern; unify into an `AdbClient` class.
4. **Add structured logging** — All errors go to `std::cerr`; no runtime log file for diagnostics.
5. **Test harness** — Zero tests today; ImGui makes unit testing challenging but ADB helpers can be tested by mocking `popen`.
6. **Memory management** — All allocations are raw; no RAII wrappers for FILE* handles from `popen`.
7. **Wireless ADB pairing** — The `net` / wireless tab is a stub; pairing via `adb pair` + `adb connect` is the obvious next feature.
