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
    m_position(XMVectorSet(0, 0, 0, 0)),
    m_yaw(0.0f),
    m_pitch(0.0f),
    m_lookDirection(XMVectorSet(1, 0, 0, 0)),
    m_upDirection(XMVectorSet(0, 0, 1, 0)),
    m_moveSpeed(20.0f),
    m_turnSpeed(XM_PIDIV2)
{
}

void SimpleCamera::Update(XMFLOAT4* pos, XMFLOAT4* rotation, XMFLOAT4* rupdirection)
{

    m_upDirection = XMLoadFloat4(rupdirection);

    m_position = XMLoadFloat4(pos);

    m_lookDirection = XMLoadFloat4(rotation);
}

XMMATRIX SimpleCamera::GetViewMatrix()
{
    return XMMatrixLookToRH(m_position, m_lookDirection, m_upDirection);
}

XMMATRIX SimpleCamera::GetProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane)
{
    return XMMatrixPerspectiveFovRH(fov, aspectRatio, nearPlane, farPlane);
}


