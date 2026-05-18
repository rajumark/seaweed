#pragma once
#include "capability.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <thread>
#include <queue>
#include <condition_variable>
#include <chrono>

class CapabilityService {
public:
    static CapabilityService& Get();
    ~CapabilityService();

    struct State {
        uint64_t requestId = 0;
        uint64_t completedRequestId = 0;
        bool loading = false;
        bool success = false;
        std::vector<std::string> data;
        std::string error;
        std::string lastDeviceId;
        std::chrono::steady_clock::time_point lastLoadTime;
    };

    uint64_t Execute(const std::string& capabilityId, CommandContext ctx = {});
    bool IsLoading(const std::string& capabilityId);
    State GetState(const std::string& capabilityId);
    void InvalidateCache(const std::string& capabilityId);

private:
    CapabilityService();
    void WorkerLoop();

    struct Entry {
        Capability* capability = nullptr;
        State state;
    };

    std::unordered_map<std::string, Entry> m_entries;
    std::mutex m_mutex;

    struct Task {
        std::string capabilityId;
        uint64_t requestId;
        CommandContext ctx;
    };
    std::queue<Task> m_queue;
    std::condition_variable m_cv;
    std::thread m_worker;
    std::atomic<bool> m_running{true};
};
