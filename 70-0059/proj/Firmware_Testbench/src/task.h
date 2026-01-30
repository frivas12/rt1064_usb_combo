#pragma once

#include <thread>
#include <chrono>

#define vTaskDelay(k) ((k == 0) ? std::this_thread::yield() : std::this_thread::sleep_for(std::chrono::milliseconds(k)))

// EOF
