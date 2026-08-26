#pragma once

#include "eventScheduler.hpp"


bool x_limit_button();

bool y_limit_button();

void initLimits(EventScheduler* scheduler);

extern bool xLimitReached;
extern bool yLimitReached;