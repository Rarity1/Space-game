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

#include "SimpleCamera.h"

SimpleCamera::SimpleCamera() :
    m_initialPosition(XMVectorSet(0, -6, 0, 0)),
    m_position(m_initialPosition),
    m_yaw(0.0f),
    m_pitch(0.0f),
    m_lookDirection(XMVectorSet(0, 1, 0, 0)),
    m_upDirection(XMVectorSet(0, 0, 1, 0)),
    m_moveSpeed(20.0f),
    m_turnSpeed(XM_PIDIV2),
    m_keysPressed{}
{
}

void SimpleCamera::Init(XMVECTOR position)
{
    m_initialPosition = position;
    Reset();
}

void SimpleCamera::SetMoveSpeed(float unitsPerSecond)
{
    m_moveSpeed = unitsPerSecond;
}

void SimpleCamera::SetTurnSpeed(float radiansPerSecond)
{
    m_turnSpeed = radiansPerSecond;
}

void SimpleCamera::Reset()
{
    m_position = m_initialPosition;
    m_yaw = 0.826;
    m_pitch = 0.0f;
    m_lookDirection = XMVectorSet(0, 1, 0, 0);
}

void SimpleCamera::Update(float elapsedSeconds)
{
    // Calculate the move vector in camera space.
    XMFLOAT3 move(0, 0, 0);

    if (m_keysPressed.a)
        move.x += 1.f;
    if (m_keysPressed.d)
        move.x -= 1.f;
    if (m_keysPressed.w)
        move.y += 1.f;
    if (m_keysPressed.s)
        move.y -= 1.f;



    float moveInterval = m_moveSpeed * elapsedSeconds;
    float rotateInterval = m_turnSpeed * elapsedSeconds;

    if (m_keysPressed.left)
        m_yaw += rotateInterval;
    if (m_keysPressed.right)
        m_yaw -= rotateInterval;
    if (m_keysPressed.up)
        m_pitch += rotateInterval;
    if (m_keysPressed.down)
        m_pitch -= rotateInterval;

    // Prevent looking too far up or down.
    m_pitch = min(m_pitch, XM_PIDIV4);
    m_pitch = max(-XM_PIDIV4, m_pitch);

    // Move the camera in model space.


    m_position -= XMVectorSet(move.x * -cosf(m_yaw) - move.y * sinf(m_yaw), move.x * sinf(m_yaw) - move.y *cosf(m_yaw), 0, 0) * moveInterval;

    // Determine the look direction.
    float r = cosf(m_pitch); 
    m_lookDirection = XMVectorSet(r * sinf(m_yaw), r * cosf(m_yaw), sinf(m_pitch), 0);
    
}

XMMATRIX SimpleCamera::GetViewMatrix()
{
    return XMMatrixLookToLH(m_position, m_lookDirection, m_upDirection);
}

XMMATRIX SimpleCamera::GetProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane)
{
    return XMMatrixPerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane);
}

void SimpleCamera::OnKeyDown(unsigned char key)
{
    switch (key)
    {
    case 'W':
        m_keysPressed.w = true;
        break;
    case 'A':
        m_keysPressed.a = true;
        break;
    case 'S':
        m_keysPressed.s = true;
        break;
    case 'D':
        m_keysPressed.d = true;
        break;
    case VK_LEFT:
        m_keysPressed.left = true;
        break;
    case VK_RIGHT:
        m_keysPressed.right = true;
        break;
    case VK_UP:
        m_keysPressed.up = true;
        break;
    case VK_DOWN:
        m_keysPressed.down = true;
        break;
    case VK_ESCAPE:
        Reset();
        break;
    }
}

void SimpleCamera::OnKeyUp(unsigned char key)
{
    switch (key)
    {
    case 'W':
        m_keysPressed.w = false;
        break;
    case 'A':
        m_keysPressed.a = false;
        break;
    case 'S':
        m_keysPressed.s = false;
        break;
    case 'D':
        m_keysPressed.d = false;
        break;
    case VK_LEFT:
        m_keysPressed.left = false;
        break;
    case VK_RIGHT:
        m_keysPressed.right = false;
        break;
    case VK_UP:
        m_keysPressed.up = false;
        break;
    case VK_DOWN:
        m_keysPressed.down = false;
        break;
    case VK_ESCAPE:
        Reset();
        break;
    }

    
}
