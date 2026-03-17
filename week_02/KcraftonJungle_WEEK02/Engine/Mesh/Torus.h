#pragma once

constexpr int TORUS_VERTEX_COUNT = 32 * 16 * 6;

inline void GetTorusVertices(FVertexSimple* V)
{
    const float PI2 = 6.28318530717958f;
    const float R = 1.5f, r = 0.05f;
    const int   M = 32, N = 16;
    int Idx = 0;

    for (int i = 0; i < M; ++i)
        for (int j = 0; j < N; ++j)
        {
            float u[2] = { PI2 * i / M, PI2 * (i + 1) / M };
            float v[2] = { PI2 * j / N, PI2 * (j + 1) / N };

            auto Make = [&](int ui, int vi) -> FVertexSimple
                {
                    float cu = cosf(u[ui]), su = sinf(u[ui]);
                    float cv = cosf(v[vi]), sv = sinf(v[vi]);
                    return { (R + r * cv) * cu, (R + r * cv) * su, r * sv,
                             1,1,1,1,
                             cv * cu, cv * su, sv };
                };

            V[Idx++] = Make(0, 0); V[Idx++] = Make(1, 0); V[Idx++] = Make(0, 1);
            V[Idx++] = Make(0, 1); V[Idx++] = Make(1, 0); V[Idx++] = Make(1, 1);
        }
}