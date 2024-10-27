#include <LedSignaler.h>
size_t tm = millis();
uint8_t ledPreset = 1;

#ifndef LED_BUILTIN
#define LED_BUILTIN 22
#endif
LedSignaler led( LED_BUILTIN, LOW);

void setup() {
  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();
  Serial.printf("Starting ledSignaler v.%s\n", LS_CLASS_VERSION);
  #if defined(ESP8266)
    led.blink(LedSignaler::BTYPE_BLINK1);
  #else
    led.blinkTask(LedSignaler::BTYPE_BLINK1);
  #endif
}

void loop() {
  if(millis() - tm > 10000){
    ++ledPreset;
    if(ledPreset > LedSignaler::BLINK_END)
      ledPreset = 1;
    led.blinkPush(ledPreset);
    tm = millis();
    Serial.printf("Changing preset: %i\n", ledPreset);
  }
  #if defined(ESP8266)
  led.update();
  #endif
}


