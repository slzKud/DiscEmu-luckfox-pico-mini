#include <atomic>
#include <thread>
#include <unistd.h>
#include <chrono>  // 添加时间库

#include "input.h"

extern "C" {
#include <libu8g2arm/gpio.h>
}

#define GPIO_NUMBER(b,g,x) (b*32+g*8+x)
#define GPIO_GROUP_A 0
#define GPIO_GROUP_B 1
#define GPIO_GROUP_C 2
#define GPIO_GROUP_D 3
//#define DEADFATTY_KEYPAD_INPUT
const int ROT_S1 = GPIO_NUMBER(1,GPIO_GROUP_C,0);
const int ROT_S2 = GPIO_NUMBER(1,GPIO_GROUP_C,1);
const int ROT_KEY = GPIO_NUMBER(1,GPIO_GROUP_C,2);

std::thread input_thread;
std::atomic<InputValue> input_value = InputValue::Unknown;
std::atomic_bool thread_stop = false;

// 长按时间阈值（毫秒）
constexpr std::chrono::milliseconds LONG_PRESS_THRESHOLD(500);

void *input_thread_func() {
    int ss1 = 0, ss2 = 0, skey = 0;
    int s1 = 0, s2 = 0, key = 0;
    
    // 按键状态跟踪变量
    bool key_active = false;
    std::chrono::steady_clock::time_point press_start_time;

    while (!thread_stop) {
        #ifdef DEADFATTY_KEYPAD_INPUT
        s1 = getGPIOValue(ROT_S2);
        s2 = getGPIOValue(ROT_S1);
        key = getGPIOValue(ROT_KEY);
        #else
        s1 = getGPIOValue(ROT_S1);
        s2 = getGPIOValue(ROT_S2);
        key = getGPIOValue(ROT_KEY);
        #endif
        // 按键检测逻辑
        if (skey != key) {
            if (!key) { // 按键按下
                key_active = true;
                press_start_time = std::chrono::steady_clock::now();
            } else {    // 按键释放
                if (key_active) {
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - press_start_time
                    );
                    
                    if (duration < LONG_PRESS_THRESHOLD) {
                        input_value.store(InputValue::Enter);
                    }
                    key_active = false;
                }
            }
        }
        
        // 长按检测
        if (key_active) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - press_start_time
            );
            
            if (duration >= LONG_PRESS_THRESHOLD) {
                input_value.store(InputValue::LongEnter);
                key_active = false; // 防止重复触发
            }
        }

#ifndef KEYPAD_INPUT
        // 旋转编码器处理
        if (ss1 != s1 && !s1) {
            if (s1 != s2) {
                input_value.store(InputValue::Up);
            } else {
                input_value.store(InputValue::Down);
            }
        }
        ss1 = s1;
        skey = key; // 更新按键状态
        usleep(5000);
#else
        // 独立按键处理
        if (ss1 != s1 && !s1) {
            input_value.store(InputValue::Up);
        }
        if (ss2 != s2 && !s2) {
            input_value.store(InputValue::Down);
        }
        ss1 = s1;
        ss2 = s2;
        skey = key; // 更新按键状态
        usleep(5000);
#endif
    }
    return nullptr;
}


InputValue input_get() { return input_value.exchange(InputValue::Unknown); }

void input_wait_enter() {
  InputValue i = Unknown;
  while (i != Back && i != Enter && i != LongEnter) {
    sleep(0);
    i = input_get();
  }
}

int input_init() {
  exportGPIOPin(ROT_S1);
  exportGPIOPin(ROT_S2);
  exportGPIOPin(ROT_KEY);
  setGPIODirection(ROT_S1, GPIO_IN);
  setGPIODirection(ROT_S2, GPIO_IN);
  setGPIODirection(ROT_KEY, GPIO_IN);

  input_thread = std::thread(input_thread_func);
  return 1;
}

void input_stop() {
  thread_stop.store(true);
  input_thread.join();
}