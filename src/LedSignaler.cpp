#include <Arduino.h>
#include "LedSignaler.h"
// define directions for LED fade
#define LED_FADE_UP   0
#define LED_FADE_DOWN 1
#define LED_FADE_WAIT 2
// constants for min and max led analog
//255 - 190 = 65 / 5 = 13
#define LED_FADE_STEPS 5
#define LED_MIN_FADE 190
#define LED_MAX_FADE 255
#define LED_FADEVALUE_TO_ANALOG(x) (LED_MIN_FADE + (LED_MAX_FADE - (x)))
#define DOT    50
#define DASH  -500
#define PAUSE -3000
// Blink presets
int16_t _BTYPE_BLINK1[] = { DOT, PAUSE};
int16_t _BTYPE_BLINK2[] = { DOT, DASH, DOT, PAUSE };
int16_t _BTYPE_BLINK3[] = { DOT, DASH, DOT, DASH, DOT,PAUSE };
int16_t _BTYPE_BLINK4[] = { DOT, DASH, DOT, DASH, DOT, DASH, DOT, PAUSE };
int16_t _BTYPE_BLINK5[] = { DOT, DASH, DOT, DASH, DOT, DASH, DOT, DASH, DOT, PAUSE };
int16_t _BTYPE_FAST[]   = { 5, -100, 5, -100, -500 };
int16_t _BTYPE_RAPID[]  = { 5, -350, 5, -350 };
int16_t _BTYPE_FADE1[]  = { 50,  -5000};
int16_t _BTYPE_FADE2[]  = { 100, -1000};
int16_t _BTYPE_FADE3[]  = { 200, -500};

BlinkPattern LedSignaler::_blinkPresets[] = {
    {},
    { 2,  _BTYPE_BLINK1 },
    { 4,  _BTYPE_BLINK2 },
    { 6,  _BTYPE_BLINK3 },
    { 8,  _BTYPE_BLINK4 },
    { 10, _BTYPE_BLINK5 },
    { 5,  _BTYPE_FAST },
    { 4,  _BTYPE_RAPID },
    { 2,  _BTYPE_FADE1 },
    { 2,  _BTYPE_FADE2 },
    { 2,  _BTYPE_FADE3 },
};

LedSignaler::LedSignaler(uint8_t pin, uint8_t active){
    _ledPin = pin;
    _state = active;
    _blinkPreset = BLINK_NONE;
    _repeats = -1;
    _blinkPresetNext = BLINK_NONE;
    _repeatsNext = -1;
    _enabled = true;
    _pause = false;
    _paused = true;
    _exitTask = false;
    _fadeDirection = LED_FADE_UP;
    _fadeValue = LED_MIN_FADE;
    _fadeInterval = 128;
    _fadeIncrement = 8;
    pinMode(_ledPin, OUTPUT);
    off();
}
LedSignaler::~LedSignaler() {
    #if defined(ESP32)
    endTask();
    #endif
    off();
 }

