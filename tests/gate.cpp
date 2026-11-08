#include "../src/FocusGate.hpp"
#include <cstdlib>
#include <iostream>

static void check(bool ok, const char* msg) {
    if (!ok) {
        std::cerr << "FAIL: " << msg << '\n';
        std::exit(1);
    }
}

int main() {
    FocusGate gate;

    check(gate.allow(true, true, true), "neutral pad in foreground");
    check(gate.allow(true, true, false), "button press in foreground");

    check(!gate.allow(false, true, false), "button in background");
    check(!gate.allow(false, true, true), "neutral pad in background");
    check(!gate.allow(true, true, false), "button still held on return");
    check(gate.allow(true, true, true), "rearm on neutral");
    check(gate.allow(true, true, false), "new press after rearm");

    gate.unfocus(); // focus lost while minimized, nothing polled
    check(!gate.allow(true, true, false), "unfocus should disarm");

    check(!gate.allow(true, false, true), "disconnected");
    check(!gate.allow(true, true, false), "reconnect with a button held");
    check(gate.allow(true, true, true), "rearm after reconnect");

    FocusGate other;
    check(!other.allow(true, true, false), "second pad starts disarmed");
    check(gate.allow(true, true, false), "pads rearm independently");

    std::cout << "ok\n";
}
