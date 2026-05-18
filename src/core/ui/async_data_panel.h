#pragma once
#include <string>
#include <vector>
#include <chrono>

class AsyncDataPanel {
public:
    AsyncDataPanel(const std::string& title, const std::string& capabilityId, int refreshMs = 4000);
    virtual ~AsyncDataPanel() = default;

    void Draw();
    bool* GetShowPtr() { return &m_show; }
    const std::string& GetTitle() const { return m_title; }

    void SetRefreshMs(int ms) { m_refreshMs = ms; }
    void TriggerRefresh();

protected:
    virtual void RenderContent() = 0;
    virtual void RenderError(const std::string& error);
    virtual void RenderEmpty();

    // Subclasses read the snapshot-safe displayed data
    const std::vector<std::string>& Displayed() const { return m_displayed; }

    bool m_show = false;
    std::string m_title;
    std::string m_capabilityId;

private:
    enum class Phase { Idle, Loading, WithData };
    Phase m_phase = Phase::Idle;
    Phase m_prevPhase = Phase::Idle;

    int m_refreshMs;
    std::chrono::steady_clock::time_point m_lastRefresh;
    std::string m_cachedDeviceId;
    std::vector<std::string> m_displayed;
    std::string m_error;
};
