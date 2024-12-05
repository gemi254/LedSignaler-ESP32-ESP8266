#include <Arduino.h>
#include "LedSignaler.h"
// define directions for LED fade
#define LED_FADE_UP   0
#define LED_FADE_DOWN 1
#define LED_FADE_WAIT 2
// constants for min and max led analog
//255 - 190 = 65 / 5 = 13
//255 - 180 = 75 / 5 = 15
#define LED_FADE_STEPS 5
#define LED_MIN_FADE 180
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
int16_t _BTYPE_FADE1[]  = { 100, -4000};
int16_t _BTYPE_FADE2[]  = { 100, -800 };
int16_t _BTYPE_FADE3[]  = { 80,  -300 };

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
    //_pause = false;
    _paused = true;
    _exitTask = false;
    _fadeDirection = LED_FADE_UP;
    _fadeValue = LED_MIN_FADE;
    _lastFadeValue = -1;
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

void LedSignaler::fadeLed(uint8_t pin, int value){
    if (value != _lastFadeValue) {
        analogWrite(pin, value);
        _lastFadeValue = value;
    }
}

void LedSignaler::on()        { fadeLed(_ledPin, _state ? 1024 : 0); }
void LedSignaler::off()       { fadeLed(_ledPin, _state ? 0 : 1024); }
void LedSignaler::enable()    { _enabled = true; }
void LedSignaler::disable()   { _enabled = false; }
bool LedSignaler::isEnabled() { return _enabled; }
bool LedSignaler::isPaused()  { return _paused; }
void LedSignaler::resume()    { _paused = false; }
void LedSignaler::pause()     { _paused = true; }
void LedSignaler::turnOn()    { pause(); on(); }
void LedSignaler::turnOff()   { pause(); off(); }
uint8_t LedSignaler::getMode()  { return _blinkPreset; }
uint8_t LedSignaler::getNextMode()  { return _blinkPresetNext; }

// Blinking begin sequence
void LedSignaler::blink(uint8_t preset, int8_t repeats){
    if(!isEnabled()) return;
    if(preset >= BLINK_END ) return;
    //Serial.printf("Blink start, preset: %i, repeats: %i, enabled: %i\n",preset, repeats, isEnabled());
    _blinkPreset = preset;
    _repeats = repeats;
    _paused = false;
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
            fadeLed(_ledPin, LED_FADEVALUE_TO_ANALOG(_fadeValue));
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
    #ifdef DEBUG
    Serial.printf("blinkPush, type: %i, repeats: %i\n",preset, repeats);
    #endif
    _blinkPresetNext = preset;
    _repeatsNext = repeats;
    // On led disabled start a new blink
    if( _blinkPreset == BLINK_NONE ){
        #if defined(ESP32)
        if(_task) blinkTask(preset, repeats);
        else blink(preset, repeats);
        #else
        blink(preset, repeats);
        #endif
    }
}

// Interupt blink task and play once
void LedSignaler::blinkInject(uint8_t preset, int8_t repeats /* -1 for infinitive */){
    if(preset >= BLINK_END ) return;
    if(preset == _blinkPreset && _repeats == repeats) return;
    #ifdef DEBUG
    Serial.printf("blinkInject, type: %i, repeats: %i\n",preset, repeats);
    #endif

    //Store current settings
    _blinkPresetNext = _blinkPreset;
    _repeatsNext = _repeats;

    if( _blinkPreset == BLINK_NONE ){
        #if defined(ESP32)
        if(_task) blinkTask(preset, repeats);
        else blink(preset, repeats);
        #else
        blink(preset, repeats);
        #endif
    }else{
        _blinkPreset = preset;
        _repeats = repeats;
        _paused = false;
        _patNdx = 0;
    }
}

// Called from thread or from app loop
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
            fadeLed(_ledPin, LED_FADEVALUE_TO_ANALOG(_fadeValue));
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
    if(!isEnabled() || (preset == _blinkPreset && _repeats == repeats) ) return;

    // Create a binary semaphore
    if(_xSemaphore == nullptr) _xSemaphore = xSemaphoreCreateBinary();
    // Block task
    xSemaphoreTake(_xSemaphore, 0);

    // Task not exists
    if (_task == nullptr){
        if (xTaskCreatePinnedToCore(&led_task,"led_task", 6 * 512, this, 2, &_task, 1) != pdTRUE) {
            _task = nullptr;
            Serial.printf("Start led task error.\n");
        }
    }

    #ifdef DEBUG
    Serial.printf("blinkTask, type: %i, repeats: %i\n",preset, repeats);
    #endif
    blink(preset, repeats);
    xSemaphoreGive(_xSemaphore);
}

// Led thread task
void LedSignaler::led_task(void *arg) {
    LedSignaler *flasher = static_cast<LedSignaler *>(arg);
    while( !flasher->_exitTask ){
        // Wait (block) until the semaphore is available
        xSemaphoreTake(flasher->_xSemaphore, portMAX_DELAY);
        flasher->update();
        xSemaphoreGive(flasher->_xSemaphore);
        // 50 ms delay, flasher->update will handle timing
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }

    #ifdef DEBUG
    Serial.printf("ledFlasher exiting..\n");
    #endif
    // Turn off the LED or perform cleanup before deleting the task
    flasher->off();

    // Release the semaphore before deleting the task
    xSemaphoreGive(flasher->_xSemaphore);

    flasher->_paused = false;
    flasher->_exitTask = false;
    flasher->_task = NULL;

    // Delete the task itself
    vTaskDelete(NULL);
}

// Signal thread to exit
void LedSignaler::endTask(){
    if(_task == nullptr) return;
    _exitTask = true;
    xSemaphoreGive(_xSemaphore);
    while(_task ) Serial.printf(".");

    if(_xSemaphore != nullptr){
        vSemaphoreDelete(_xSemaphore);
        _xSemaphore = nullptr;
    }
}
#endif