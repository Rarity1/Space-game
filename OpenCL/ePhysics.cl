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

typedef struct UPVERTNORM
{
    XMFLOAT3 Vert;
    XMFLOAT3 Norm;
}
UPVERTNORM;


typedef struct MODEL
{
    int index[3];
    UPVERTNORM vects[3];
}
MODEL;

typedef struct WORKDATA
{
    int bIndex[2];
    XMFLOAT3 Position;
    int wWorkCount;
    int tWorkCount;
    int Offset;
}
WORKDATA;


typedef struct RETURNDATA
{
    bool coll;
    int index1[3];
    int index2[3];
    XMFLOAT3 dir[2];
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
    if(dist <= 0.001)
    {
        return 0.0;
    }
    else
    {
        return sqrt(dist);
    }
}

XMFLOAT3 fDirection(XMFLOAT3 pos1, XMFLOAT3 pos2) {
    XMFLOAT3 Result = SubXMFLOAT3(pos2, pos1);
    float mag = fDistance(pos1, pos2);
    Result = MulXMFLOAT3(Result, 1/mag);
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

int Sign(float x)
{
    int result = 0;
    if(x != 0)
    {
        if (x < 0) result = -1;
        if (x > 0) result = 1;
    }

    return result;
}

int OddOneOut(float x, float y, float z)
{

    int result = 0;
    if(Sign(x) * Sign(y) > 0)
    {
        result = 2;
    }else if(Sign(x) * Sign(z) > 0)
    {
        result = 1;
    }
    else if (Sign(y) * Sign(z) > 0 || Sign(x) != 0)
    {
        result = 0;
    }else if(Sign(y) != 0)
    {
        result = 1;
    }else if(Sign(z) != 0)
    {
        result = 2;
    }
    else
    {
        result = 3;
    }


    return result;
}

//Dist is dist from plane. Positive means in front negative means behind. 
CINTERVAL ComputeInterval(float VV0, float VV1, float VV2, float Dist0, float Dist1, float Dist2)
{
    CINTERVAL Result;
    Result.CoPlan = false;

    float A, B, C, X0, X1 = 0;

    //A or X should be odd man out. YZ or BC should be on same side.

    switch (OddOneOut(Dist0, Dist1, Dist2))
    {
        case 0:
            {

                A = VV0;
                B = (VV1 - VV0) * Dist0;
                C = (VV2 - VV0) * Dist0;
                X0 = Dist0 - Dist1;
                X1 = Dist0 - Dist2;
                break;
            }
        case 1:
            {

                A = VV1;
                B = (VV0 - VV1) * Dist1;
                C = (VV2 - VV1) * Dist1;
                X0 = Dist1 - Dist0;
                X1 = Dist1 - Dist2;
                break;
            }
        case 2:
            {

                A = VV2;
                B = (VV0 - VV2) * Dist2;
                C = (VV1 - VV2) * Dist2;
                X0 = Dist2 - Dist0;
                X1 = Dist2 - Dist1;
                break;
            }
        case 3:
            {
                Result.CoPlan = true;
                break;
            }
    }

    Result.V0 = A;
    Result.V1 = B;
    Result.V2 = C;
    Result.X0 = X0;
    Result.X1 = X1;

    return Result;
}



//This is supposed to say object is colliding when triangle is inside another object but doesnt work currently
CLOSEFORM CloseCheck(float TDistance[3], XMFLOAT3 WAPoint, XMFLOAT3 WBPoint, XMFLOAT3 WCPoint, XMFLOAT3 TAPoint, XMFLOAT3 TBPoint, XMFLOAT3 TCPoint, XMFLOAT3 Bone1, XMFLOAT3 TPos, XMFLOAT3 Bone2, float bdist, XMFLOAT3 bdir, XMFLOAT3 WNorm, XMFLOAT3 TNorm)
{
    CLOSEFORM Result;
    Result.dist = 0.0;
    Result.coll = true;

    float WDistance[3];
    Result.norm[0] = WNorm;
    Result.norm[1] = TNorm;

    float TScalar = XMVector3Dot(TNorm, TAPoint);
    WDistance[0] = XMVector3Dot(TNorm, AddXMFLOAT3(WAPoint, MulXMFLOAT3(bdir, -bdist))) - TScalar;
    WDistance[1] = XMVector3Dot(TNorm, AddXMFLOAT3(WBPoint, MulXMFLOAT3(bdir, -bdist))) - TScalar;
    WDistance[2] = XMVector3Dot(TNorm, AddXMFLOAT3(WCPoint, MulXMFLOAT3(bdir, -bdist))) - TScalar;

    if (WDistance[0] < 0 && WDistance[1] < 0 && WDistance[2] < 0)
    {

        XMFLOAT3 iBDir = MulXMFLOAT3(bdir, -1);
        XMFLOAT3 Bonedir = bdir;
        XMFLOAT3 Bone2 = MulXMFLOAT3(bdir, bdist);
        XMFLOAT3 WNewPointA = SubXMFLOAT3(WAPoint, MulXMFLOAT3(Bonedir, XMVector3Dot(WAPoint, Bonedir)));
        XMFLOAT3 WNewPointB = SubXMFLOAT3(WBPoint, MulXMFLOAT3(Bonedir, XMVector3Dot(WBPoint, Bonedir)));
        XMFLOAT3 WNewPointC = SubXMFLOAT3(WCPoint, MulXMFLOAT3(Bonedir, XMVector3Dot(WCPoint, Bonedir)));


        XMFLOAT3 TNewPointA = SubXMFLOAT3(TAPoint, MulXMFLOAT3(Bonedir, XMVector3Dot(TAPoint, Bonedir)));
        XMFLOAT3 TNewPointB = SubXMFLOAT3(TBPoint, MulXMFLOAT3(Bonedir, XMVector3Dot(TBPoint, Bonedir)));
        XMFLOAT3 TNewPointC = SubXMFLOAT3(TCPoint, MulXMFLOAT3(Bonedir, XMVector3Dot(TCPoint, Bonedir)));


        //Make sure triangles are actually ontop of each other
        if (CoplanCheck(Bonedir, WNewPointA, WNewPointB, WNewPointC, TNewPointA, TNewPointB, TNewPointC))
        {
            Result.dist = 0.1;

        }
        else
        {
            Result.coll = false;
        }
    }
    else
    {
        Result.coll = false;
    }


    return Result;
}


CLOSEFORM TooClose(MODEL Tri1, MODEL Tri2, XMFLOAT3 Bone1, XMFLOAT3 TBone, XMFLOAT3 TPos)
{
    CLOSEFORM Result;
    Result.dist = 0.0;
    Result.coll = true;


    XMFLOAT3 WAPoint = Tri1.vects[0].Vert;
    XMFLOAT3 WBPoint = Tri1.vects[1].Vert;
    XMFLOAT3 WCPoint = Tri1.vects[2].Vert;
    XMFLOAT3 TAPoint = AddXMFLOAT3(Tri2.vects[0].Vert, TPos);
    XMFLOAT3 TBPoint = AddXMFLOAT3(Tri2.vects[1].Vert, TPos);
    XMFLOAT3 TCPoint = AddXMFLOAT3(Tri2.vects[2].Vert, TPos);



    float TDistance[3];
    float WDistance[3];

    XMFLOAT3 WNorm = Tri1.vects[0].Norm;
    XMFLOAT3 TNorm = Tri2.vects[0].Norm;
    Result.norm[0] = WNorm;
    Result.norm[1] = TNorm;

    //Check all sides then set collision apropriately. Check if inside other object
    TDistance[0] = XMVector3Dot(WNorm, SubXMFLOAT3(TAPoint, WAPoint));
    TDistance[1] = XMVector3Dot(WNorm, SubXMFLOAT3(TBPoint, WAPoint));
    TDistance[2] = XMVector3Dot(WNorm, SubXMFLOAT3(TCPoint, WAPoint));



    if (!(Sign(TDistance[0]) == Sign(TDistance[1]) && Sign(TDistance[0]) == Sign(TDistance[2])))
    {
        WDistance[0] = XMVector3Dot(TNorm, SubXMFLOAT3(WAPoint, TAPoint));
        WDistance[1] = XMVector3Dot(TNorm, SubXMFLOAT3(WBPoint, TAPoint));
        WDistance[2] = XMVector3Dot(TNorm, SubXMFLOAT3(WCPoint, TAPoint));


        if (Sign(WDistance[0]) == Sign(WDistance[1]) && Sign(WDistance[0]) == Sign(WDistance[2]))
        {
            Result.coll = false;
            return Result;
        }


        XMFLOAT3 ISectLineDir = XMVector3Cross(WNorm, TNorm);
        int iSectdex = 0;

        float ISect0 = fabs(ISectLineDir.x);

        if (fabs(ISectLineDir.y) > ISect0) ISect0 = fabs(ISectLineDir.y), iSectdex = 1;
        if (fabs(ISectLineDir.z) > ISect0) ISect0 = fabs(ISectLineDir.z), iSectdex = 2;

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

            //figure out what this code means and fix it because its wrong.
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

            if (test0[0] > test0[1])
            {
                float temp;
                temp = test0[0];
                test0[0] = test0[1];
                test0[1] = temp;
            }

            if (test1[0] > test1[1])
            {
                float temp;
                temp = test1[0];
                test1[0] = test1[1];
                test1[1] = temp;
            }

            if (test0[1] < test1[0] || test1[1] < test0[0]) {
                Result.coll = false;
            }
            else
            {
                Result.dist = 0.1;
                return Result;
            }
        }
    }
    else if(TDistance[0] < 0 && TDistance[1] < 0 && TDistance[2] < 0)
    {

        //Result = CloseCheck(TDistance, WAPoint, WBPoint, WCPoint, TAPoint, TBPoint, TCPoint, Bone1, TPos, Bone2, bdist, bdir, WNorm, TNorm);
        Result.coll = false;
    }
    else
    {
        Result.coll = false;
    }


    return Result;
}


