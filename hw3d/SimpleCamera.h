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
    void Update(XMFLOAT4* pos, XMFLOAT4* rotation, XMFLOAT4* rupdirection);
    FXMMATRIX GetViewMatrix();
    CXMMATRIX GetProjectionMatrix(float fov, float aspectRatio, float nearPlane = 0.1f, float farPlane = 100000.0f);
private:
    void Reset();
    XMVECTOR m_position;
    XMVECTOR m_lookDirection;
    XMVECTOR m_upDirection;
};
