typedef struct XMFLOAT4
{
    float x;
    float y;
    float z;
    float w;
}
XMFLOAT4;

typedef struct XMFLOAT2
{
    float x;
    float y;
}
XMFLOAT2;

typedef struct XMFLOAT3
{
    float x;
    float y;
    float z;
}
XMFLOAT3;

typedef struct INTINDEX
{
    int Index[3];
}
INTINDEX;

typedef struct UPVERTNORM
{
    XMFLOAT3 Vert;
    XMFLOAT3 Norm;
}
UPVERTNORM;

typedef struct SPHERE
{
    XMFLOAT3 Center;
    double Radius;
}
SPHERE;


XMFLOAT3 MulXMFLOAT3(XMFLOAT3 a, float b)
{
    XMFLOAT3 result = { 0, 0, 0 };

    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;

    return result;
}

XMFLOAT3 SubXMFLOAT3(XMFLOAT3 a, XMFLOAT3 b)
{
    XMFLOAT3 result = { 0, 0, 0 };

    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;

    return result;
}

float XMVector3Dot(XMFLOAT3 a, XMFLOAT3 b)
{
    float result = 0;
    result += a.x * b.x;
    result += a.y * b.y;
    result += a.z * b.z;
    return result;
}

float fDistance(XMFLOAT3 pos1, XMFLOAT3 pos2)
{
    XMFLOAT3 Result = SubXMFLOAT3(pos2, pos1);
    float dist = XMVector3Dot(Result, Result);
    if (dist <= 0.001)
    {
        return 0.0;
    }
    else
    {
        return sqrt(dist);
    }
}

XMFLOAT3 fDirection(XMFLOAT3 pos1, XMFLOAT3 pos2)
{
    XMFLOAT3 Result = SubXMFLOAT3(pos2, pos1);
    float mag = fDistance(pos1, pos2);
    Result = MulXMFLOAT3(Result, 1 / mag);
    return Result;
}

XMFLOAT3 AddXMFLOAT3(XMFLOAT3 a, XMFLOAT3 b)
{
    XMFLOAT3 result = { 0, 0, 0 };

    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;

    return result;
}





XMFLOAT3 XMVector3Cross(XMFLOAT3 a, XMFLOAT3 b)
{
    XMFLOAT3 result = { 0, 0, 0 };

    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;

    return result;
}


void kernel sphColl(global const SPHERE* Sphere, global const int* workIndex, global const UPVERTNORM* Model, global const int* IndexMap, global const INTINDEX* IndexBuff, global int* RetData){
    int sd = get_global_id(0);
    int off = get_global_id(1);

    if ( XMVector3Dot(Model[workIndex[sd]].Norm, Sphere[off].Center) >= 0.0)
    {
        RetData[sd] = 69;
    }
}