#include <Arduino.h>

struct Phase {
  uint32_t duration;
  uint8_t value;

  Phase(){}

  Phase(uint32_t dur, uint8_t val) {
    duration = dur;
    value = val;
  }
};


class LedBlink {
  uint8_t pin;
  bool inverse;

  uint32_t timer;
  bool active;

  Phase *tasks; // [ [duration_msec, state], [duration_msec, state], ... ] => [ [100, LOW], [100, HIGH], ... ]
  uint16_t numTasks;
  uint32_t totalTime;

public:
  LedBlink(uint8_t _pin, Phase *_tasks, int size, bool _inverse = false) {
    pin = _pin;
    pinMode(pin, OUTPUT);
    inverse = _inverse;
    active = false;

    numTasks = size;
    tasks = new Phase[numTasks];
    totalTime = 0;
    
    for(int i = 0; i < numTasks; i ++) {
      Phase state = _tasks[i];
      if(inverse)
        state.value = state.value == HIGH? LOW : HIGH;
        
      tasks[i] = state;
      //Serial.printf("LedBlink [%d] = %d, %d\n", i, state.duration, state.value);
      totalTime += state.duration;
    }
  }

  ~LedBlink() {
    delete [] tasks;
  }

  void start() {
    active = true;
    timer = 0;
  }

  void stop() {
    active = false;
  }

  void update(uint32_t frameTime) {
    if(!active)
      return;

    timer = (timer + frameTime)%totalTime;

    unsigned long countTime = 0;
    for(int i = 0; i < numTasks; i ++) {
      Phase state = tasks[i];
      if( timer < countTime + state.duration ) {
          digitalWrite(pin, state.value);
          return;
      }
      countTime += state.duration;
    }
  }
};