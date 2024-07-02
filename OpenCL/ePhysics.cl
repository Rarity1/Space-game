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

typedef struct CINTERVAL
{
    float V0;
    float V1;
    float V2;
    float X0;
    float X1;
    bool CoPlan;
}
CINTERVAL;

typedef struct BONE
{
    XMFLOAT3 Position;
    int numIndices;
    int iOffset;
}
BONE;

typedef struct MODEL
{
    int index[3];
    XMFLOAT3 vects[3];
}
MODEL;

typedef struct WORKDATA
{
    int bIndex[2];
    XMFLOAT3 Position;
    int wWorkCount;
    int tWorkCount;
    int tOffset;
}
WORKDATA;

typedef struct RETURNDATA
{
    bool coll;
    int index1[3];
    int index2[3];
    XMFLOAT4 dir[2];
    float dist[2];
}
RETURNDATA;


typedef struct CLOSEFORM
{
    bool coll;
    XMFLOAT3 norm[2];
    float dist;
}
CLOSEFORM;

typedef struct INTINDEX
{
    int Index[3];
}INTINDEX;


#define SORT(min,max)        \
          if (min > max)     \
                 {           \
                float temp;  \
                temp = min;  \
                min = max;   \
                max = temp;  \
             }




float fDistance(XMFLOAT3 pos1, XMFLOAT3 pos2)
{
    float x = pow((pos2.x - pos1.x), 2);
    float y = pow((pos2.y - pos1.y), 2);
    float z = pow((pos2.z - pos1.z), 2);
    return sqrt(x + y + z);
}

