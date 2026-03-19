#pragma once

static FVertexSimple triangle_vertices[] =
{
    //   x      y     z     r     g     b     a      nx      ny      nz
    {  0.0f,  1.0f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f,  0.0f,   1.0f,   -0.1f },   // top
        { -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f, 1.0f, -0.832f, -0.555f, -0.1f },    // left
    {  1.0f, -1.0f, 0.0f,  0.0f, 1.0f, 0.0f, 1.0f,  0.832f, -0.555f, -0.1f }   // right

};

