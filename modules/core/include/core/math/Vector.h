#pragma once

namespace Phoenix
{
    struct Vector2
    {
        Vector2(float _x, float _y) : x(_x), y(_y) {};
        float x{}, y{};

        static Vector2 Zero() { return {0.0f, 0.0f}; }
    };
    struct Vector2Int
    {
        Vector2Int(int _x, int _y) : x(_x), y(_y) {};
        int x{}, y{};

        static Vector2Int Zero() { return {0, 0}; }
    };

    struct Vector3
    {
        Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {};
        float x{}, y{}, z{};

        static Vector3 Zero() { return {0.0f, 0.0f, 0.0f}; }
    };
    struct Vector3Int
    {
        Vector3Int(int _x, int _y, int _z) : x(_x), y(_y), z(_z) {};
        int x, y, z;

        static Vector3Int Zero() { return {0, 0, 0}; }
    };
}