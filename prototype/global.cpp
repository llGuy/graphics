#include <GLFW/glfw3.h>
#include "global.h"

window_data WindowData = {};

current_time TimeData = {};

void
current_time::Reset(void)
{
    Current = std::chrono::high_resolution_clock::now();
}

float
current_time::Elapsed(void)
{
    std::chrono::high_resolution_clock::time_point Now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> Seconds = Now - Current;
    float Count = Seconds.count();

    return Count;
}