void LedSignaler::on()        { analogWrite(_ledPin, _state ? 1024 : 0); }
void LedSignaler::off()       { analogWrite(_ledPin, _state ? 0 : 1024); }
void LedSignaler::enable()    { _enabled = true; }
void LedSignaler::disable()   { _enabled = false; }
bool LedSignaler::isEnabled() { return _enabled; }
bool LedSignaler::isPaused()  { return _paused; }
void LedSignaler::resume()    { _paused = false; }
void LedSignaler::pause()     { _paused = true; }
void LedSignaler::turnOn()    { pause(); on(); }
void LedSignaler::turnOff()   { pause(); off(); }
uint8_t LedSignaler::getMode()  { return _blinkPreset; }
//Start blinking
void LedSignaler::blink(uint8_t preset, int8_t repeats){
    if(!isEnabled()) return;
    if(preset >= BLINK_END ) return;
    //Serial.printf("Blink start, preset: %i, repeats: %i, enabled: %i\n",preset, repeats, isEnabled());
    _blinkPreset = preset;
    _repeats = repeats;
    _paused = false;
    _pause = false;
    switch (preset)
    {
        case BTYPE_FADE1:
        case BTYPE_FADE2:
        case BTYPE_FADE3:
            {
            _patNdx = 0;
            _blinkIntv = _blinkPresets[_blinkPreset].pattern[0];

            // How fast to increment ?
            _fadeInterval = (int)(abs(_blinkIntv) / LED_FADE_STEPS);
            // How smooth to fade ?
            _fadeIncrement = (int)(2 * abs(LED_MAX_FADE - LED_MIN_FADE) / (_fadeInterval));
            _fadeValue = LED_MIN_FADE;
            _blinkIntv = _fadeInterval;
            //Serial.printf("Update start: %i, ndx: %i, repeat: %i, intv: %i, fintv: %i, inc: %i \n",_blinkPreset, _patNdx, _repeats, _blinkIntv,_fadeInterval,_fadeIncrement);
            analogWrite(_ledPin, LED_FADEVALUE_TO_ANALOG(_fadeValue));
            _patNdx = 1;
            _blinkTick = millis();
            break;
            }
        case BLINK_NONE:
            turnOff();
            break;

        default:
            _blinkIntv = _blinkPresets[_blinkPreset].pattern[0];
            //Serial.printf("Update start type: %i, ndx: 0, repeat: %i, intv: %i\n",_blinkPreset, _repeats, _blinkIntv);
            if (_blinkIntv < 0)
                off();
            else
                on();

            ++_patNdx;
            _blinkIntv = abs(_blinkIntv);
            _blinkTick = millis();
            break;
    }
    resume();
}
// Set next blink pattern
void LedSignaler::blinkPush(uint8_t preset, int8_t repeats /* -1 for infinitive */){
    if(preset >= BLINK_END ) return;
    //Serial.printf("blinkPush, type: %i, repeats: %i\n",type, repeats);
    _blinkPresetNext = preset;
    _repeatsNext = repeats;
    if( _blinkPreset == BLINK_NONE )
        blink(preset, repeats);
}
// Interupt blink task and play once
void LedSignaler::blinkInject(uint8_t preset, int8_t repeats /* -1 for infinitive */){
    if(preset >= BLINK_END ) return;
    //Serial.printf("blinkInject, type: %i, repeats: %i\n",type, repeats);
    //Store current settings
    _blinkPresetNext = _blinkPreset;
    _repeatsNext = _repeats;

    if( _blinkPreset == BLINK_NONE )
        blink(preset, repeats);
    else{
        _blinkPreset = preset;
        _repeats = repeats;
        _paused = false;
        _patNdx = 0;
    }
}

