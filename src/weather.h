#ifndef _WEATHER_H
#define _WEATHER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "state/app_state.h"

bool get_weather(void);

#endif
