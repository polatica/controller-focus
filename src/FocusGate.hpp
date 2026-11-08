#pragma once

// one gate per controller slot
struct FocusGate {
    bool armed = false;

    void unfocus() { armed = false; }

    bool allow(bool foreground, bool connected, bool neutral) {
        if (!foreground || !connected) {
            armed = false;
            return false;
        }
        // after losing focus (or a disconnect) wait for a neutral pad before re-enabling
        if (!armed && neutral) armed = true;
        return armed;
    }
};
