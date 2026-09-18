#pragma once

#include <chrono>

class Time
{
public:
    using clock = std::chrono::steady_clock;

    static float DeltaTime()
    {
        return deltaTime;
    }

    static void Update()
    {
        const clock::time_point now = clock::now();
        deltaTime = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
    }

private:
    inline static float deltaTime = 0.0f;
    inline static clock::time_point lastTime = clock::now();
};
