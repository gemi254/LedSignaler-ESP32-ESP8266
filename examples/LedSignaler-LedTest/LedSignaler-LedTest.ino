#include <LedSignaler.h>

bool ledState = false;
#ifndef LED_BUILTIN
#define LED_BUILTIN 22
#endif
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
      //Serial.printf("cmd: %s\n", cmdBuff);
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
      }else if(strncasecmp(cmdBuff, "blink",5)  == 0 ||
               strncasecmp(cmdBuff, "push",4)   == 0  ||
               strncasecmp(cmdBuff, "inject",6) == 0) {
        char cmd[10];
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
        Serial.printf("blink: enabled:%i, paused:%i, mode:%i\n",led.isEnabled(), led.isPaused(), led.getMode());
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
  led.blink(LedSignaler::BTYPE_BLINK1);
}

void loop() {
  if(Serial.available()) serialEvent();
  led.update();
}


