#include <LedSignaler.h>

// Define the led pin if not defined
#ifndef LED_BUILTIN
#define LED_BUILTIN 22
#endif

bool ledState = false;
LedSignaler led( LED_BUILTIN, LOW);

#define BUFF_LEN 24
char cmdBuff[BUFF_LEN];
int cmdBuffPos=0;
// Serial data received
void serialEvent() {
	while (Serial.available()) {
		char inChar = (char) Serial.read();
		cmdBuff[cmdBuffPos++] = inChar;
		if (inChar == '\n' || inChar == '\r' || cmdBuffPos > BUFF_LEN - 2) {
			//Mark the array end without \n
			cmdBuff[cmdBuffPos - 1]='\0';
      Serial.printf("cmd: %s\n", cmdBuff);

      if(strcasecmp(cmdBuff, "on") == 0){
        Serial.printf("on\n");
        led.on();

      }else if(strcasecmp(cmdBuff, "off") == 0){
        Serial.printf("ledoff\n");
        led.off();

      }else if(strcasecmp(cmdBuff, "enable") == 0){
        Serial.printf("enable\n");
        led.enable();

      }else if(strcasecmp(cmdBuff, "disable") == 0){
        Serial.printf("disable\n");
        led.disable();

      }else if(strcasecmp(cmdBuff, "pause") == 0){
        Serial.printf("pause\n");
        led.pause();

      }else if(strcasecmp(cmdBuff, "resume") == 0){
        Serial.printf("resume\n");
        led.resume();
#if defined(ESP32)
      }else if(strcasecmp(cmdBuff, "endtask") == 0){
        Serial.printf("Ending task\n");
        led.endTask();
#endif
      }else if(strncasecmp(cmdBuff, "blink",5)   == 0 ||
               strncasecmp(cmdBuff, "task",4)    == 0 ||
               strncasecmp(cmdBuff, "push",4)    == 0 ||
               strncasecmp(cmdBuff, "inject",6)  == 0) {
        char cmd[25];
        char *t = strtok (cmdBuff, " ");
        if(t == NULL) break;
        strcpy(cmd, t);
        t = strtok(NULL, " ");
        if(t == NULL) break;
        uint8_t type = atoi(t);
        int loops = -1;
        t = strtok(NULL, ",");
        if(t != NULL) loops = atoi(t);

        if(strcasecmp("blink", cmd) == 0){
          Serial.printf("blink %i, %i\n",type, loops);
          if(loops==-1) led.blink(type);
          else led.blink(type, loops);

#if defined(ESP32)
        }else if(strcasecmp("task", cmd) == 0){
          Serial.printf("task blink %i, %i\n",type, loops);
          if(loops==-1) led.blinkTask(type);
          else led.blinkTask(type, loops);
#endif
        }else if(strcasecmp("push", cmd) == 0){
          Serial.printf("push %i, %i\n",type,loops);
          if(loops==-1) led.blinkPush(type);
          else led.blinkPush(type, loops);

        }else if(strcasecmp("inject", cmd) == 0){
          Serial.printf("inject %i, %i\n",type,loops);
          if(loops==-1) led.blinkInject(type);
          else led.blinkInject(type, loops);
        }

      }else if(strcasecmp(cmdBuff, "status") == 0){
        Serial.printf("blink: enabled:%i, paused:%i, mode:%i, next: %i ", led.isEnabled(), led.isPaused(), led.getMode(), led.getNextMode());

      }else{
        Serial.printf("Unknown cmd\n");
      }

			cmdBuffPos = 0;
			memset(cmdBuff, 0, sizeof(cmdBuff));
		}
	}
}

void setup() {
  Serial.begin(115200);
  Serial.print("\n\n\n\n");
  Serial.flush();
  Serial.printf("Starting ledSignaler v.%s\n", LS_CLASS_VERSION);
  #if defined(ESP8266)
  // Start blink task with update on loop
  led.blink(LedSignaler::BTYPE_BLINK1);
  #else
  // Start an indepentend thread for handling the led
  led.blinkTask(LedSignaler::BTYPE_FADE1);
  #endif
}
size_t tm = millis();
void loop() {
  if(Serial.available()) serialEvent();

#if defined(ESP8266)
  led.update();
#else
  //On esp32 can use thread (blinkTask) or loop update (blink) mode
  if(!led.hasTask()) led.update();
#endif

  // Monitor led mode
  if(millis() - tm > 3000){
    Serial.printf("Led mode: %i, next: %i\n", led.getMode(), led.getNextMode());
#if defined(ESP32)
    Serial.printf("Led task: %i\n", led.hasTask());
#endif
    tm = millis();
  }
}


