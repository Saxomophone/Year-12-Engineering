#pragma once
#include "eventScheduler.hpp"

extern bool areaObstructed;

void initUltrasonic(int trigger, int echo, int timeout, float successProportion);

bool check_area_obstructed();