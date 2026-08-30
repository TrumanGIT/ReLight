#pragma once
#include "SKSEMenuFramework.h"
#include <chrono>

namespace UI {

    // Based on Log Watcher state machine.
    enum class buttonState {
        Idle,
        Working,
        Success,
        Fail
    };

    class buttonTicker {

        buttonState state{ buttonState::Idle };
        std::chrono::steady_clock::time_point until{};

    public:

        void set(const buttonState& s, const float& seconds = 2.0f) {
            state = s;
            if (s == buttonState::Success || s == buttonState::Fail) {
                until = std::chrono::steady_clock::now() + std::chrono::milliseconds((int)(seconds * 1000.0f));
            }
        }

        void tick() {
            if ((state == buttonState::Success || state == buttonState::Fail) && std::chrono::steady_clock::now() >= until) {
                state = buttonState::Idle;
            }
        }

        buttonState getState() const {
            return state;
		}
    };

    class RefreshTicker
    {
        std::chrono::seconds interval;
        std::chrono::steady_clock::time_point last{};
        bool started{ false };

    public:

        RefreshTicker(const std::chrono::seconds& interval)
            : interval(interval)
        {}

        bool shouldTick() {
            auto now = std::chrono::steady_clock::now();
            if (!started) {
                started = true;
                last = now;
				return true;  // first refresh is done immediately
            }

            if (now - last >= interval) {
                last = now;
                return true;
            }

            return false;
        }

        void reset() {
            started = false;
        }

        void setInterval(const std::chrono::seconds& interval) {
           this->interval = interval;
        }
    };

    inline void renderDone(buttonTicker& t, const float& x, const float& y) {

        t.tick();

        if (t.getState() == buttonState::Idle) return;

        ImGuiMCP::ImVec2 oldPos = ImGuiMCP::GetCursorPos();
        ImGuiMCP::SetCursorPos({ x, y });

        FontAwesome::PushSolid();
        if (t.getState() == buttonState::Working) {
            // We can show some pregress here if we want.
        }
        else if (t.getState() == buttonState::Success) {
            ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text, ImGuiMCP::ImVec4{ 0.2f, 1.0f, 0.2f, 1.0f }); // green
            auto s = FontAwesome::UnicodeToUtf8(0xf00c);
            ImGuiMCP::Text("%s", s.c_str());
            ImGuiMCP::PopStyleColor();
        }
        else { // Fail
            ImGuiMCP::PushStyleColor(ImGuiMCP::ImGuiCol_Text, ImGuiMCP::ImVec4{ 1.0f, 0.2f, 0.2f, 1.0f }); // red
            auto s = FontAwesome::UnicodeToUtf8(0xf00d);
            ImGuiMCP::Text("%s", s.c_str());
            ImGuiMCP::PopStyleColor();
        }
        FontAwesome::Pop();

        ImGuiMCP::SetCursorPos(oldPos);
    }


};