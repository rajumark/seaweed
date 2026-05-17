#pragma once
#include <string>
#include <functional>

// Confirmation dialog types
enum class ConfirmationType {
    Uninstall,
    ClearData
};

// Confirmation dialog state
struct ConfirmationDialogState {
    bool isOpen = false;
    ConfirmationType type = ConfirmationType::Uninstall;
    std::string packageName;
    std::string title;
    std::string message;
    bool doNotShowAgain = false;
    std::function<void()> onConfirm;
    std::function<void()> onCancel;
};

// Confirmation dialog functions
void ShowConfirmationDialog();
void OpenConfirmationDialog(ConfirmationType type, const std::string& packageName, 
                           std::function<void()> onConfirm, std::function<void()> onCancel = nullptr);
bool ShouldShowConfirmation(ConfirmationType type);
