#include "on_air.h"

#include "core/display.h"
#include "core/utils.h"
#include <globals.h>

namespace {
    volatile OnAirState g_onAirState = OnAirState::Off;

    struct OnAirStateStyle {
        OnAirState state;
        const char *label;
        const char *subtitle;
        uint16_t background;
        uint16_t text;
        uint16_t accent;
    };

    constexpr uint16_t makeColor(uint8_t r, uint8_t g, uint8_t b) {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }

    const OnAirStateStyle &styleForState(OnAirState state) {
        static OnAirStateStyle styles[] = {
            {OnAirState::Off, "OFF AIR", "Studio is idle", 0, 0, 0},
            {OnAirState::Standby, "STANDBY", "Stand by for cues", 0, 0, 0},
            {OnAirState::Live, "ON AIR", "Quiet please", 0, 0, 0},
        };

        static bool accentInitialised = false;
        styles[0].background = bruceConfig.bgColor;
        styles[0].text = bruceConfig.priColor;
        styles[0].accent = bruceConfig.secColor;

        if (!accentInitialised) {
            styles[1].background = makeColor(245, 158, 11);
            styles[1].text = makeColor(26, 32, 44);
            styles[1].accent = makeColor(217, 119, 6);

            styles[2].background = makeColor(220, 38, 38);
            styles[2].text = TFT_WHITE;
            styles[2].accent = makeColor(127, 29, 29);
            accentInitialised = true;
        }

        for (const auto &style : styles) {
            if (style.state == state) return style;
        }
        return styles[0];
    }

    OnAirState nextState(OnAirState state) {
        switch (state) {
        case OnAirState::Off:
            return OnAirState::Standby;
        case OnAirState::Standby:
            return OnAirState::Live;
        case OnAirState::Live:
        default:
            return OnAirState::Off;
        }
    }

    OnAirState previousState(OnAirState state) {
        switch (state) {
        case OnAirState::Off:
            return OnAirState::Live;
        case OnAirState::Standby:
            return OnAirState::Off;
        case OnAirState::Live:
        default:
            return OnAirState::Standby;
        }
    }

    void drawSubtitle(const OnAirStateStyle &style, int16_t y) {
        tft.setTextSize(FM);
        tft.setTextColor(style.text, style.background);
        tft.drawCentreString(style.subtitle, tftWidth / 2, y, 1);
    }

    void drawFooter(const OnAirStateStyle &style) {
        tft.setTextSize(FP);
        tft.setTextColor(style.text, style.background);
        tft.drawCentreString("Next/Sel change • Prev back • Esc exit", tftWidth / 2, tftHeight - 18, 1);
        tft.drawCentreString("State updates instantly from the WebUI", tftWidth / 2, tftHeight - 30, 1);
    }

    void drawTitle(const OnAirStateStyle &style) {
        tft.setTextSize(FM);
        tft.setTextColor(style.text, style.background);
        tft.drawCentreString("STUDIO STATUS", tftWidth / 2, 14, 1);
    }

    void drawAccentFrame(const OnAirStateStyle &style) {
        int margin = 6;
        int radius = 8;
        tft.drawRoundRect(margin, margin, tftWidth - 2 * margin, tftHeight - 2 * margin, radius, style.accent);
    }

    void drawState(const OnAirStateStyle &style) {
        tft.fillScreen(style.background);
        drawAccentFrame(style);
        drawTitle(style);

        const String label(style.label);
        const int availableWidth = tftWidth - 20;
        uint8_t fontSize = 8;
        while (fontSize > 2) {
            if (label.length() * 6 * fontSize <= availableWidth) break;
            --fontSize;
        }

        tft.setTextSize(fontSize);
        tft.setTextColor(style.text, style.background);
        int16_t mainTextY = (tftHeight / 2) - (fontSize * 4);
        if (mainTextY < 32) mainTextY = 32;
        tft.drawCentreString(label, tftWidth / 2, mainTextY, 1);

        drawSubtitle(style, mainTextY + fontSize * 8 + 6);
        drawFooter(style);
    }

    OnAirState parseStateName(const String &name, bool &ok) {
        String normalized = name;
        normalized.toLowerCase();
        normalized.trim();

        if (normalized == "on" || normalized == "live" || normalized == "onair" || normalized == "on_air") {
            ok = true;
            return OnAirState::Live;
        }
        if (normalized == "standby" || normalized == "ready") {
            ok = true;
            return OnAirState::Standby;
        }
        if (normalized == "off" || normalized == "offair" || normalized == "idle") {
            ok = true;
            return OnAirState::Off;
        }

        ok = false;
        return g_onAirState;
    }
}

OnAirState getOnAirState() { return g_onAirState; }

void setOnAirState(OnAirState state) { g_onAirState = state; }

bool setOnAirState(const String &stateName) {
    bool ok = false;
    OnAirState state = parseStateName(stateName, ok);
    if (!ok) return false;
    setOnAirState(state);
    return true;
}

const char *onAirStateToString(OnAirState state) {
    switch (state) {
    case OnAirState::Standby:
        return "standby";
    case OnAirState::Live:
        return "live";
    case OnAirState::Off:
    default:
        return "off";
    }
}

String onAirStateDisplayLabel(OnAirState state) {
    return styleForState(state).label;
}

void showOnAirSign() {
    wakeUpScreen();
    returnToMenu = false;
    OnAirState lastState = getOnAirState();
    drawState(styleForState(lastState));

    while (!returnToMenu) {
        if (check(EscPress)) break;

        OnAirState state = getOnAirState();
        bool shouldRedraw = state != lastState;

        if (check(NextPress) || check(SelPress)) {
            state = nextState(state);
            setOnAirState(state);
            shouldRedraw = true;
        }

        if (check(PrevPress)) {
            state = previousState(state);
            setOnAirState(state);
            shouldRedraw = true;
        }

        if (shouldRedraw) {
            drawState(styleForState(state));
            lastState = state;
        }

        delay(50);
    }

    backToMenu();
}
