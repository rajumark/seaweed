# ADBKing Architecture

## Layer Diagram

```
┌─────────────────────────────────────────────┐
│                  Panels                       │
│  (apps_panel, theme_panel, ...)               │
│  Each inherits AsyncDataPanel                │
├─────────────────────────────────────────────┤
│              PanelRegistry                    │
│  Register / DrawAll / Toggle / SaveState     │
├─────────────────────────────────────────────┤
│   AsyncDataPanel (base class)                │
│   - Timer + immediate load logic            │
│   - Phase state machine (Idle/Loading/Data)  │
│   - Snapshot flicker protection             │
│   - Device-change detection                 │
├─────────────────────────────────────────────┤
│            CapabilityService                 │
│  - Single background worker (task queue)     │
│  - Cache per capability (fixed TTL)         │
│  - Request IDs prevent stale-write races    │
│  - ADB command execution via DeviceManager  │
├─────────────────────────────────────────────┤
│           CapabilityRegistry                 │
│  Register / Find capabilities by id         │
├─────────────────────────────────────────────┤
│        Capability (metadata struct)          │
│  - id, name, category, commandTemplate      │
│  - cachePolicy, cacheTtlMs, lineParser      │
│  - requiresDevice, keywords                 │
├─────────────────────────────────────────────┤
│           Core Infrastructure                │
│  GlobalConfig  DeviceManager  EmulatorManager│
│  PanelRegistry  CommandRegistry  Spotlight   │
└─────────────────────────────────────────────┘
```

## How to add a new screen

1. **Register the ADB capability** in `register_capabilities.cpp`:

```cpp
reg.Register({
    "list_devices",                          // unique id
    "List Devices",                          // human name
    "Devices",                               // category
    "devices -l",                            // adb command (appended after "adb -s <id> ")
    CachePolicy::Fixed,                      // None / Fixed / UntilChange
    30000,                                   // cache TTL ms
    [](const std::string& raw) {             // line parser
        std::vector<std::string> lines;
        // parse raw output, populate lines
        return lines;
    },
    {"devices", "connected", "list"},        // keywords
    false                                    // requiresDevice?
});
```

2. **Create the panel class** in `src/panels/`:

```cpp
#include "core/ui/async_data_panel.h"

class DevicesPanel : public AsyncDataPanel {
public:
    DevicesPanel() : AsyncDataPanel("Devices", "list_devices", 10000) {}
protected:
    void RenderContent() override {
        for (const auto& line : Displayed())
            ImGui::Text("%s", line.c_str());
    }
};
```

3. **Register the panel** in `register_all_panels.cpp`:

```cpp
static DevicesPanel* s_devicesPanel = nullptr;
static void DrawDevicesPanel() { if (s_devicesPanel) s_devicesPanel->Draw(); }

void RegisterAllPanels() {
    // ... existing panels ...
    s_devicesPanel = new DevicesPanel();
    PanelRegistry::Get().Register({
        "devices", "Devices", "Devices",
        DrawDevicesPanel,
        s_devicesPanel->GetShowPtr(),
        {"connected", "adb"}
    });
}
```

**That's it.** Auto-appears in menu bar and Spotlight.

## CommandContext

Every async execution carries a `CommandContext{ deviceId }` captured at request time. This prevents races where the selected device changes between request and execution. The context is stored in the task queue alongside the capability ID and request ID.

Panels automatically pass their cached device ID via context. The service falls back to `GlobalConfig::GetSelectedDeviceId()` if context is empty (backward compat).

## Key Design Decisions

### Why CapabilityService instead of per-panel threads?
At scale (1000+ capabilities), per-panel threads are unmanageable. A single background worker with a task queue is efficient and avoids thread explosion. Request IDs prevent stale writes.

### Why AsyncDataPanel instead of manual orchestration?
Every panel needs identical lifecycle logic (timer, loading, snapshot, device-change). Extracting it into a base class eliminates duplication and ensures consistent behavior.

### No event bus (yet)
Device-change detection is done via polling (`GetSelectedDeviceId()` compared per frame). This is sufficient for the current scale. If performance becomes an issue, replace with a push model.

### Cache TTL over deduplication
Capability-level TTLs (30s for packages, 5min for device props) are simpler than deduplication. If a user toggles a panel rapidly, the cache serves stale data within TTL instead of re-executing ADB.

## Directory Layout

```
src/
├── core/
│   ├── capability/
│   │   ├── capability.h                  # Capability metadata struct
│   │   ├── capability_registry.h/.cpp    # Registry: register, find
│   │   ├── capability_service.h/.cpp     # Executor + cache + worker
│   │   └── register_capabilities.h/.cpp  # All capability registrations
│   ├── device/                            # (future) device events
│   ├── registry/
│   │   ├── panel_registry.h/.cpp          # Panel management
│   │   └── command_registry.h/.cpp        # Spotlight commands
│   └── ui/
│       └── async_data_panel.h/.cpp        # Panel base class
├── panels/
│   ├── about_panel.cpp
│   ├── apps_panel.cpp
│   ├── device_list_panel.cpp
│   ├── emulator_panel.cpp
│   ├── theme_panel.cpp
│   ├── wireless_panel.cpp
│   └── register_all_panels.h/.cpp
└── main.cpp
```

## Future Directions (When to Add)

| Pattern | When to introduce |
|---------|------------------|
| Event bus (push over poll) | When polling 50+ panels causes measurable perf drop |
| Cancellation tokens | When ADB commands block for >5s on slow devices |
| Incremental diffing | When package lists exceed 5k entries and updates stutter |
| Multi-device sessions | When the app supports side-by-side device comparison |
| Streaming capabilities | When adding logcat/screenrecord/chrome devtools |
| Generic AsyncDataPanel<T> template | When typed data (not just strings) is needed across panels |
