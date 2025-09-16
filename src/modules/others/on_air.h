#ifndef __ON_AIR_SIGN_H__
#define __ON_AIR_SIGN_H__

#include <Arduino.h>

enum class OnAirState : uint8_t {
    Off = 0,
    Standby = 1,
    Live = 2,
};

void showOnAirSign();

void setOnAirState(OnAirState state);
OnAirState getOnAirState();

bool setOnAirState(const String &stateName);
const char *onAirStateToString(OnAirState state);
String onAirStateDisplayLabel(OnAirState state);

#endif
