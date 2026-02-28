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

static XMFLOAT3 Zero = {0.0,0.0,0.0 };
//Accuracy seems to be somewhere between 0.01 and 0.001 not amazing but good enough I guess
static double epsilon = 0.005;

typedef enum
{
    X, Y, Z
}XYZ;


typedef struct CINTERVAL
{
    float Vector0Distance;
    float Vector1Distance;
    float Vector2Distance;
    float Scalar0Difference;
    float Scalar1Difference;
    bool isCoPlanar;
}
CINTERVAL;

typedef struct BONE
{
    XMFLOAT3 Position;
    int numIndices;
    int iOffset;
}
BONE;

typedef struct Vertex
{
    XMFLOAT3 normal;
    XMFLOAT3 position;
    XMFLOAT2 tc;
}
Vertex;

typedef struct TRIANGLE
{
    Vertex Vertices[3];
}
TRIANGLE;

typedef struct WORKDATA
{
    int bIndex[2];
    XMFLOAT3 Position;
}
WORKDATA;

typedef struct RETURNDATA
{
    float dist[2];
    int index[2];
}
RETURNDATA;


typedef struct CLOSEFORM
{
    float dist[2];
}
CLOSEFORM;

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
    if(dist <= 0)
    {
        return 0.0;
    }
    else
    {
        return sqrt(dist);
    }
}

