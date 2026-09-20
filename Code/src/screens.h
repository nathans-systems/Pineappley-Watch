#pragma once
#include "watch.h"
#include "buttons.h"

void screenEnter(AppState s);
void screenTick (AppState s, BtnEvent ev);

// True while the stopwatch is counting, wherever you are in the UI. main.cpp
// uses this to hold off standby and deep sleep.
bool stopwatchActive();
