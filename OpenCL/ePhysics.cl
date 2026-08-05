
__constant static float3 Zero = {0.0,0.0,0.0};
__constant static double epsilon = 0.005;

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
    float3 Position;
    unsigned int numIndices;
    unsigned int iOffset;
}
BONE;

typedef struct Vertex
{
    float3 normal;
    float3 position;
    float2 tc;
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
    float3 Position;
}
WORKDATA;

typedef struct RETURNDATA
{
    float dist[2];
    unsigned int index[2];
}
RETURNDATA;


typedef struct CLOSEFORM
{
    float dist[2];
}
CLOSEFORM;

float3 Mulfloat3(float3 a, float b)
{
    float3 result = { 0, 0, 0 };

    result.x = a.x * b;
    result.y = a.y * b;
    result.z = a.z * b;

    return result;
}

float3 Subfloat3(float3 a, float3 b)
{
    float3 result = { 0, 0, 0 };

    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;

    return result;
}

float XMVector3Dot(float3 a, float3 b)
{
    float result = 0;
    result += a.x * b.x;
    result += a.y * b.y;
    result += a.z * b.z;
    return result;
}

float fDistance(float3 pos1, float3 pos2)
{
    float3 Result = Subfloat3(pos2, pos1);
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

double dDistance(float3 pos1, float3 pos2)
{
    float3 Result = Subfloat3(pos2, pos1);
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

float3 fDirection(float3 pos1, float3 pos2) {
    float3 Result = Subfloat3(pos2, pos1);
    double mag = dDistance(pos1, pos2);
    mag = mag != 0 ? 1 / mag : 0;
    Result = Mulfloat3(Result, mag);
    return Result;
}

float3 Addfloat3(float3 a, float3 b)
{
    float3 result = { 0, 0, 0 };

    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;

    return result;
}

float3 XMVector3Cross(float3 a, float3 b)
{
    float3 result = { 0, 0, 0 };

    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;

    return result;
}


double areaOfTriangle(float3 TriA, float3 TriB, float3 TriC)
{
    float3 AB = XMVector3Cross(TriA, TriB);
    float3 AC = XMVector3Cross(TriA, TriC);
    float3 ABC = XMVector3Cross(AB, AC);

    double Area = dDistance(ABC, Zero)/2;
    return Area;
}

bool PointInTri(float3 TestPoint, float3 TriA, float3 TriB, float3 TriC)
{

    float3 Vect1 = Subfloat3(TriB, TriA);
    float3 Vect2 = Subfloat3(TestPoint, TriA);

    float3 Normal1 = fDirection(Zero, XMVector3Cross(Vect1, Vect2));
    Vect1 = Subfloat3(TriC, TriB);
    Vect2 = Subfloat3(TestPoint, TriB);
    float3 Normal2 = fDirection(Zero, XMVector3Cross(Vect1, Vect2));


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
 CLOSEFORM CloseCheck(float TDistance[3], float3 WAPoint, float3 WBPoint, float3 WCPoint, float3 TAPoint, float3 TBPoint, float3 TCPoint, float3 Bone1, float3 TPos, float3 Bone2, float bdist, float3 bdir, float3 WNorm, float3 TNorm)
{
    CLOSEFORM Result;
    Result.dist[0] = 0;
    Result.dist[1] = 0;


    float WDistance[3];

    float TScalar = XMVector3Dot(TNorm, TAPoint);
    WDistance[0] = XMVector3Dot(TNorm, Addfloat3(WAPoint, Mulfloat3(bdir, -bdist))) - TScalar;
    WDistance[1] = XMVector3Dot(TNorm, Addfloat3(WBPoint, Mulfloat3(bdir, -bdist))) - TScalar;
    WDistance[2] = XMVector3Dot(TNorm, Addfloat3(WCPoint, Mulfloat3(bdir, -bdist))) - TScalar;

    if (WDistance[0] < 0 && WDistance[1] < 0 && WDistance[2] < 0)
    {

        float3 iBDir = Mulfloat3(bdir, -1);
        float3 Bonedir = bdir;
        float3 Bone2 = Mulfloat3(bdir, bdist);
        float3 WNewPointA = Subfloat3(WAPoint, Mulfloat3(Bonedir, XMVector3Dot(WAPoint, Bonedir)));
        float3 WNewPointB = Subfloat3(WBPoint, Mulfloat3(Bonedir, XMVector3Dot(WBPoint, Bonedir)));
        float3 WNewPointC = Subfloat3(WCPoint, Mulfloat3(Bonedir, XMVector3Dot(WCPoint, Bonedir)));


        float3 TNewPointA = Subfloat3(TAPoint, Mulfloat3(Bonedir, XMVector3Dot(TAPoint, Bonedir)));
        float3 TNewPointB = Subfloat3(TBPoint, Mulfloat3(Bonedir, XMVector3Dot(TBPoint, Bonedir)));
        float3 TNewPointC = Subfloat3(TCPoint, Mulfloat3(Bonedir, XMVector3Dot(TCPoint, Bonedir)));


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
    float3 a;
    float3 b;
}XMF32;

XMF32 LIntersectPoint(float3 TriA, float3 TriB, float3 TriC, float3 Tri2A, float3 Tri2Normal){
    XMF32 Result;
    //tripoint direction b to a
    float3 BA = fDirection(TriB, TriA);
    //tripoint direction c to a
    float3 CA = fDirection(TriC, TriA);

    float t2D = XMVector3Dot(Tri2Normal, Tri2A);

    float BAd = (t2D - XMVector3Dot(Tri2Normal, TriB)) / XMVector3Dot(Tri2Normal, BA);
    float CAd = (t2D - XMVector3Dot(Tri2Normal, TriC)) / XMVector3Dot(Tri2Normal, CA);

    //Goal is to find a point along the direction BA or CA that intersects the plane of Tri2 then project that point back onto the first triangle and measure the distance between the two points
    //This isnt correct?

    Result.a = Addfloat3(TriB, Mulfloat3(BA, BAd));
    Result.b = Addfloat3(TriC, Mulfloat3(CA, CAd));
    return Result;
}


float TriDist(float3 TriA, float3 TriB, float3 TriC, float3 Tri2A, float3 Tri2B, float3 Tri2C, float3 Tri2Normal, float3 TriNormal, float Tri1Scalar[3], float Tri2Scalar[3])
{
    float Result = 0.1;

    //Goal is to find a point along the direction BA or CA that intersects the plane of Tri2 then project that point back onto the first triangle and measure the distance between the two points
    //This isnt correct?
    //float distBAp = ;


    //float distCAp = XMVector3Dot(Subfloat3(Tri2A, TriA), Tri2Normal) * CAd;

    //Need to have both points to figure out distance to move. Putting thoughts here to remember later

    XMF32 Tri1;
    XMF32 Tri2;
    int OddSide1 = OddOneOut(Tri1Scalar[0], Tri1Scalar[1], Tri1Scalar[2]);
    int OddSide2 = OddOneOut(Tri2Scalar[0], Tri2Scalar[1], Tri2Scalar[2]);
    float Dist1 = 0;
    float Dist2 = 0;
    float3 OddTri1;
    float3 OddTri2;


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

        if (PointInTri(Addfloat3(OddTri1, Mulfloat3(Tri2Normal, sqrt(fabs(Dist1)))), Tri2A, Tri2B, Tri2C))
        {
            Result = 69;
        }
    }

    return Result;

}

CLOSEFORM TooClose(TRIANGLE Tri1, TRIANGLE Tri2, float3 TPos)
{
    CLOSEFORM Result;
    Result.dist[0] = 0;
    Result.dist[1] = 0;


    float3 WAPoint = Tri1.Vertices[0].position;
    float3 WBPoint = Tri1.Vertices[1].position;
    float3 WCPoint = Tri1.Vertices[2].position;
    float3 TAPoint = Addfloat3(Tri2.Vertices[0].position, TPos);
    float3 TBPoint = Addfloat3(Tri2.Vertices[1].position, TPos);
    float3 TCPoint = Addfloat3(Tri2.Vertices[2].position, TPos);


    //A:0 B:1 C:2
    float TNormalScalar[3];
    float WNormalScalar[3];

    float3 WNorm = Tri1.Vertices[0].normal;
    float3 TNorm = Tri2.Vertices[0].normal;

    //A:0 B:1 C:2 Scalar distance of T*point mapped onto plane of WNorm given WA as origin.
    //float3 AverageW = Mulfloat3(Addfloat3(WAPoint, Addfloat3(WBPoint, WCPoint)),0.333333333333333333);
    TNormalScalar[0] = XMVector3Dot(WNorm, Subfloat3(TAPoint, WAPoint));
    TNormalScalar[1] = XMVector3Dot(WNorm, Subfloat3(TBPoint, WAPoint));
    TNormalScalar[2] = XMVector3Dot(WNorm, Subfloat3(TCPoint, WAPoint));



    if (!(Sign(TNormalScalar[0]) == Sign(TNormalScalar[1]) && Sign(TNormalScalar[0]) == Sign(TNormalScalar[2])))
    {
        //float3 AverageT = Mulfloat3(Addfloat3(TAPoint, Addfloat3(TBPoint, TCPoint)), 0.333333333333333333);
        //A:0 B:1 C:2 Scalar distance of W*point mapped onto plane of TNorm given TA as origin.
        WNormalScalar[0] = XMVector3Dot(TNorm, Subfloat3(WAPoint, TAPoint));
        WNormalScalar[1] = XMVector3Dot(TNorm, Subfloat3(WBPoint, TAPoint));
        WNormalScalar[2] = XMVector3Dot(TNorm, Subfloat3(WCPoint, TAPoint));


        if (Sign(WNormalScalar[0]) == Sign(WNormalScalar[1]) && Sign(WNormalScalar[0]) == Sign(WNormalScalar[2]))
        {
            return Result;
        }

        //Vector describing the intersection line of the two normal planes
        float3 ISectLineDir = XMVector3Cross(WNorm, TNorm);


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


void kernel coll(global const TRIANGLE* WModel, global const TRIANGLE* TModel, global const float3* TPos, global const unsigned int* WorkingIndices, global RETURNDATA* retdat){
unsigned int Wind = get_global_id(0);
unsigned int Tind = get_global_id(1);
unsigned int rd = Wind;

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