double dDistance(XMFLOAT3 pos1, XMFLOAT3 pos2)
{
    XMFLOAT3 Result = SubXMFLOAT3(pos2, pos1);
    double dist = 0;
    dist += Result.x * Result.x;
    dist += Result.y * Result.y;
    dist += Result.z * Result.z;

    if (dist <= epsilon)
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
    double mag = dDistance(pos1, pos2);
    mag = mag != 0 ? 1 / mag : 0;
    Result = MulXMFLOAT3(Result, mag);
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


double areaOfTriangle(XMFLOAT3 TriA, XMFLOAT3 TriB, XMFLOAT3 TriC)
{
    XMFLOAT3 AB = XMVector3Cross(TriA, TriB);
    XMFLOAT3 AC = XMVector3Cross(TriA, TriC);
    XMFLOAT3 ABC = XMVector3Cross(AB, AC);

    double Area = dDistance(ABC, Zero)/2;
    return Area;
}

bool PointInTri(XMFLOAT3 TestPoint, XMFLOAT3 TriA, XMFLOAT3 TriB, XMFLOAT3 TriC)
{

    XMFLOAT3 Vect1 = SubXMFLOAT3(TriB, TriA);
    XMFLOAT3 Vect2 = SubXMFLOAT3(TestPoint, TriA);

    XMFLOAT3 Normal1 = fDirection(Zero, XMVector3Cross(Vect1, Vect2));
    Vect1 = SubXMFLOAT3(TriC, TriB);
    Vect2 = SubXMFLOAT3(TestPoint, TriB);
    XMFLOAT3 Normal2 = fDirection(Zero, XMVector3Cross(Vect1, Vect2));


    //Check if sum of A1, A2 and A3 is same as A
    return (fabs(XMVector3Dot(Normal1, Normal2)) > (1.0 - epsilon));
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
    //None can be all 1 or all -1. If they are the same they have to be 0
    int SignX = Sign(x);
    int SignY = Sign(y);
    int SignZ = Sign(z);
    //X:0 Y:1 Z:2
    int result = 0;
    if (SignX == SignY)
    {
        result = 2;
    }
    else if(SignX == SignZ)
    {
        result = 1;
    }else if (SignY == SignZ){
        result = 0;
    }else if (SignY <= 0)
    {
        result = 1;
    }else if(SignZ <= 0)
    {
        result = 2;
    }
    if(SignX == 0, SignY == 0, SignZ == 0 )
    {
        //Coplanar
        return 3;
    }

    return result;
}

//Dist is dist from plane. Positive means in front negative means behind. 
CINTERVAL ComputeInterval(float Vector0Vert, float Vector1Vert, float Vector2Vert, float Scalar[3])
{
    CINTERVAL Result;
    Result.isCoPlanar = false;

    float XDistance, YDistance, ZDistance, ScalarDifference0, ScalarDifference1 = 0;

    //A or X should be odd man out. YZ or BC should be on same side.

    switch (OddOneOut(Scalar[0], Scalar[1], Scalar[2]))
    {
        case 0:
            {

                XDistance = Vector0Vert;
                YDistance = (Vector1Vert - Vector0Vert) * Scalar[0];
                ZDistance = (Vector2Vert - Vector0Vert) * Scalar[0];
                ScalarDifference0 = Scalar[0] - Scalar[1];
                ScalarDifference1 = Scalar[0] - Scalar[2];
                break;
            }
        case 1:
            {

                XDistance = Vector1Vert;
                YDistance = (Vector0Vert - Vector1Vert) * Scalar[1];
                ZDistance = (Vector2Vert - Vector1Vert) * Scalar[1];
                ScalarDifference0 = Scalar[1] - Scalar[0];
                ScalarDifference1 = Scalar[1] - Scalar[2];
                break;
            }
        case 2:
            {

                XDistance = Vector2Vert;
                YDistance = (Vector0Vert - Vector2Vert) * Scalar[2];
                ZDistance = (Vector1Vert - Vector2Vert) * Scalar[2];
                ScalarDifference0 = Scalar[2] - Scalar[0];
                ScalarDifference1 = Scalar[2] - Scalar[1];
                break;
            }
        case 3:
            {
                Result.isCoPlanar = true;
                break;
            }
    }

    Result.Vector0Distance = XDistance;
    Result.Vector1Distance = YDistance;
    Result.Vector2Distance = ZDistance;
    Result.Scalar0Difference = ScalarDifference0;
    Result.Scalar1Difference = ScalarDifference1;

    return Result;
}



//This is supposed to say object is colliding when triangle is inside another object but doesnt work currently
/*
 CLOSEFORM CloseCheck(float TDistance[3], XMFLOAT3 WAPoint, XMFLOAT3 WBPoint, XMFLOAT3 WCPoint, XMFLOAT3 TAPoint, XMFLOAT3 TBPoint, XMFLOAT3 TCPoint, XMFLOAT3 Bone1, XMFLOAT3 TPos, XMFLOAT3 Bone2, float bdist, XMFLOAT3 bdir, XMFLOAT3 WNorm, XMFLOAT3 TNorm)
{
    CLOSEFORM Result;
    Result.dist[0] = 0;
    Result.dist[1] = 0;


    float WDistance[3];

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
        //if (CoplanCheck(Bonedir, WNewPointA, WNewPointB, WNewPointC, TNewPointA, TNewPointB, TNewPointC))
        {
            Result.dist[0] = 0.1;
            Result.dist[1] = 0.1;

        }

    }



    return Result;
}
 */
typedef struct XMF32
{
    XMFLOAT3 a;
    XMFLOAT3 b;
}XMF32;

XMF32 LIntersectPoint(XMFLOAT3 TriA, XMFLOAT3 TriB, XMFLOAT3 TriC, XMFLOAT3 Tri2A, XMFLOAT3 Tri2Normal){
    XMF32 Result;
    //tripoint direction b to a
    XMFLOAT3 BA = fDirection(TriB, TriA);
    //tripoint direction c to a
    XMFLOAT3 CA = fDirection(TriC, TriA);

    float t2D = XMVector3Dot(Tri2Normal, Tri2A);

    float BAd = (t2D - XMVector3Dot(Tri2Normal, TriB)) / XMVector3Dot(Tri2Normal, BA);
    float CAd = (t2D - XMVector3Dot(Tri2Normal, TriC)) / XMVector3Dot(Tri2Normal, CA);

    //Goal is to find a point along the direction BA or CA that intersects the plane of Tri2 then project that point back onto the first triangle and measure the distance between the two points
    //This isnt correct?

    Result.a = AddXMFLOAT3(TriB, MulXMFLOAT3(BA, BAd));
    Result.b = AddXMFLOAT3(TriC, MulXMFLOAT3(CA, CAd));
    return Result;
}


float TriDist(XMFLOAT3 TriA, XMFLOAT3 TriB, XMFLOAT3 TriC, XMFLOAT3 Tri2A, XMFLOAT3 Tri2B, XMFLOAT3 Tri2C, XMFLOAT3 Tri2Normal, XMFLOAT3 TriNormal, float Tri1Scalar[3], float Tri2Scalar[3])
{
    float Result = 0.1;

    //Goal is to find a point along the direction BA or CA that intersects the plane of Tri2 then project that point back onto the first triangle and measure the distance between the two points
    //This isnt correct?
    //float distBAp = ;


    //float distCAp = XMVector3Dot(SubXMFLOAT3(Tri2A, TriA), Tri2Normal) * CAd;

    //Need to have both points to figure out distance to move. Putting thoughts here to remember later

    XMF32 Tri1;
    XMF32 Tri2;
    int OddSide1 = OddOneOut(Tri1Scalar[0], Tri1Scalar[1], Tri1Scalar[2]);
    int OddSide2 = OddOneOut(Tri2Scalar[0], Tri2Scalar[1], Tri2Scalar[2]);
    float Dist1 = 0;
    float Dist2 = 0;
    XMFLOAT3 OddTri1;
    XMFLOAT3 OddTri2;


    //Check if odd one out has the shorter distance to move. Possible it doesnt
    switch (OddSide1)
    {
        case 0:
            {
                Tri1 = LIntersectPoint(TriA, TriB, TriC, Tri2A ,Tri2Normal);
                Dist1 = Tri1Scalar[0];
                OddTri1 = TriA;

                break;

            }
        case 1:
            {
                Tri1 = LIntersectPoint(TriB, TriC, Tri2A, Tri2A, Tri2Normal);

                Dist1 = Tri1Scalar[1];
                OddTri1 = TriB;

                break;
            }
        case 2:
            {
                Tri1 = LIntersectPoint(TriC, TriA, TriB, Tri2A, Tri2Normal);
                Dist1 = Tri1Scalar[2];
                OddTri1 = TriC;

                break;
            }
        case 3:
            {
                //Distance to move
                return 69;
                break;
            }

    }
    switch (OddSide2)
    {
        case 0:
            {

                Tri2 = LIntersectPoint(Tri2A, Tri2B, Tri2C, TriA, TriNormal);
                Dist2 = Tri2Scalar[0];
                OddTri2 = Tri2A;

                break;
            }
        case 1:
            {
                Tri2 = LIntersectPoint(Tri2B, Tri2C, Tri2A, TriA, TriNormal);
                Dist2 = Tri2Scalar[1];
                OddTri2 = Tri2B;

                break;
            }
        case 2:
            {
                Tri2 = LIntersectPoint(Tri2C, Tri2A, Tri2B, TriA, TriNormal);
                Dist2 = Tri2Scalar[2];
                OddTri2 = Tri2C;

                break;
            }
        case 3:
            {
                //Distance to move
                return 69;
                break;
            }

    }
      /*
    if (PointInTri(Tri1.a, Tri2A, Tri2B, Tri2C))
    {

        if (PointInTri(Tri2.a, TriA, TriB, TriC))
        {

            return 1;
        }
        else
        if (PointInTri(Tri2.b, TriA, TriB, TriC))
        {

            return 2;
        }
    }
    else
    if (PointInTri(Tri1.b, Tri2A, Tri2B, Tri2C))
    {
        if (PointInTri(Tri2.a, TriA, TriB, TriC))
        {

            return 3;
        }
        else
        if (PointInTri(Tri2.b, TriA, TriB, TriC))
        {

            return 4;
        }
    }
    else
    {

    }
      */

    //Test case odd side is negative and collides with other triangle. Move distance = distance from odd point to plane of other triangle
    if(Dist1 < 0)
    {

        if (PointInTri(AddXMFLOAT3(OddTri1, MulXMFLOAT3(Tri2Normal, sqrt(fabs(Dist1)))), Tri2A, Tri2B, Tri2C))
        {
            Result = 69;
        }
    }

    return Result;

}

CLOSEFORM TooClose(TRIANGLE Tri1, TRIANGLE Tri2, XMFLOAT3 TPos)
{
    CLOSEFORM Result;
    Result.dist[0] = 0;
    Result.dist[1] = 0;


    XMFLOAT3 WAPoint = Tri1.Vertices[0].position;
    XMFLOAT3 WBPoint = Tri1.Vertices[1].position;
    XMFLOAT3 WCPoint = Tri1.Vertices[2].position;
    XMFLOAT3 TAPoint = AddXMFLOAT3(Tri2.Vertices[0].position, TPos);
    XMFLOAT3 TBPoint = AddXMFLOAT3(Tri2.Vertices[1].position, TPos);
    XMFLOAT3 TCPoint = AddXMFLOAT3(Tri2.Vertices[2].position, TPos);


    //A:0 B:1 C:2
    float TNormalScalar[3];
    float WNormalScalar[3];

    XMFLOAT3 WNorm = Tri1.Vertices[0].normal;
    XMFLOAT3 TNorm = Tri2.Vertices[0].normal;

    //A:0 B:1 C:2 Scalar distance of T*point mapped onto plane of WNorm given WA as origin.
    //XMFLOAT3 AverageW = MulXMFLOAT3(AddXMFLOAT3(WAPoint, AddXMFLOAT3(WBPoint, WCPoint)),0.333333333333333333);
    TNormalScalar[0] = XMVector3Dot(WNorm, SubXMFLOAT3(TAPoint, WAPoint));
    TNormalScalar[1] = XMVector3Dot(WNorm, SubXMFLOAT3(TBPoint, WAPoint));
    TNormalScalar[2] = XMVector3Dot(WNorm, SubXMFLOAT3(TCPoint, WAPoint));



    if (!(Sign(TNormalScalar[0]) == Sign(TNormalScalar[1]) && Sign(TNormalScalar[0]) == Sign(TNormalScalar[2])))
    {
        //XMFLOAT3 AverageT = MulXMFLOAT3(AddXMFLOAT3(TAPoint, AddXMFLOAT3(TBPoint, TCPoint)), 0.333333333333333333);
        //A:0 B:1 C:2 Scalar distance of W*point mapped onto plane of TNorm given TA as origin.
        WNormalScalar[0] = XMVector3Dot(TNorm, SubXMFLOAT3(WAPoint, TAPoint));
        WNormalScalar[1] = XMVector3Dot(TNorm, SubXMFLOAT3(WBPoint, TAPoint));
        WNormalScalar[2] = XMVector3Dot(TNorm, SubXMFLOAT3(WCPoint, TAPoint));


        if (Sign(WNormalScalar[0]) == Sign(WNormalScalar[1]) && Sign(WNormalScalar[0]) == Sign(WNormalScalar[2]))
        {
            return Result;
        }

        //Vector describing the intersection line of the two normal planes
        XMFLOAT3 ISectLineDir = XMVector3Cross(WNorm, TNorm);


        //Finding greatest magnitude component of Intersection line(Aka biggest difference between planes)
        //X:0 Y:1 Z:2
        XYZ iSectdex = X;
        {
            float ISect0 = fabs(ISectLineDir.x);
            if (fabs(ISectLineDir.y) > ISect0){
                ISect0 = fabs(ISectLineDir.y);
                iSectdex = Y;
            }
            if (fabs(ISectLineDir.z) > ISect0) {
                iSectdex = Z;
            }
        }

        CINTERVAL cIntervalW;
        CINTERVAL cIntervalT;
        switch (iSectdex)
        {
            //X
            case X:
                {
                    cIntervalW = ComputeInterval(WAPoint.x, WBPoint.x, WCPoint.x, WNormalScalar);
                    cIntervalT = ComputeInterval(TAPoint.x, TBPoint.x, TCPoint.x, TNormalScalar);
                    break;
                }
            //Y
            case Y:
                {
                    cIntervalW = ComputeInterval(WAPoint.y, WBPoint.y, WCPoint.y, WNormalScalar);
                    cIntervalT = ComputeInterval(TAPoint.y, TBPoint.y, TCPoint.y, TNormalScalar);
                    break;
                }
            //Z
            case Z:
                {
                    cIntervalW = ComputeInterval(WAPoint.z, WBPoint.z, WCPoint.z, WNormalScalar);
                    cIntervalT = ComputeInterval(TAPoint.z, TBPoint.z, TCPoint.z, TNormalScalar);
                    break;
                }
        }

        if (!cIntervalW.isCoPlanar)
        {

            float WS0S1Product = cIntervalW.Scalar0Difference * cIntervalW.Scalar1Difference;
            float TS0S1Product = cIntervalT.Scalar0Difference * cIntervalT.Scalar1Difference;
            float TWScalarpProduct = WS0S1Product * TS0S1Product;


            float W0Size = cIntervalW.Vector0Distance * TWScalarpProduct;
            float W1Size = W0Size + cIntervalW.Vector1Distance * cIntervalW.Scalar1Difference * TS0S1Product;
            float W2Size = W0Size + cIntervalW.Vector2Distance * cIntervalW.Scalar0Difference * TS0S1Product;

            float T0Size = cIntervalT.Vector0Distance * TWScalarpProduct;
            float T1Size = T0Size + cIntervalT.Vector1Distance * WS0S1Product * cIntervalT.Scalar1Difference;
            float T2Size = T0Size + cIntervalT.Vector2Distance * WS0S1Product * cIntervalT.Scalar0Difference;


            float LeastToMostW[2];
            float LeastToMostT[2];

            LeastToMostW[0] = W1Size;
            LeastToMostW[1] = W2Size;
            LeastToMostT[0] = T1Size;
            LeastToMostT[1] = T2Size;

            if (LeastToMostW[0] > LeastToMostW[1])
            {

                float temp;
                temp = LeastToMostW[0];
                LeastToMostW[0] = LeastToMostW[1];
                LeastToMostW[1] = temp;
            }

            if (LeastToMostT[0] > LeastToMostT[1])
            {

                float temp;
                temp = LeastToMostT[0];
                LeastToMostT[0] = LeastToMostT[1];
                LeastToMostT[1] = temp;
            }
            if (LeastToMostW[1] < LeastToMostT[0] || LeastToMostT[1] < LeastToMostW[0])
            {

                return Result;
            }
            //TriDist(WAPoint, WBPoint, WCPoint, TAPoint, TBPoint, TCPoint, TNorm, WNorm, WNormalScalar, TNormalScalar)
            Result.dist[0] = 0.01;
            Result.dist[1] = 0.01;

            // Project triangle vertices onto intersection line direction


        }
        else
        {             
            //Tris are coplanar so move them accordingly if theyre intersecting
            //Distance to move W
            //Result.dist[0] = 0.001;
            //Distance to move T
            //Result.dist[1] = 0.001;
        }

    }
    //else if(TNormalScalar[0] < 0 && TNormalScalar[1] < 0 && TNormalScalar[2] < 0)
    {
        //Result.dist[0] = 0.1;
        //Result.dist[1] = 0.1;
        //Result = CloseCheck(TDistance, WAPoint, WBPoint, WCPoint, TAPoint, TBPoint, TCPoint, Bone1, TPos, Bone2, bdist, bdir, WNorm, TNorm);
    }

    return Result;
}


void kernel coll(global const TRIANGLE* WModel, global const TRIANGLE* TModel, global const XMFLOAT3* TPos, global const int* WorkingIndices, global RETURNDATA* retdat){
int Wind = get_global_id(0);
int Tind = get_global_id(1);
int rd = Wind;

    if (!(retdat[rd].dist[0] != 0 || retdat[rd].dist[1] != 0))
    {
        CLOSEFORM Result = TooClose(WModel[WorkingIndices[Wind]], TModel[WorkingIndices[Tind]], TPos[0]);

        if (Result.dist[0] != 0 || Result.dist[1] != 0)
        {
            retdat[rd].index[0] = WorkingIndices[Wind];
            retdat[rd].index[1] = WorkingIndices[Tind];
            retdat[rd].dist[0] = Result.dist[0];
            retdat[rd].dist[1] = Result.dist[1];

        }
    }
}
