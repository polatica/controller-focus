#include <Geode/Geode.hpp>
#include <Geode/modify/CCApplication.hpp>
#include <Geode/modify/CCEGLView.hpp>
#include <Geode/utils/Keyboard.hpp>
#include <array>
#include <cmath>
#include "FocusGate.hpp"

using namespace geode::prelude;

namespace {
std::array<FocusGate, XUSER_MAX_COUNT> gates;

bool isForeground() {
    auto window = GetForegroundWindow();
    if (!window) return false;
    DWORD pid = 0;
    GetWindowThreadProcessId(window, &pid);
    return pid == GetCurrentProcessId();
}

bool isNeutral(XINPUT_GAMEPAD const& pad) {
    // same deadzone as geode, small drift shouldn't count
    auto centered = [](SHORT axis) {
        return std::abs(static_cast<int>(axis)) <= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
    };
    return pad.wButtons == 0 &&
        pad.bLeftTrigger <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD &&
        pad.bRightTrigger <= XINPUT_GAMEPAD_TRIGGER_THRESHOLD &&
        centered(pad.sThumbLX) && centered(pad.sThumbLY) &&
        centered(pad.sThumbRX) && centered(pad.sThumbRY);
}

// release whatever GD thinks is still held.
// no SendInput here, it would end up in the other game after alt-tab
void releaseController(CXBOXController* controller, int userIndex) {
    struct Binding { bool CXBOXController::*state; enumKeyCodes key; };
    static constexpr Binding bindings[] = {
        {&CXBOXController::m_buttonA, CONTROLLER_A},
        {&CXBOXController::m_buttonB, CONTROLLER_B},
        {&CXBOXController::m_buttonX, CONTROLLER_X},
        {&CXBOXController::m_buttonY, CONTROLLER_Y},
        {&CXBOXController::m_start, CONTROLLER_Start},
        {&CXBOXController::m_back, CONTROLLER_Back},
        {&CXBOXController::m_lb, CONTROLLER_LB},
        {&CXBOXController::m_rb, CONTROLLER_RB},
        {&CXBOXController::m_lt, CONTROLLER_LT},
        {&CXBOXController::m_rt, CONTROLLER_RT},
        {&CXBOXController::m_dpadUp, CONTROLLER_Up},
        {&CXBOXController::m_dpadDown, CONTROLLER_Down},
        {&CXBOXController::m_dpadLeft, CONTROLLER_Left},
        {&CXBOXController::m_dpadRight, CONTROLLER_Right},
        {&CXBOXController::m_leftThumbUp, CONTROLLER_LTHUMBSTICK_UP},
        {&CXBOXController::m_leftThumbDown, CONTROLLER_LTHUMBSTICK_DOWN},
        {&CXBOXController::m_leftThumbLeft, CONTROLLER_LTHUMBSTICK_LEFT},
        {&CXBOXController::m_leftThumbRight, CONTROLLER_LTHUMBSTICK_RIGHT},
        {&CXBOXController::m_rightThumbUp, CONTROLLER_RTHUMBSTICK_UP},
        {&CXBOXController::m_rightThumbDown, CONTROLLER_RTHUMBSTICK_DOWN},
        {&CXBOXController::m_rightThumbLeft, CONTROLLER_RTHUMBSTICK_LEFT},
        {&CXBOXController::m_rightThumbRight, CONTROLLER_RTHUMBSTICK_RIGHT},
    };
    const int offset = userIndex > 1 ? 1 : 0;
    for (auto const& binding : bindings) {
        if (!(controller->*binding.state)) continue;
        controller->*binding.state = false;
        auto key = static_cast<enumKeyCodes>(static_cast<int>(binding.key) + offset);
        KeyboardInputData data(key, KeyboardInputData::Action::Release, {},
            utils::getInputTimestamp(), KeyboardModifier::None);
        if (KeyboardInputEvent(key).send(data) == ListenerResult::Propagate) {
            CCKeyboardDispatcher::get()->dispatchKeyboardMSG(key, false, false, data.timestamp);
        }
    }
}
}

class $modify(ControllerFocusApplication, CCApplication) {
    static void onModify(auto& self) {
        (void)self.setHookPriorityPre("cocos2d::CCApplication::updateControllerKeys", Priority::VeryEarly);
        (void)self.setHookPriorityPre("cocos2d::CCApplication::updateMouseControl", Priority::VeryEarly);
    }

    void updateControllerKeys(CXBOXController* controller, int userIndex) {
        if (!controller) return;
        const int slot = controller->m_userIndex;
        XINPUT_STATE state{};
        const bool foreground = isForeground();
        const bool validSlot = slot >= 0 && slot < XUSER_MAX_COUNT;
        const bool connected = foreground && validSlot && XInputGetState(slot, &state) == ERROR_SUCCESS;
        if (!validSlot || !gates[slot].allow(foreground, connected, isNeutral(state.Gamepad))) {
            releaseController(controller, userIndex);
            m_obLeftThumb = CCPointZero;
            m_obRightThumb = CCPointZero;
            return;
        }
        CCApplication::updateControllerKeys(controller, userIndex);
    }

    void updateMouseControl() {
        if (!isForeground()) {
            m_obLeftThumb = CCPointZero;
            m_obRightThumb = CCPointZero;
            return;
        }
        CCApplication::updateMouseControl();
    }
};

namespace {
    // onGLFWWindowFocus is protected, this is just to get at it
    struct FocusViewAccess : CCEGLView {
        using CCEGLView::onGLFWWindowFocus;
    };

    void focusChanged(CCEGLView* view, GLFWwindow* window, int focused) {
        // when minimized GD may stop polling, so disarm right here
        if (!focused) {
            for (auto& gate : gates) gate.unfocus();
            if (auto app = CCApplication::get()) {
                app->m_obLeftThumb = CCPointZero;
                app->m_obRightThumb = CCPointZero;
            }
        }
        static_cast<FocusViewAccess*>(view)->onGLFWWindowFocus(window, focused);
    }
}

$on_mod(Loaded) {
    // not in geode's CCEGLView bindings, so $modify can't hook it
    auto address = addresser::getNonVirtual(&FocusViewAccess::onGLFWWindowFocus);
    auto hook = Mod::get()->hook(reinterpret_cast<void*>(address), &focusChanged,
        "Controller Focus window focus", tulip::hook::TulipConvention::Thiscall);
    if (!hook) log::error("Could not install focus hook: {}", hook.unwrapErr());
    log::info("Controller Focus loaded");
}
