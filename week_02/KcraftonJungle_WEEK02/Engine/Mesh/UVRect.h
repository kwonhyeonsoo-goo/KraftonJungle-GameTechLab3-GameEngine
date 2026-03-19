#pragma once
static FVertexUV UVrect_vertices[] = {
    //   x      y      z      u     v      nx  ny  nz
    { -1000.0f, -1000.0f, 0.0f,   0.0f, 0.0f,    0,  0,  1 }, // 좌상 (origin)
    { 1000.0f, -1000.0f, 0.0f,   1.0f, 0.0f,    0,  0,  1 }, // 우상
    { 1000.0f, 1000.0f, 0.0f,   1.0f, 1.0f,    0,  0,  1 }, // 우하
    { -1000.0f, 1000.0f, 0.0f,   0.0f, 1.0f,    0,  0,  1 }  // 좌하
};

// 인덱스 데이터는 동일
static UINT UVrect_indices[] = {
    0,1,2,
    0,2,3,
    0,2,1,
    0,3,2
};