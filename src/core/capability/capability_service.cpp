#include "capability_service.h"
#include "capability_registry.h"
#include "device_manager.h"
#include "global_config.h"
#include <sstream>
#include <algorithm>

CapabilityService& CapabilityService::Get() {
    static CapabilityService instance;
    return instance;
}

CapabilityService::CapabilityService() {
    m_worker = std::thread(&CapabilityService::WorkerLoop, this);
}

CapabilityService::~CapabilityService() {
    m_running.store(false);
    m_cv.notify_one();
    if (m_worker.joinable())
        m_worker.join();
}

uint64_t CapabilityService::Execute(const std::string& capabilityId, CommandContext ctx) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_entries.find(capabilityId);
    if (it == m_entries.end()) {
        Capability* cap = CapabilityRegistry::Get().Find(capabilityId);
        if (!cap) return 0;
        Entry entry;
        entry.capability = cap;
        it = m_entries.emplace(capabilityId, std::move(entry)).first;
    }

    Entry& entry = it->second;
    if (entry.state.loading) return entry.state.requestId;

    // Resolve device if not provided
    if (ctx.deviceId.empty())
        ctx.deviceId = GlobalConfig::GetSelectedDeviceId();

    // Check cache (only if same device)
    if (entry.capability->cachePolicy == CachePolicy::Fixed && ctx.deviceId == entry.state.lastDeviceId) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - entry.state.lastLoadTime).count();
        if (elapsed < entry.capability->cacheTtlMs && entry.state.success)
            return entry.state.completedRequestId;
    }

    entry.state.requestId++;
    entry.state.loading = true;
    entry.state.lastDeviceId = ctx.deviceId;
    m_queue.push({capabilityId, entry.state.requestId, std::move(ctx)});
    m_cv.notify_one();
    return entry.state.requestId;
}

bool CapabilityService::IsLoading(const std::string& capabilityId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_entries.find(capabilityId);
    return it != m_entries.end() && it->second.state.loading;
}

CapabilityService::State CapabilityService::GetState(const std::string& capabilityId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_entries.find(capabilityId);
    if (it == m_entries.end()) return {};
    return it->second.state;
}

void CapabilityService::InvalidateCache(const std::string& capabilityId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_entries.find(capabilityId);
    if (it != m_entries.end()) {
        it->second.state.lastLoadTime = {};
        it->second.state.success = false;
    }
}

void CapabilityService::WorkerLoop() {
    while (m_running.load()) {
        Task task;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] {
                return !m_queue.empty() || !m_running.load();
            });
            if (!m_running.load()) return;
            task = m_queue.front();
            m_queue.pop();
        }

        Entry entry;
        uint64_t requestId;
        std::string commandTemplate;
        bool requiresDevice;
        CommandContext ctx;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_entries.find(task.capabilityId);
            if (it == m_entries.end()) continue;
            entry = it->second;
            requestId = task.requestId;
            commandTemplate = entry.capability->commandTemplate;
            requiresDevice = entry.capability->requiresDevice;
            ctx = task.ctx;
        }

        if (ctx.deviceId.empty() && requiresDevice) {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_entries.find(task.capabilityId);
            if (it != m_entries.end() && it->second.state.requestId == requestId) {
                it->second.state.loading = false;
                it->second.state.success = false;
                it->second.state.error = "No device selected";
            }
            continue;
        }

        std::string adbPath = GlobalConfig::GetADBPath();
        if (adbPath.empty() && requiresDevice) {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_entries.find(task.capabilityId);
            if (it != m_entries.end() && it->second.state.requestId == requestId) {
                it->second.state.loading = false;
                it->second.state.success = false;
                it->second.state.error = "ADB path not configured";
            }
            continue;
        }

        std::string command;
        if (requiresDevice)
            command = "\"" + adbPath + "\" -s " + ctx.deviceId + " " + commandTemplate;
        else
            command = "\"" + adbPath + "\" " + commandTemplate;

        std::string output = DeviceManager::GetInstance().ExecuteCommand(command);

        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_entries.find(task.capabilityId);
        if (it == m_entries.end() || it->second.state.requestId != requestId)
            continue;

        auto& state = it->second.state;
        state.loading = false;
        state.lastLoadTime = std::chrono::steady_clock::now();
        state.completedRequestId = requestId;

        if (output.empty()) {
            state.success = false;
            state.error = "No output from ADB command";
            state.data.clear();
        } else {
            state.success = true;
            state.error.clear();
            if (entry.capability->lineParser) {
                state.data = entry.capability->lineParser(output);
            } else {
                std::vector<std::string> lines;
                std::istringstream iss(output);
                std::string line;
                while (std::getline(iss, line)) {
                    line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                    if (!line.empty())
                        lines.push_back(line);
                }
                state.data = std::move(lines);
            }
        }
    }
}
