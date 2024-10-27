# LedSignaler
Multi-platform library for controlling leds and generating signals on esp32/esp8266 devices. Can be used for various purposes such as indicating status, alerts, or notifications.


## Description
LedSignaler uses presets of dots (led On), dashes (led Off) and pause(led off) to generate signals using any onboard led. Presets can also inlcude fading leds in specific patterns.


## Features
* Using led presets
* Display running state
* Display error codes on a typical application
* Using threads on esp32 for smoother results


## How to use
```
// include led class
#include "LedSignaler.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 22
#endif

// Define the led class
LedSignaler led( LED_BUILTIN, LOW);
```


On ESP8266 call led.update on loop
```
// Start 2 blinks preset with infinitive loops
led.blink(2, -1);

void loop() {
  led.update();
}
```
On ESP32 start a blink task
```
// Start 2 blinks preset with infinitive loops
led.blinkTask(2, -1);
```

## LedSignaler preset call functions
```
// Wait current preset to end and play a 3 blinks preset.
// At end will return to original preset
led.blinkPush(3, -1);

// Interrupt current preset and play a 3 blinks preset 2 times.
// At end will return to original preset.
led.blinkInject(3, 2);
```
## Compile
Download library files and place them on ./libraries directory under ArduinoProjects
Then include the **LedSignaler.h** in your application and compile..

+ compile for arduino-esp3 or arduino-esp8266.

###### If you get compilation errors on arduino-esp32 you need to update your arduino-esp32 library in the IDE using Boards Manager

