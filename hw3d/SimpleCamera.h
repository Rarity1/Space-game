//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once
#include <d3d12.h>
#include <DirectXMath.h>

using namespace DirectX;

class SimpleCamera
{
public:
    SimpleCamera();

    void Init(XMVECTOR position);
    void Update(XMFLOAT4* pos, float* yaw, float* pitch, float* roll, XMFLOAT4* rupdirection);
    XMMATRIX GetViewMatrix();
    XMMATRIX GetProjectionMatrix(float fov, float aspectRatio, float nearPlane = 1.0f, float farPlane = 1000.0f);
    void SetMoveSpeed(float unitsPerSecond);
    void SetTurnSpeed(float radiansPerSecond);

private:
    void Reset();


    XMVECTOR m_position;
    float m_yaw;                // Relative to the +z axis.
    float m_pitch;                // Relative to the xz plane.
    float m_roll;
    XMVECTOR m_lookDirection;
    XMVECTOR m_upDirection;
    float m_moveSpeed;            // Speed at which the camera moves, in units per second.
    float m_turnSpeed;            // Speed at which the camera turns, in radians per second.

};
