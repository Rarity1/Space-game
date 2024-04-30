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

typedef struct WORKDATA
{
    XMFLOAT4 dir;
    float dist;
    int Index;
    int Index2;
}
WORKDATA;

typedef struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT2 tc;
    XMFLOAT3 normal;
}
Vertex;

typedef struct MODEL
{
    int index[3];
    Vertex vects[3];
}
MODEL;

typedef struct RETURNDATA
{
    bool coll;
    int index1[3];
    int index2[3];
}
RETURNDATA;

typedef struct Intersect
{
    bool coplanar;
    float isect0;
    float isect1;
    XMFLOAT3 isectpoint0;
    XMFLOAT3 isectpoint1;
}
Intersect;

#define SORT(a,b)       \
if (a > b)    \
             {          \
               float c; \
               c = a;     \
               a = b;     \
               b = c;     \
             }

#define POINT_IN_TRI(V0,U0,U1,U2)           \
{                                           \
  float a, b, c, d0, d1, d2;                     \
  /* is T1 completly inside T2? */          \
  /* check if V0 is inside tri(U0,U1,U2) */ \
  a = U1[i1] - U0[i1];                          \
  b = -(U1[i0] - U0[i0]);                       \
  c = -a * U0[i0] - b * U0[i1];                     \
  d0 = a * V0[i0] + b * V0[i1] + c;                   \
                                            \
  a = U2[i1] - U1[i1];                          \
  b = -(U2[i0] - U1[i0]);                       \
  c = -a * U1[i0] - b * U1[i1];                     \
  d1 = a * V0[i0] + b * V0[i1] + c;                   \
                                            \
  a = U0[i1] - U2[i1];                          \
  b = -(U0[i0] - U2[i0]);                       \
  c = -a * U2[i0] - b * U2[i1];                     \
  d2 = a * V0[i0] + b * V0[i1] + c;                   \
  if (d0 * d1 > 0.0)                             \
  {                                         \
    if (d0 * d2 > 0.0) return true;                 \
  }                                         \
}

#define EDGE_EDGE_TEST(V0,U0,U1)                      \
Bx = U0[i0] - U1[i0];                                   \
  By = U0[i1] - U1[i1];                                   \
  Cx = V0[i0] - U0[i0];                                   \
  Cy = V0[i1] - U0[i1];                                   \
  f = Ay * Bx - Ax * By;                                      \
  d = By * Cx - Bx * Cy;                                      \
  if ((f > 0 && d >= 0 && d <= f) || (f < 0 && d <= 0 && d >= f))  \
  {                                                   \
    e = Ax * Cy - Ay * Cx;                                    \
    if (f > 0)                                           \
    {                                                 \
      if (e >= 0 && e <= f) return true;                      \
    }                                                 \
    else                                              \
    {                                                 \
      if (e <= 0 && e >= f) return true;                      \
    }                                                 \
}

#define EDGE_AGAINST_TRI_EDGES(V0,V1,U0,U1,U2) \
{                                              \
  float Ax, Ay, Bx, By, Cx, Cy, e, d, f;               \
  Ax = V1[i0] - V0[i0];                            \
  Ay = V1[i1] - V0[i1];                            \
  /* test edge U0,U1 against V0,V1 */          \
  EDGE_EDGE_TEST(V0, U0, U1);                    \
  /* test edge U1,U2 against V0,V1 */          \
  EDGE_EDGE_TEST(V0, U1, U2);                    \
  /* test edge U2,U1 against V0,V1 */          \
  EDGE_EDGE_TEST(V0, U2, U0);                    \
}



float fDistance(XMFLOAT3 pos1, XMFLOAT3 pos2)
{
    float x = pow((pos2.x - pos1.x), 2);
    float y = pow((pos2.y - pos1.y), 2);
    float z = pow((pos2.z - pos1.z), 2);
    return sqrt(x + y + z);
}

