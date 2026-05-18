#include "async_data_panel.h"
#include "core/capability/capability_service.h"
#include "global_config.h"
#include "imgui.h"

AsyncDataPanel::AsyncDataPanel(const std::string& title, const std::string& capabilityId, int refreshMs)
    : m_title(title)
    , m_capabilityId(capabilityId)
    , m_refreshMs(refreshMs)
{
}

void AsyncDataPanel::Draw() {
    if (!m_show) return;

    if (!ImGui::Begin(m_title.c_str(), &m_show)) {
        ImGui::End();
        return;
    }

    auto& service = CapabilityService::Get();
    auto now = std::chrono::steady_clock::now();

    // Detect device change → invalidate cache, reset
    std::string currentDevice = GlobalConfig::GetSelectedDeviceId();
    if (currentDevice != m_cachedDeviceId) {
        m_cachedDeviceId = currentDevice;
        service.InvalidateCache(m_capabilityId);
        m_phase = Phase::Idle;
        m_displayed.clear();
    }

    // Snapshot: load completed this frame → capture with flicker check
    auto capState = service.GetState(m_capabilityId);
    if (m_phase == Phase::Loading && !capState.loading) {
        if (capState.success) {
            m_error.clear();
            if (capState.data != m_displayed) {
                m_displayed = capState.data;
            }
            m_phase = Phase::WithData;
        } else {
            m_error = capState.error;
            m_phase = Phase::WithData;
        }
    }

    CommandContext ctx;
    ctx.deviceId = m_cachedDeviceId;

    // Immediate load: device selected, nothing shown yet
    if (m_phase == Phase::Idle && !m_cachedDeviceId.empty()) {
        service.Execute(m_capabilityId, ctx);
        m_phase = Phase::Loading;
        m_lastRefresh = now;
    }

    // Timer load
    if (m_phase != Phase::Loading && !m_cachedDeviceId.empty()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - m_lastRefresh).count();
        if (elapsed >= m_refreshMs) {
            service.Execute(m_capabilityId, ctx);
            m_phase = Phase::Loading;
            m_lastRefresh = now;
        }
    }

    // Render
    if (m_phase == Phase::Loading && m_displayed.empty()) {
        // Silent initial load — no "Loading..." text
    } else if (!m_error.empty() && m_displayed.empty()) {
        RenderError(m_error);
    } else if (m_displayed.empty()) {
        RenderEmpty();
    } else {
        RenderContent();
    }

    ImGui::End();
}

void AsyncDataPanel::TriggerRefresh() {
    CapabilityService::Get().InvalidateCache(m_capabilityId);
    m_phase = Phase::Idle;
}

void AsyncDataPanel::RenderError(const std::string& error) {
    ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", error.c_str());
}

void AsyncDataPanel::RenderEmpty() {
    ImGui::TextDisabled("No data available. Select a device.");
}
