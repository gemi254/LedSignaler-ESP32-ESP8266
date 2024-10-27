#ifndef LedSignaler_h
#define LedSignaler_h
#include <Arduino.h>

#define LS_CLASS_VERSION "1.0.0"          // LedSignaler class version

typedef struct tagBlinkPattern{
  int16_t 	size;
  int16_t  *pattern;
} BlinkPattern;

class LedSignaler {
    public:
        LedSignaler(uint8_t pin, uint8_t active);
        ~LedSignaler();
    public:
        void on();
        void off();
        void enable();
        void disable();
        bool isEnabled();
        bool isPaused();
        void resume();
        void pause();
        void turnOn();                                        // Pause task and turn led on
        void turnOff();                                       // Pause task and turn led off
        uint8_t getMode();                                    // Get active preset
        void blink(uint8_t preset, int8_t repeats = -1);      // Start blinking
        void blinkPush(uint8_t preset, int8_t repeats = -1);  // Set next blink pattern (repeats = -1 for infinitive)
        void blinkInject(uint8_t preset, int8_t repeats = 1); // Interupt blink task and play once
        void update();                                        // Must called from loop when not in thread mode
#if defined(ESP32)
    public:
    // Begin in thread mode
        void blinkTask(uint8_t preset, int8_t repeats = -1);
    private:
        void pauseWait();
        static void led_task(void *arg);
        void endTask();
#endif

    private:
        uint8_t				_ledPin;
        uint8_t				_state;

        int8_t				_blinkPreset;
        int8_t              _repeats;
        int8_t				_blinkPresetNext;
        int8_t              _repeatsNext;

        int8_t				_patNdx;
        int16_t				_blinkIntv;

        uint32_t			_blinkTick;
        bool                _enabled;

    private:
        bool                _pause;
        bool                _paused;
        bool                _exitTask;
#if defined(ESP32)
        TaskHandle_t        _task = nullptr;
#endif

    private:
        byte                _fadeDirection;
        int                 _fadeValue;
        int                 _fadeInterval;          // How fast to increment?
        int                 _fadeIncrement;         // How smooth to fade?

    public:
        enum BLINK_TYPE{
            BLINK_NONE = 0,	 // ___________
            BTYPE_BLINK1,	 // -___
            BTYPE_BLINK2,	 // --_--___
            BTYPE_BLINK3,    // --_--_--___
            BTYPE_BLINK4,    // --_--_--_--___
            BTYPE_BLINK5,    // --_--_--_--_--___
            BTYPE_FAST,      // -__-__
            BTYPE_RAPID,     // -_-_
            BTYPE_FADE1,
            BTYPE_FADE2,
            BTYPE_FADE3,
            BLINK_END       // ____________
        };
        static BlinkPattern _blinkPresets[];
};
#endif