void LedSignaler:: update(){
    if(isPaused() || !isEnabled() || _blinkPreset == BLINK_NONE) return;
    if ((int16_t)(millis() - _blinkTick) < _blinkIntv) return;

    // Preset sequence end
    if( _patNdx >= _blinkPresets[_blinkPreset].size){
        //Serial.printf("Led preset end, repeat: %i, mextBlink: %i, nextRepeats: %i patNdx: %i\n",_repeats, _blinkPresetNext, _repeatsNext,_patNdx);
        _patNdx = 0;
        if(_blinkPreset >= BTYPE_FADE1){
            _blinkIntv = _blinkPresets[_blinkPreset].pattern[0];
            _patNdx = 1;
        }
        // Last repeat on infinitive and Next seq exists
        if( (_repeats == 1 || _repeats == -1) && _blinkPresetNext != BLINK_NONE){
            blink(_blinkPresetNext, _repeatsNext);
            // Disable next blink type
            _blinkPresetNext = BLINK_NONE;
            _repeatsNext = -1;
        }else if(_repeats > 1){ // Loop again
            --_repeats;
        }else if(_repeats != -1){ // Not infinitive
            _blinkPreset = BLINK_NONE;
        }
        return;
    }

    if(_blinkPreset >= BTYPE_FADE1){     // Fade
        // Start up direction
        if (_fadeDirection == LED_FADE_UP) {
            _fadeValue = _fadeValue + _fadeIncrement;
            if (_fadeValue >= LED_MAX_FADE) { // At max, limit and change direction
                _fadeValue = LED_MAX_FADE;
                _fadeDirection = LED_FADE_DOWN;
            }
            _blinkIntv = _fadeInterval;
            //Serial.printf("UP %i, %i\n", _blinkIntv, _fadeValue);
        }else if (_fadeDirection == LED_FADE_WAIT) {
            _fadeDirection = LED_FADE_UP;
            _blinkIntv = abs(_blinkPresets[_blinkPreset].pattern[_patNdx]);
            //Serial.printf("Wait %i, LED_FADE_UP\n", _blinkIntv);
            ++_patNdx;
        } else {
            //if we aren't going up, we're going down
            _fadeValue = _fadeValue - _fadeIncrement;
            if (_fadeValue <= LED_MIN_FADE) { // At min, limit and change direction
                _fadeValue = LED_MIN_FADE;
                _fadeDirection = LED_FADE_WAIT;
            }
            _blinkIntv = _fadeInterval;
            //Serial.printf("DN %i, %i\n", _blinkIntv,_fadeValue);
        }
        // Only need to update when it changes
        if (_fadeDirection != LED_FADE_WAIT){
            analogWrite(_ledPin, LED_FADEVALUE_TO_ANALOG(_fadeValue));
        }
    }else{ // Blink
        _blinkIntv = _blinkPresets[_blinkPreset].pattern[_patNdx];
        //Serial.printf("Update type: %i, ndx: %i, repeat: %i, intv: %i\n",_blinkPreset, _patNdx, _repeats, _blinkIntv);
        if (_blinkIntv < 0)
            off();
        else
            on();

        _blinkIntv = abs(_blinkIntv);
        ++_patNdx;
    }

    _blinkTick = millis();
}
#if defined(ESP32)
// Begin in thread mode
void LedSignaler::blinkTask(uint8_t preset, int8_t repeats){
    if(!isEnabled()) return;
    if(preset == _blinkPreset && _repeats == repeats) return;
    if(_task){
        pauseWait();
    }else{
        if (xTaskCreatePinnedToCore(&led_task,"led_task", 2*1024, this, 2, &_task, 1) != pdTRUE) {
            _task = nullptr;
            Serial.printf("startTask failed\n");
        }
        Serial.printf("Started led task.\n");
    }
    blink(preset, repeats);
}
// Wait for thread to pause
void LedSignaler::pauseWait() {
    _pause = true;
    while(!_paused){
        //Serial.print(".");
        usleep(100 * 1000);
    }
}
// Led thread task
void LedSignaler::led_task(void *arg) {
    LedSignaler *flasher = static_cast<LedSignaler *>(arg);
    while( !flasher->_exitTask ){
        if(flasher->_pause){
            Serial.printf("Pause Led task\n");
            flasher->_paused = true;
            flasher->off();
            // Sleep until resume
            while(flasher->_pause){
                flasher->_paused = true;
                Serial.printf("Led paused\n");
                usleep(100 * 1000);
            }
            flasher->_paused = false;
        }

        flasher->update();
        //usleep((flasher->_blinkIntv)* 1000);
        usleep(100 * 1000);
    }
    Serial.printf("ledFlasher exiting..\n");
    flasher->off();
    flasher->_paused = false;
    flasher->_exitTask = false;
    flasher->_task = NULL;
    vTaskDelete( NULL );
}
// Signal thread to exit
void LedSignaler::endTask(){
    _pause = false;
    _exitTask = true;
}
#endif