void kernel coll(global const UPVERTNORM* WModel, global const UPVERTNORM* TModel, global const XMFLOAT3* WBone, global const XMFLOAT3* TBone, global const INTINDEX* WbIndexBuff, global const INTINDEX* TbIndexBuff, global const int* WbIndexMap, global const int* TbIndexMap, global const WORKDATA* WData, global const int* WorkingIndices, global RETURNDATA* retdat){
int sd = get_global_id(0);
int Wind = get_global_id(1);
int Tind = get_global_id(2);



if (!retdat[sd].coll)
{
    XMFLOAT3 TPos = WData[sd].Position;

    int wOffset = WData[sd].Offset;
    int tOffset = wOffset + WData[sd].wWorkCount;

    MODEL Work = { { 0, 0, 0 }, { WModel[WbIndexBuff[WbIndexMap[WorkingIndices[wOffset + Wind]]].Index[0]], WModel[WbIndexBuff[WbIndexMap[WorkingIndices[wOffset + Wind]]].Index[1]], WModel[WbIndexBuff[WbIndexMap[WorkingIndices[wOffset + Wind]]].Index[2]] } };

    MODEL TWork = { { 0, 0, 0 }, { TModel[TbIndexBuff[TbIndexMap[WorkingIndices[tOffset + Tind]]].Index[0]], TModel[TbIndexBuff[TbIndexMap[WorkingIndices[tOffset + Tind]]].Index[1]], TModel[TbIndexBuff[TbIndexMap[WorkingIndices[tOffset + Tind]]].Index[2]] } };
    
    CLOSEFORM Result = TooClose(Work, TWork, WBone[WData[sd].bIndex[0]], TBone[WData[sd].bIndex[1]], TPos);

    if (Result.dist != 0)
    {
        retdat[sd].index1[0] = WbIndexBuff[WbIndexMap[WorkingIndices[wOffset + Wind]]].Index[0];
        retdat[sd].index1[1] = WbIndexBuff[WbIndexMap[WorkingIndices[wOffset + Wind]]].Index[1];
        retdat[sd].index1[2] = WbIndexBuff[WbIndexMap[WorkingIndices[wOffset + Wind]]].Index[2];
        retdat[sd].index2[0] = TbIndexBuff[TbIndexMap[WorkingIndices[tOffset + Tind]]].Index[0];
        retdat[sd].index2[1] = TbIndexBuff[TbIndexMap[WorkingIndices[tOffset + Tind]]].Index[1];
        retdat[sd].index2[2] = TbIndexBuff[TbIndexMap[WorkingIndices[tOffset + Tind]]].Index[2];

        retdat[sd].coll = Result.coll;
        retdat[sd].dist[0] = Result.dist;
        retdat[sd].dist[1] = 0;
        retdat[sd].dir[0] = Result.norm[0];
        retdat[sd].dir[1] = Result.norm[1];

    }
    

}



}