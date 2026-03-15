#pragma once

static FVertexSimple rect_vertices[] = {
    //    x,      y,      z,      r,      g,      b,      a,      nx,    ny,    nz
    { -0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f }, // 0: 좌측 상단
    {  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f }, // 1: 우측 상단
    {  0.5f, -0.5f,  0.0f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f }, // 2: 우측 하단
    { -0.5f, -0.5f,  0.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f }  // 3: 좌측 하단
};

// 인덱스 데이터는 동일
static UINT rect_indices[] = {
    0, 1, 2,
    0, 2, 3
};