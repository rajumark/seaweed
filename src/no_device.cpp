#include "no_device.h"
#include "imgui.h"
#include <string>

void ShowNoDevice() {
    ImGui::BeginChild("NoDeviceContent");
    
    // Title
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20.0f);
    ImGui::TextColored(ImVec4(0.2f, 0.6f, 1.0f, 1.0f), "No Device Connected");
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Main content area
    float contentWidth = ImGui::GetContentRegionAvail().x;
    float centerX = (contentWidth - 600.0f) * 0.5f;
    if (centerX > 0) {
        ImGui::SetCursorPosX(centerX);
    }
    
    ImGui::BeginGroup();
    ImGui::SetNextItemWidth(600.0f);
    
    // Connection Instructions Section
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "How to Connect Your USB Device");
    ImGui::Spacing();
    
    ImGui::TextWrapped("Follow these steps to connect your Android device:");
    ImGui::Spacing();
    
    // Step 1
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Step 1: Enable Developer Options");
    ImGui::BulletText("Go to Settings > About Phone");
    ImGui::BulletText("Tap 'Build Number' 7 times to enable Developer Options");
    ImGui::BulletText("Go back to Settings > System > Developer Options");
    ImGui::Spacing();
    
    // Step 2
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Step 2: Enable USB Debugging");
    ImGui::BulletText("In Developer Options, enable 'USB Debugging'");
    ImGui::BulletText("Enable 'USB Debugging (Security Settings)' if available");
    ImGui::Spacing();
    
    // Step 3
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Step 3: Connect Your Device");
    ImGui::BulletText("Connect your device to computer via USB cable");
    ImGui::BulletText("Select 'File Transfer' or 'MTP' mode when prompted");
    ImGui::BulletText("Allow USB debugging when the popup appears on your device");
    ImGui::Spacing();
    
    // Step 4
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Step 4: Verify Connection");
    ImGui::BulletText("Your device should appear in the device tabs above");
    ImGui::BulletText("If not, try disconnecting and reconnecting the USB cable");
    ImGui::Spacing();
    
    ImGui::Separator();
    ImGui::Spacing();
    
    // Troubleshooting Section
    ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "Troubleshooting");
    ImGui::Spacing();
    
    ImGui::TextWrapped("If you're having trouble connecting your device:");
    ImGui::Spacing();
    
    // Common Issues
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Common Issues:");
    ImGui::Spacing();
    
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Device not recognized:");
    ImGui::BulletText("Try a different USB cable");
    ImGui::BulletText("Try a different USB port");
    ImGui::BulletText("Install/update USB drivers for your device");
    ImGui::Spacing();
    
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "USB Debugging not working:");
    ImGui::BulletText("Make sure Developer Options is enabled");
    ImGui::BulletText("Toggle USB Debugging off and on again");
    ImGui::BulletText("Revoke USB debugging authorizations and try again");
    ImGui::Spacing();
    
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Permission denied:");
    ImGui::BulletText("Check if USB debugging popup appeared on device");
    ImGui::BulletText("Tap 'Allow' when the popup appears");
    ImGui::BulletText("Check 'Always allow from this computer' if available");
    ImGui::Spacing();
    
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "ADB not found:");
    ImGui::BulletText("Make sure ADB is installed and in your PATH");
    ImGui::BulletText("Try running 'adb devices' in terminal to test");
    ImGui::BulletText("Restart ADB server: 'adb kill-server' then 'adb start-server'");
    ImGui::Spacing();
    
    // Additional Help
    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Need more help?");
    ImGui::BulletText("Check your device manufacturer's website for USB drivers");
    ImGui::BulletText("Ensure your device is unlocked when connecting");
    ImGui::BulletText("Try connecting in different USB modes (MTP, PTP, etc.)");
    ImGui::BulletText("Restart both your computer and device if issues persist");
    
    ImGui::EndGroup();
    
    ImGui::EndChild();
}