XMFLOAT3 AddXMFLOAT3(XMFLOAT3 a, XMFLOAT3 b)
{
    XMFLOAT3 result = { 0, 0, 0 };

    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;

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

XMFLOAT3 MulXMFLOAT3(XMFLOAT3 a, float b)
{
    XMFLOAT3 result = { 0, 0, 0 };

    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;

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
float XMVector3Dot(XMFLOAT3 a, XMFLOAT3 b)
{
    float result = 0;
    result += a.x * b.x;
    result += a.y * b.y;
    result += a.z * b.z;
    return result;
}

bool CoplanTriTri(XMFLOAT3 XN, float V0[3], float V1[3], float V2[3],
                     float U0[3], float U1[3], float U2[3])
{
    float A[3];
    short i0, i1;
    A[0] = fabs(XN.x);
    A[1] = fabs(XN.y);
    A[2] = fabs(XN.z);
    if (A[0] > A[1])
    {
        if (A[0] > A[2])
        {
            i0 = 1;      /* A[0] is greatest */
            i1 = 2;
        }
        else
        {
            i0 = 0;      /* A[2] is greatest */
            i1 = 1;
        }
    }
    else   /* A[0]<=A[1] */
    {
        if (A[2] > A[1])
        {
            i0 = 0;      /* A[2] is greatest */
            i1 = 1;
        }
        else
        {
            i0 = 0;      /* A[1] is greatest */
            i1 = 2;
        }
    }

    EDGE_AGAINST_TRI_EDGES(V0, V1, U0, U1, U2);
    EDGE_AGAINST_TRI_EDGES(V1, V2, U0, U1, U2);
    EDGE_AGAINST_TRI_EDGES(V2, V0, U0, U1, U2);

    POINT_IN_TRI(V0, U0, U1, U2);
    POINT_IN_TRI(U0, V0, V1, V2);

    return false;
}


#define NEWCOMPUTE_INTERVALS(VV0,VV1,VV2,D0,D1,D2,D0D1,D0D2,A,B,C,X0,X1) \
{ \
        if (D0D1 > 0.0f) \
        { \
                /* here we know that D0D2<=0.0 */ \
            /* that is D0, D1 are on the same side, D2 on the other or on the plane */ \
                A = VV2; B = (VV0 - VV2) * D2; C = (VV1 - VV2) * D2; X0 = D2 - D0; X1 = D2 - D1; \
        } \
        else if (D0D2 > 0.0f)\
        { \
                /* here we know that d0d1<=0.0 */ \
            A = VV1; B = (VV0 - VV1) * D1; C = (VV2 - VV1) * D1; X0 = D1 - D0; X1 = D1 - D2; \
        } \
        else if (D1 * D2 > 0.0f || D0 != 0.0f) \
        { \
                /* here we know that d0d1<=0.0 or that D0!=0.0 */ \
                A = VV0; B = (VV1 - VV0) * D0; C = (VV2 - VV0) * D0; X0 = D0 - D1; X1 = D0 - D2; \
        } \
        else if (D1 != 0.0f) \
        { \
                A = VV1; B = (VV0 - VV1) * D1; C = (VV2 - VV1) * D1; X0 = D1 - D0; X1 = D1 - D2; \
        } \
        else if (D2 != 0.0f) \
        { \
                A = VV2; B = (VV0 - VV2) * D2; C = (VV1 - VV2) * D2; X0 = D2 - D0; X1 = D2 - D1; \
        } \
        else \
        { \
                /* triangles are coplanar */ \
                return CoplanTriTri(NPlane1, V0, V1, V2, U0, U1, U2); \
        } \
}

bool Intersects(MODEL Tri1, MODEL Tri2)
{
    float xx, yy, xxyy, tmp;
    float isect1[2], isect2[2];
    float a, b, c, x0, x1;
    float d, e, f, y0, y1;
    XMFLOAT3 ZeroVert = { 0, 0, 0 };
    XMFLOAT3 BVert0 = Tri2.vects[0].position;
    XMFLOAT3 BVert1 = Tri2.vects[1].position;
    XMFLOAT3 BVert2 = Tri2.vects[2].position;
    XMFLOAT3 AVert0 = Tri1.vects[0].position;
    XMFLOAT3 AVert1 = Tri1.vects[1].position;
    XMFLOAT3 AVert2 = Tri1.vects[2].position;



    XMFLOAT3 NPlane1 = XMVector3Cross(SubXMFLOAT3(AVert1, AVert0), SubXMFLOAT3(AVert2, AVert0));
    float ADist = -XMVector3Dot(NPlane1, AVert0);

    float BDist0 = XMVector3Dot(NPlane1, BVert0) + ADist;
    float BDist1 = XMVector3Dot(NPlane1, BVert1) + ADist;
    float BDist2 = XMVector3Dot(NPlane1, BVert2) + ADist;


    float B0xB1 = BDist0 * BDist1;
    float B0xB2 = BDist0 * BDist2;
    if (B0xB1 > 0.0 && B0xB2 > 0.0)
    {
        return false;
    }

    XMFLOAT3 NPlane2 = XMVector3Cross(SubXMFLOAT3(BVert1, BVert0), SubXMFLOAT3(BVert2, BVert0));
    float BDist = -XMVector3Dot(NPlane2, BVert0);

    float ADist0 = XMVector3Dot(NPlane2, AVert0) + BDist;
    float ADist1 = XMVector3Dot(NPlane2, AVert1) + BDist;
    float ADist2 = XMVector3Dot(NPlane2, AVert2) + BDist;


    float A0xA1 = ADist0 * ADist1;
    float A0xA2 = ADist0 * ADist2;
    if (A0xA1 > 0.0 && A0xA2 > 0.0)
    {
        return false;
    }


    XMFLOAT3 IntersectLineDir = XMVector3Cross(NPlane1, NPlane2);


    int index = 0;
    float max = fabs(IntersectLineDir.x);
    float bb = fabs(IntersectLineDir.y);
    float cc = fabs(IntersectLineDir.z);
    if (bb > max) max = bb, index = 1;
    if (cc > max) max = cc, index = 2;

    float V0[3];
    float V1[3];
    float V2[3];
    float U0[3];
    float U1[3];
    float U2[3];

    V0[0] = AVert0.x;
    V0[1] = AVert0.y;
    V0[2] = AVert0.z;
    V1[0] = AVert1.x;
    V1[1] = AVert1.y;
    V1[2] = AVert1.z;
    V2[0] = AVert2.x;
    V2[1] = AVert2.y;
    V2[2] = AVert2.z;
    U0[0] = BVert0.x;
    U0[1] = BVert0.y;
    U0[2] = BVert0.z;
    U1[0] = BVert1.x;
    U1[1] = BVert1.y;
    U1[2] = BVert1.z;
    U2[0] = BVert2.x;
    U2[1] = BVert2.y;
    U2[2] = BVert2.z;


    float vp0 = 0.0;
    float vp1 = 0.0;
    float vp2 = 0.0;

    float up0 = 0.0;
    float up1 = 0.0;
    float up2 = 0.0;

    vp0 = V0[index];
    vp1 = V1[index];
    vp2 = V2[index];

    up0 = U0[index];
    up1 = U1[index];
    up2 = U2[index];

    NEWCOMPUTE_INTERVALS(vp0, vp1, vp2, ADist0, ADist1, ADist2, A0xA1, A0xA2, a, b, c, x0, x1);

    NEWCOMPUTE_INTERVALS(up0, up1, up2, BDist0, BDist1, BDist2, B0xB1, B0xB2, d, e, f, y0, y1);


    xx = x0 * x1;
    yy = y0 * y1;
    xxyy = xx * yy;


    tmp = a * xxyy;
    isect1[0] = tmp + b * x1 * yy;
    isect1[1] = tmp + c * x0 * yy;

    tmp = d * xxyy;
    isect2[0] = tmp + e * xx * y1;
    isect2[1] = tmp + f * xx * y0;

    SORT(isect1[0], isect1[1]);
    SORT(isect2[0], isect2[1]);

    if (isect1[1] < isect2[0] || isect2[1] < isect1[0]) return false;

    return true;
}


void kernel coll(global const MODEL* WModel, global const MODEL* TModel, global XMFLOAT3* Wposition, global XMFLOAT3* Tposition, global const int* sizeT, global RETURNDATA* retdat){
    int id = get_global_id(0);
    int Tid = get_global_id(1);
bool temp = retdat[id].coll;
MODEL Work = WModel[id];
Work.vects[0].position = AddXMFLOAT3(Work.vects[0].position, Wposition[0]);
Work.vects[1].position = AddXMFLOAT3(Work.vects[1].position, Wposition[0]);
Work.vects[2].position = AddXMFLOAT3(Work.vects[2].position, Wposition[0]);
MODEL WorkT = TModel[Tid];
WorkT.vects[0].position = AddXMFLOAT3(WorkT.vects[0].position, Tposition[0]);
WorkT.vects[1].position = AddXMFLOAT3(WorkT.vects[1].position, Tposition[0]);
WorkT.vects[2].position = AddXMFLOAT3(WorkT.vects[2].position, Tposition[0]);
temp = Intersects(Work, WorkT);

if (temp)
{
    retdat[id].coll = true;
    retdat[id].index1[0] = Work.index[0];
    retdat[id].index1[1] = Work.index[1];
    retdat[id].index1[2] = Work.index[2];
    retdat[id].index2[0] = WorkT.index[0];
    retdat[id].index2[1] = WorkT.index[1];
    retdat[id].index2[2] = WorkT.index[2];
}

}