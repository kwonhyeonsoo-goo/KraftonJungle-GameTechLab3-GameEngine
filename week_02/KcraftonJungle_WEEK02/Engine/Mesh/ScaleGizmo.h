#pragma once
#include <cmath>
#include "FVertexSimple.h"

#define SCALE_GIZMO_SN 8
constexpr int SCALE_GIZMO_VERTEX_COUNT = SCALE_GIZMO_SN * 12 + 36;

static const float SG_SR = 0.03f;
static const float SG_SH = 0.85f;
static const float SG_HH = 0.15f;

static void SG_EmitVert(FVertexSimple* V, int& Idx,
    float x, float y, float z,
    float nx, float ny, float nz)
{
    V[Idx++] = { x, y, z, 1, 1, 1, 1, nx, ny, nz };
}

static void SG_EmitQuad(FVertexSimple* V, int& Idx,
    float ax, float ay, float az,
    float bx, float by, float bz,
    float cx, float cy, float cz,
    float dx, float dy, float dz,
    float nx, float ny, float nz)
{
    SG_EmitVert(V, Idx, ax, ay, az, nx, ny, nz);
    SG_EmitVert(V, Idx, bx, by, bz, nx, ny, nz);
    SG_EmitVert(V, Idx, cx, cy, cz, nx, ny, nz);
    SG_EmitVert(V, Idx, ax, ay, az, nx, ny, nz);
    SG_EmitVert(V, Idx, cx, cy, cz, nx, ny, nz);
    SG_EmitVert(V, Idx, dx, dy, dz, nx, ny, nz);
}

static void SG_EmitShaft(FVertexSimple* V, int& Idx)
{
    const float PI2 = 6.28318530718f;
    for (int i = 0; i < SCALE_GIZMO_SN; ++i)
    {
        float a0 = PI2 * i / SCALE_GIZMO_SN, c0 = cosf(a0), s0 = sinf(a0);
        float a1 = PI2 * (i + 1) / SCALE_GIZMO_SN, c1 = cosf(a1), s1 = sinf(a1);

        SG_EmitVert(V, Idx, SG_SR * c0, 0, SG_SR * s0, c0, 0, s0);
        SG_EmitVert(V, Idx, SG_SR * c1, 0, SG_SR * s1, c1, 0, s1);
        SG_EmitVert(V, Idx, SG_SR * c1, SG_SH, SG_SR * s1, c1, 0, s1);
        SG_EmitVert(V, Idx, SG_SR * c0, 0, SG_SR * s0, c0, 0, s0);
        SG_EmitVert(V, Idx, SG_SR * c1, SG_SH, SG_SR * s1, c1, 0, s1);
        SG_EmitVert(V, Idx, SG_SR * c0, SG_SH, SG_SR * s0, c0, 0, s0);

        SG_EmitVert(V, Idx, 0, 0, 0, 0, -1, 0);
        SG_EmitVert(V, Idx, SG_SR * c0, 0, SG_SR * s0, 0, -1, 0);
        SG_EmitVert(V, Idx, SG_SR * c1, 0, SG_SR * s1, 0, -1, 0);

        SG_EmitVert(V, Idx, 0, SG_SH, 0, 0, 1, 0);
        SG_EmitVert(V, Idx, SG_SR * c1, SG_SH, SG_SR * s1, 0, 1, 0);
        SG_EmitVert(V, Idx, SG_SR * c0, SG_SH, SG_SR * s0, 0, 1, 0);
    }
}

static void SG_EmitHead(FVertexSimple* V, int& Idx)
{
    const float y0 = SG_SH, y1 = SG_SH + SG_HH * 2.f;
    const float x0 = -SG_HH, x1 = SG_HH;
    const float z0 = -SG_HH, z1 = SG_HH;

    SG_EmitQuad(V, Idx, x0, y0, z1, x1, y0, z1, x1, y1, z1, x0, y1, z1, 0, 0, 1);
    SG_EmitQuad(V, Idx, x1, y0, z0, x0, y0, z0, x0, y1, z0, x1, y1, z0, 0, 0, -1);
    SG_EmitQuad(V, Idx, x1, y0, z1, x1, y0, z0, x1, y1, z0, x1, y1, z1, 1, 0, 0);
    SG_EmitQuad(V, Idx, x0, y0, z0, x0, y0, z1, x0, y1, z1, x0, y1, z0, -1, 0, 0);
    SG_EmitQuad(V, Idx, x0, y1, z1, x1, y1, z1, x1, y1, z0, x0, y1, z0, 0, 1, 0);
    SG_EmitQuad(V, Idx, x0, y0, z0, x1, y0, z0, x1, y0, z1, x0, y0, z1, 0, -1, 0);
}

inline void GetScaleGizmoVertices(FVertexSimple* V)
{
    int Idx = 0;
    SG_EmitShaft(V, Idx);
    SG_EmitHead(V, Idx);
}