XMFLOAT3 fDirection(XMFLOAT3 pos1, XMFLOAT3 pos2) {
    XMFLOAT3 result = { 0, 0, 0};
    result.x = (pos2.x - pos1.x);
    result.y = (pos2.y - pos1.y);
    result.z = (pos2.z - pos1.z);
    float mag = fDistance(pos1, pos2);
    result.x = result.x / mag;
    result.y = result.y / mag;
    result.z = result.z / mag;
    return result;
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
float Xyzret(XMFLOAT3 V, int Case)
{
    switch (Case)
    {
        case 0:
            {
                return V.x;
            }
        case 1:
            {
                return V.y;
            }
        case 2:
            {
                return V.z;
            }
    }
}
bool EdgeEdgeTest(float Axy[2], XMFLOAT3 V0, XMFLOAT3 U0, XMFLOAT3 U1, int Case0, int Case1)
{
    float Bx, By, Cx, Cy, e, d, f;
    float Ax = Axy[0];
    float Ay = Axy[1];
    switch (Case0)
    {
        case 0:
            {
                Bx = U0.x - U1.x;
                Cx = V0.x - U0.x;
                break;
            }
        case 1:
            {
                Bx = U0.y - U1.y;
                Cx = V0.y - U0.y;
                break;
            }
        case 2:
            {
                Bx = U0.z - U1.z;
                Cx = V0.z - U0.z;
                break;
            }
    }

    switch (Case1)
    {
        case 0:
            {
                By = U0.x - U1.x;
                Cy = V0.x - U0.x;
                break;
            }
        case 1:
            {
                By = U0.y - U1.y;
                Cy = V0.y - U0.y;
                break;
            }
        case 2:
            {
                By = U0.z - U1.z;
                Cy = V0.z - U0.z;
                break;
            }
    }


    f = Ay * Bx - Ax * By;
    d = By * Cx - Bx * Cy;
    if ((f > 0 && d >= 0 && d <= f) || (f < 0 && d <= 0 && d >= f))
    {
        e = Ax * Cy - Ay * Cx;
        if (f > 0)
        {
            if (e >= 0 && e <= f) return true;
        }
        else
        {
            if (e <= 0 && e >= f) return true;
        }
    }

    return false;
}

bool EdgeTriTest(XMFLOAT3 V0, XMFLOAT3 V1, XMFLOAT3 U0, XMFLOAT3 U1, XMFLOAT3 U2, int Case0, int Case1)
{
    bool Result = false;
    float Axy[2];
    switch (Case0)
    {
        case 0:
            {
                Axy[0] = V1.x - V0.x;
                break;
            }
        case 1:
            {
                Axy[0] = V1.y - V0.y;
                break;
            }
        case 2:
            {
                Axy[0] = V1.z - V0.z;
                break;
            }
    }
    switch (Case1)
    {
        case 0:
            {
                Axy[1] = V1.x - V0.x;
                break;
            }
        case 1:
            {
                Axy[1] = V1.y - V0.y;
                break;
            }
        case 2:
            {
                Axy[1] = V1.z - V0.z;
                break;
            }
    }

    Result = EdgeEdgeTest(Axy, V0, U0, U1, Case0, Case1);
    Result = Result ? true : EdgeEdgeTest(Axy, V0, U1, U2, Case0, Case1);
    Result = Result ? true : EdgeEdgeTest(Axy, V0, U2, U0, Case0, Case1);


    return Result;
}



bool PointInTri(XMFLOAT3 V0, XMFLOAT3 U0, XMFLOAT3 U1, XMFLOAT3 U2, int Case0, int Case1)
{

                                              
      float a, b, c, d0, d1, d2;
    /* is T1 completly inside T2? */
    /* check if V0 is inside tri(U0,U1,U2) */

    float U00, U01, U10, U11, U20, U21, V1, V2;
    
    U00 = Xyzret(U0, Case0);
    U01 = Xyzret(U0, Case1);
    U10 = Xyzret(U1, Case0);
    U11 = Xyzret(U1, Case1);
    U20 = Xyzret(U2, Case0);
    U21 = Xyzret(U2, Case1);
    V1 = Xyzret(V0, Case0);
    V2 = Xyzret(V0, Case1);


      a = U11 - U01;                          
      b = -(U10 - U00);                       
      c = -a * U00 - b * U01;                     
      d0 = a * V1 + b * V2 + c;                   
                                                
      a = U21 - U11;                          
      b = -(U20 - U10);                       
      c = -a * U10 - b * U11;                     
      d1 = a * V1 + b * V2 + c;                   
                                                
      a = U01 - U21;                          
      b = -(U00 - U20);                       
      c = -a * U20 - b * U21;                     
      d2 = a * V1 + b * V2 + c;                   
      if (d0 * d1 > 0.0)                             
      {                                         
        if (d0 * d2 > 0.0) return true;                 
      }                                         

}

bool CoplanCheck(XMFLOAT3 XN, XMFLOAT3 V0, XMFLOAT3 V1, XMFLOAT3 V2,
                     XMFLOAT3 U0, XMFLOAT3 U1, XMFLOAT3 U2)
{
    bool Result = false;
    float A[3];
    int i0, i1;
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

    Result = EdgeTriTest(V0, V1, U0, U1, U2, i0, i1);
    Result = Result ? true : EdgeTriTest(V1, V2, U0, U1, U2, i0, i1);
    Result = Result ? true : EdgeTriTest(V2, V0, U0, U1, U2, i0, i1);

    Result = Result ? true : PointInTri(V0, U0, U1, U2, i0, i1);
    Result = Result ? true : PointInTri(U0, V0, V1, V2, i0, i1);

    return Result;
}


CINTERVAL ComputeInterval(float VV0, float VV1, float VV2, float Dist0, float Dist1, float Dist2)
{
    CINTERVAL Result;
    Result.CoPlan = false;

    float D0D1 = Dist0 * Dist1;
    float D0D2 = Dist0 * Dist2;
    float A, B, C, X0, X1 = 0;

        if (D0D1 > 0.0f) 
        { 
                /* here we know that D0D2<=0.0 */ 
            /* that is D0, D1 are on the same side, D2 on the other or on the plane */ 
                A = VV2; B = (VV0 - VV2) * Dist2; C = (VV1 - VV2) * Dist2; X0 = Dist2 - Dist0; X1 = Dist2 - Dist1; 
        } 
        else if (D0D2 > 0.0f)
        { 
                /* here we know that d0d1<=0.0 */ 
            A = VV1; B = (VV0 - VV1) * Dist1; C = (VV2 - VV1) * Dist1; X0 = Dist1 - Dist0; X1 = Dist1 - Dist2; 
        } 
        else if (Dist1 * Dist2 > 0.0f || Dist0 != 0.0f) 
        { 
                /* here we know that d0d1<=0.0 or that D0!=0.0 */ 
                A = VV0; B = (VV1 - VV0) * Dist0; C = (VV2 - VV0) * Dist0; X0 = Dist0 - Dist1; X1 = Dist0 - Dist2; 
        } 
        else if (Dist1 != 0.0f) 
        { 
                A = VV1; B = (VV0 - VV1) * Dist1; C = (VV2 - VV1) * Dist1; X0 = Dist1 - Dist0; X1 = Dist1 - Dist2; 
        } 
        else if (Dist2 != 0.0f) 
        { 
                A = VV2; B = (VV0 - VV2) * Dist2; C = (VV1 - VV2) * Dist2; X0 = Dist2 - Dist0; X1 = Dist2 - Dist1; 
        } 
        else 
        {
        /* triangles are coplanar */
        Result.CoPlan = true;
        }

    Result.V0 = A;
    Result.V1 = B;
    Result.V2 = C;
    Result.X0 = X0;
    Result.X1 = X1;

    return Result;
}

float SwitchLowest(XMFLOAT3 APoint, XMFLOAT3 BPoint, XMFLOAT3 CPoint, XMFLOAT3 BoneDir, XMFLOAT3 Bone,int Case)
{
    float BScal = XMVector3Dot(BoneDir, BoneDir);

    float Result = 0;

    switch (Case)
    {
        case 0:
            {
                Result = XMVector3Dot(SubXMFLOAT3(APoint, Bone), BoneDir) / BScal;
                break;
            }
        case 1:
            {
                Result = XMVector3Dot(SubXMFLOAT3(BPoint, Bone), BoneDir) / BScal;
                break;
            }
        case 2:
            {
                Result = XMVector3Dot(SubXMFLOAT3(CPoint, Bone), BoneDir) / BScal;
                break;
            }
    }

    return Result;
}


//Why doesnt this workkk
float CloseDistanceCheck(XMFLOAT3 Bone1, XMFLOAT3 Bone2, float TDistance[3], float WDistance[3], XMFLOAT3 TAPoint, XMFLOAT3 TBPoint, XMFLOAT3 TCPoint, XMFLOAT3 WAPoint, XMFLOAT3 WBPoint, XMFLOAT3 WCPoint)
{
    XMFLOAT3 BoneDir = fDirection(Bone1, Bone2);
    float bDist = fDistance(Bone1, Bone2);
    XMFLOAT3 iBDir = MulXMFLOAT3(BoneDir, -1);

    int TLowestInd = 0;
    if (TDistance[1] < TDistance[0])
    {
        TLowestInd = 1;
        if (TDistance[2] < TDistance[1])
        {
            TLowestInd = 2;
        }
    }
    else if (TDistance[2] < TDistance[0])
    {
        TLowestInd = 2;
    }
    float TPointDist = SwitchLowest(iBDir, TAPoint, TBPoint, TCPoint, Bone2,TLowestInd);

    XMFLOAT3 TNewPoint = AddXMFLOAT3( MulXMFLOAT3(iBDir, sqrt(TPointDist)), Bone2);

    int WLowestInd = 0;
    if (WDistance[1] < WDistance[0])
    {
        WLowestInd = 1;
        if (WDistance[2] < WDistance[1])
        {
            WLowestInd = 2;
        }
    }
    else if (WDistance[2] < WDistance[0])
    {
        WLowestInd = 2;
    }
    float WPointDist = SwitchLowest(BoneDir, WAPoint, WBPoint, WCPoint, Bone1,WLowestInd);
    XMFLOAT3 WNewPoint = AddXMFLOAT3(MulXMFLOAT3(BoneDir, sqrt(WPointDist)), Bone1);

    return fDistance(WNewPoint, Bone2)+fDistance(TNewPoint, Bone1);
}

CLOSEFORM TooClose(MODEL Tri1, MODEL Tri2, XMFLOAT3 Bone1, XMFLOAT3 TBone, XMFLOAT3 TPos)
{
    CLOSEFORM Result;
    Result.dist = 0.0;
    Result.coll = true;

    XMFLOAT3 Zero = {0,0,0};

    Result.norm[0] = Zero;
    Result.norm[1] = Zero;

    XMFLOAT3 Bone2 = AddXMFLOAT3(TBone, TPos);
    float bdist = fDistance(Bone1, Bone2);

    XMFLOAT3 WAPoint = Tri1.vects[0];
    XMFLOAT3 WBPoint = Tri1.vects[1];
    XMFLOAT3 WCPoint = Tri1.vects[2];
    XMFLOAT3 TAPoint = Tri2.vects[0];
    XMFLOAT3 TBPoint = Tri2.vects[1];
    XMFLOAT3 TCPoint = Tri2.vects[2];
    TAPoint = AddXMFLOAT3(TAPoint, TPos);
    TBPoint = AddXMFLOAT3(TBPoint, TPos);
    TCPoint = AddXMFLOAT3(TCPoint, TPos);


    int TIndex = 0;
    int TIndex1 = 1;
    int TIndex2 = 2;

    int WIndex = 0;
    int WIndex1 = 1;
    int WIndex2 = 2;


    XMFLOAT3 TDir[3];
    XMFLOAT3 WDir[3];

    float TDistance[3];
    float WDistance[3];



    XMFLOAT3 WNorm = XMVector3Cross(SubXMFLOAT3(WBPoint, WAPoint), SubXMFLOAT3(WCPoint, WAPoint));
    XMFLOAT3 TNorm;
    XMFLOAT3 ISectLineDir;
    int iSectdex = 0;


    XMFLOAT3 WAverage = MulXMFLOAT3(AddXMFLOAT3(AddXMFLOAT3(WAPoint, WBPoint), WCPoint), 1.0/3.0);


    //Check all sides then set collision apropriately. Check if inside other object
    float WScalar = -XMVector3Dot(WNorm, WAverage);

    TDistance[0] = (XMVector3Dot(WNorm, TAPoint)) + WScalar;
    TDistance[1] = (XMVector3Dot(WNorm, TBPoint)) + WScalar;
    TDistance[2] = (XMVector3Dot(WNorm, TCPoint)) + WScalar;

    if (TDistance[0] * TDistance[1] > 0 && TDistance[0] * TDistance[2] > 0)
    {
        Result.coll = false;
    }

    //If T Tri is all on one side or not; false if it is, true if not
    if (Result.coll)
    {
        TNorm = XMVector3Cross(SubXMFLOAT3(TBPoint, TAPoint), SubXMFLOAT3(TCPoint, TAPoint));
        XMFLOAT3 TAverage = MulXMFLOAT3(AddXMFLOAT3(AddXMFLOAT3(TAPoint, TBPoint), TCPoint), 1.0 / 3.0);

        float TScalar = -XMVector3Dot(TNorm, TAverage);


        ISectLineDir = XMVector3Cross(WNorm, TNorm);


        float ISect0 = fabs(ISectLineDir.x);
        if (fabs(ISectLineDir.y) > ISect0) ISect0 = fabs(ISectLineDir.y), iSectdex = 1;
        if (fabs(ISectLineDir.z) > ISect0) ISect0 = fabs(ISectLineDir.z), iSectdex = 2;

        WDistance[0] = (XMVector3Dot(TNorm, WAPoint)) + TScalar;
        WDistance[1] = (XMVector3Dot(TNorm, WBPoint)) + TScalar;
        WDistance[2] = (XMVector3Dot(TNorm, WCPoint)) + TScalar;

        if (WDistance[0] * WDistance[1] > 0 && WDistance[0] * WDistance[2] > 0)
        {
            Result.coll = false;
        }

        CINTERVAL cval0;
        CINTERVAL cval1;
        switch (iSectdex)
        {
            case 0:
                {
                    cval0 = ComputeInterval(WAPoint.x, WBPoint.x, WCPoint.x, WDistance[0], WDistance[1], WDistance[2]);
                    cval1 = ComputeInterval(TAPoint.x, TBPoint.x, TCPoint.x, TDistance[0], TDistance[1], TDistance[2]);
                    break;
                }
            case 1:
                {
                    cval0 = ComputeInterval(WAPoint.y, WBPoint.y, WCPoint.y, WDistance[0], WDistance[1], WDistance[2]);
                    cval1 = ComputeInterval(TAPoint.y, TBPoint.y, TCPoint.y, TDistance[0], TDistance[1], TDistance[2]);
                    break;
                }
            case 2:
                {
                    cval0 = ComputeInterval(WAPoint.z, WBPoint.z, WCPoint.z, WDistance[0], WDistance[1], WDistance[2]);
                    cval1 = ComputeInterval(TAPoint.z, TBPoint.z, TCPoint.z, TDistance[0], TDistance[1], TDistance[2]);
                    break;
                }
        }


        

        if (cval0.CoPlan)
        {
            Result.coll = CoplanCheck(WNorm, WAPoint, WBPoint, WCPoint, TAPoint, TBPoint, TCPoint);
        }
        else
        {
            float xx = cval0.X0 * cval0.X1;
            float yy = cval1.X0 * cval1.X1;
            float xxyy = xx * yy;


            float tmp = cval0.V0 * xxyy;
            float i = tmp + cval0.V1 * cval0.X1 * yy;
            float i1 = tmp + cval0.V2 * cval0.X0 * yy;

            tmp = cval1.V0 * xxyy;
            float u = tmp + cval1.V1 * xx * cval1.X1;
            float u1 = tmp + cval1.V2 * xx * cval1.X0;


            float test0[2];
            float test1[2];

            test0[0] = i;
            test0[1] = i1;
            test1[0] = u;
            test1[1] = u1;

            SORT(test0[0], test0[1]);
            SORT(test1[0], test1[1]);
            if (test0[1] < test1[0] || test1[1] < test0[0]) Result.coll = false;

            int TIndex = 0;

            if (TDistance[0] > TDistance[1])
            {
                TIndex = 1;
                if (TDistance[1]  > TDistance[2] )
                {
                    TIndex = 2;
                }
            }
            else
            {
                if (TDistance[0]  > TDistance[2] )
                {
                    TIndex = 2;
                }
            }

            int WIndex = 0;

            if (WDistance[0] > WDistance[1])
            {
                WIndex = 1;
                if (WDistance[1] > WDistance[2])
                {
                    WIndex = 2;
                }
            }
            else
            {
                if (WDistance[0] > WDistance[2])
                {
                    WIndex = 2;
                }
            }
            Result.dist = TDistance[TIndex] > WDistance[WIndex] ? -(TDistance[TIndex]) : -(WDistance[WIndex]);
            Result.norm[0] = MulXMFLOAT3(WNorm, 1 / fDistance(WNorm, Zero));
            Result.norm[1] = MulXMFLOAT3(TNorm, 1 / fDistance(TNorm, Zero));
        }


    }
    else
    {

        //Figure out why this doesnt work & remove false
        
        if (TDistance[0] < 0 && false)
        {
            TNorm = XMVector3Cross(SubXMFLOAT3(TBPoint, TAPoint), SubXMFLOAT3(TCPoint, TAPoint));
            XMFLOAT3 TAverage = MulXMFLOAT3(AddXMFLOAT3(AddXMFLOAT3(TAPoint, TBPoint), TCPoint), 1.0 / 3.0);

            float TScalar = -XMVector3Dot(TNorm, TAverage);

            WDistance[0] = (XMVector3Dot(TNorm, WAPoint)) + TScalar;
            WDistance[1] = (XMVector3Dot(TNorm, WBPoint)) + TScalar;
            WDistance[2] = (XMVector3Dot(TNorm, WCPoint)) + TScalar;

            if (WDistance[0] < 0 || WDistance[1] < 0 || WDistance[2] < 0)
            {

                //Check distance from bone that objects are colliding
                if(CoplanCheck(WNorm, WAPoint, WBPoint, WCPoint, TAPoint, TBPoint, TCPoint))
                {
                    float dist = CloseDistanceCheck(Bone1, Bone2, TDistance, WDistance, TAPoint, TBPoint, TCPoint, WAPoint, WBPoint, WCPoint);
                    XMFLOAT3 Bonedir = fDirection(Bone1, Bone2);
                    float TDiff = XMVector3Dot(fDirection(Zero, TNorm), Bonedir);
                    float WDiff = -XMVector3Dot(fDirection(Zero, WNorm), Bonedir);
                    if (TDiff < 0 && WDiff < 0 && dist - fDistance(Bone1, Bone2) < 0)
                    {

                            Result.coll = true;
                            Result.dist = fDistance(Bone1, Bone2) - dist;
                            Result.norm[0] = MulXMFLOAT3(WNorm, 1 / fDistance(WNorm, Zero));
                            Result.norm[1] = MulXMFLOAT3(TNorm, 1 / fDistance(TNorm, Zero));
                    }
                    
                }

            }

        }
        
    }
    

    return Result;
}


void kernel coll(global const XMFLOAT3* WModel, global const XMFLOAT3* TModel, global const XMFLOAT3* WBone, global const XMFLOAT3* TBone, global const INTINDEX* WbIndexBuff, global const INTINDEX* TbIndexBuff, global const WORKDATA* WData, global const int* WorkingIndices, global RETURNDATA* retdat){
int sd = get_global_id(0);
int Wind = get_global_id(1);
int Tind = get_global_id(2);



if (!retdat[sd].coll)
{

    XMFLOAT3 TPos = WData[sd].Position;

    MODEL Work = { {0,0,0 }, { WModel[WbIndexBuff[WorkingIndices[Wind + WData[sd].tOffset]].Index[0]], WModel[WbIndexBuff[WorkingIndices[Wind + WData[sd].tOffset]].Index[1]], WModel[WbIndexBuff[WorkingIndices[Wind + WData[sd].tOffset]].Index[2]] } };


    MODEL TWork = { { 0, 0, 0 }, { TModel[TbIndexBuff[WorkingIndices[Tind + WData[sd].wWorkCount + WData[sd].tOffset]].Index[0]], TModel[TbIndexBuff[WorkingIndices[Tind + WData[sd].wWorkCount + WData[sd].tOffset]].Index[1]], TModel[TbIndexBuff[WorkingIndices[Tind + WData[sd].wWorkCount + WData[sd].tOffset]].Index[2]] } };

    CLOSEFORM Result = TooClose(Work, TWork, WBone[WData[sd].bIndex[0]], TBone[WData[sd].bIndex[1]], TPos);


    bool fart = false;
    if (Result.coll)
    {
        fart = true;
    }

    if (fart)
    {
        retdat[sd].index1[0] = WbIndexBuff[WorkingIndices[Wind]].Index[0];
        retdat[sd].index1[1] = TbIndexBuff[WorkingIndices[Tind + WData[sd].wWorkCount + WData[sd].tOffset]].Index[0];
        retdat[sd].index1[2] = Wind;
        retdat[sd].coll = Result.coll;
        retdat[sd].dist[0] = Result.dist;
        retdat[sd].dir[0].x = Result.norm[0].x;
        retdat[sd].dir[0].y = Result.norm[0].y;
        retdat[sd].dir[0].z = Result.norm[0].z;
        retdat[sd].dir[1].x = Result.norm[1].x;
        retdat[sd].dir[1].y = Result.norm[1].y;
        retdat[sd].dir[1].z = Result.norm[1].z;


    }


}



}