#include <atomic>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <sstream>

// Need to make non windows version of dynamic library exporting
#ifndef DLL
#ifdef DESCDLL
#define DLL __declspec(dllexport)
#else
#define DLL __declspec(dllimport)
#endif
#endif

#ifndef _COMMONDSE
#define _COMMONDSE
constexpr double degrees90 = std::numbers::pi_v<double>*0.5;
#define _DEGREES90 degrees90

//Current path to find files etc
#define _CURRENTPATH std::filesystem::current_path().parent_path().parent_path()

// Unique Object ID
typedef uint64_t UOID;


struct alignas(8) FLOAT2 {

  float x;
  float y;

  FLOAT2() = default;
  inline FLOAT2(float x, float y) : x(x), y(y) {};
  FLOAT2 operator+(const FLOAT2 &a) const { return {a.x + x, a.y + y}; };
  FLOAT2 operator-(const FLOAT2 &a) const { return {x - a.x, y - a.y }; };
  FLOAT2 operator/(const float &a) const { auto div = 1/a; return { x * div, y * div}; };
  FLOAT2 operator*(const FLOAT2 &a) const {
    float result = 0;
    result += a.x * x;
    result += a.y * y;
    return {result, result};
  };
};
struct alignas(16) FLOAT3 {

  float x;
  float y;
  float z;

  FLOAT3() = default;
  inline FLOAT3(float x, float y, float z) : x(x), y(y), z(z) {};
  FLOAT3 operator+(const FLOAT3 &a) const {
    return {a.x + x, a.y + y, a.z + z};
  };
  FLOAT3 operator-(const FLOAT3 &a) const {
    return { x - a.x,  y - a.y, z - a.z};
  };
  FLOAT3 operator/(const float &a) const { return {x / a, y / a, z / a}; };
  float operator*(const FLOAT3 &a) const {
    float result = 0;
    result += a.x * x;
    result += a.y * y;
    result += a.z * z;
    return result;
  };

  FLOAT3 operator*(const float &a) const {
    FLOAT3 result;
    result.x = a * x;
    result.y = a * y;
    result.z = a * z;
    return result;
  };
  inline FLOAT3 Normal()const{
    auto& tf = *this;
    float dp = tf * tf;
    dp = dp > 0 ? 1.f / sqrtf(dp) : dp;
    return tf * dp;
  };
  FLOAT3 CrossP(const FLOAT3 &Other) const {
    FLOAT3 result;

    result.x = y * Other.z - z * Other.y;
    result.y = z * Other.x - x * Other.z;
    result.z = x * Other.y - y * Other.x;

    return result;
  }
};

struct alignas(16) FLOAT4 {
  float x;
  float y;
  float z;
  float w;

  FLOAT4() = default;
  FLOAT4(const FLOAT3 &fl3) : x(fl3.x), y(fl3.y), z(fl3.z), w(0) {};
  inline FLOAT4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {};
  FLOAT4 operator+(const FLOAT4 &a) const {
    return {a.x + x, a.y + y, a.z + z, a.w + w};
  };
  FLOAT4 operator-(const FLOAT4 &a) const {
    return {x - a.x, y - a.y, z - a.z, w - a.w};
  };
  inline FLOAT4 operator/(const double &a) const {
    double div = 1.f/a;
    return *this * div;
  };
  bool operator==(const FLOAT4& other)const{
    return other.x == x && other.y == y && other.z == z && other.w == w;
  };
  bool operator!=(const FLOAT4 other)const{
    return (other.x != x || other.y != y || other.z != z || other.w != w);
  }
  // dot product
  inline float operator*(const FLOAT4 &a) const {
    float result = 0;
    result += a.x * x;
    result += a.y * y;
    result += a.z * z;
    result += a.w * w;
    return result;
  };
  inline FLOAT4 operator*(const double &a) const {
    FLOAT4 result;
    result.x = a * x;
    result.y = a * y;
    result.z = a * z;
    result.w = a * w;
    return result;
  };
  //Cross product is 3Dimensional ONLY
  FLOAT4 CrossP(const FLOAT4 &Other) const {
    FLOAT4 result = {0,0,0,0};
    result.x = y * Other.z - z * Other.y;
    result.y = z * Other.x - x * Other.z;
    result.z = x * Other.y - y * Other.x;
    return result;
  }
  inline FLOAT4 Normal()const{
    auto& tf = *this;
    float dp = tf * tf;
    dp = dp > 0 ? 1.f / sqrtf(dp) : dp;
    return tf * dp;
  };
  FLOAT4 Conjugate() const{
    return {-x, -y, -z, w};
  }
  FLOAT4 QuaternionMul(const FLOAT4& other) const{
    FLOAT4 Result;
    Result.x = (other.w * x) + (other.x * w) + (other.y * z) - (other.z * y);
    Result.y = (other.w * y) - (other.x * z) + (other.y * w) + (other.z * x);
    Result.z = (other.w * z) + (other.x * y) - (other.y * x) + (other.z * w);
    Result.w = (other.w * w) - (other.x * x) - (other.y * y) - (other.z * z);
    return Result;
  };
  //Angle in radians
  static FLOAT4 RotateQuaternion(const FLOAT4& Axis, const float& Angle){
    auto angl = Angle*0.5;
    float sin = sinf(angl);
    float cos = cosf(angl);
    return FLOAT4{Axis.x * sin, Axis.y * sin, Axis.z * sin, 1.f * cos};
  }
  inline FLOAT4 RotateByQuaternion(const FLOAT4& Quaternion) const{
    auto rot = Quaternion.QuaternionMul(*this);
    return rot.QuaternionMul(Quaternion.Conjugate());
  }
  inline FLOAT4 Real()const{
    return {x, y, z, 0};
  };
};

struct alignas(16) FLOAT2X2 {

  FLOAT2 a;
  FLOAT2 b;

  FLOAT2X2() = default;
  FLOAT2X2(const FLOAT2X2 &old) : a(old.a), b(old.b) {};
  FLOAT2X2(FLOAT2 a, FLOAT2 b, FLOAT2 c) : a(a), b(b) {};
  FLOAT2X2(float a0, float b0, float c0, float d0, float a1, float b1, float c1,
           float d1)
      : a({
            a0,
            b0,
        }),
        b({
            a1,
            b1,
        }) {};
  // dot product of matrix
  FLOAT2X2 operator*(const FLOAT2X2 &other) const {
    FLOAT2X2 result;
    // columns
    FLOAT2 zero = {other.a.x, other.b.x};
    FLOAT2 one = {other.a.y, other.b.y};
    // dot products
    result.a = a * zero;
    result.b = b * one;
    return result;
  };
  float Determinant() { return (a.x * b.y) - (a.y * b.x); };
};

inline float CofactorHelp(FLOAT3 a, FLOAT3 b, uint8_t ColumnIgnore) {
  FLOAT2X2 result;
  switch (ColumnIgnore) {
  case (0):
    result.a = {a.y, a.z};
    result.b = {b.y, b.z};
    break;
  case (1):
    result.a = {a.x, a.z};
    result.b = {b.x, b.z};
    break;
  case (2):
    result.a = {a.x, a.y};
    result.b = {b.x, b.y};
    break;
  }
  return result.Determinant();
};

struct FLOAT3X3 {

  FLOAT3 a;
  FLOAT3 b;
  FLOAT3 c;

  FLOAT3X3() = default;
  FLOAT3X3(const FLOAT3X3 &old) : a(old.a), b(old.b), c(old.c) {};
  FLOAT3X3(FLOAT3 a, FLOAT3 b, FLOAT3 c) : a(a), b(b), c(c) {};
  FLOAT3X3(float a0, float b0, float c0, float a1, float b1, float c1, float a2,
           float b2, float c2)
      : a({a0, b0, c0}), b({a1, b1, c1}), c({a2, b2, c2}) {};
  // dot product of matrix
  FLOAT3X3 operator*(const FLOAT3X3 &other) const {
    FLOAT3X3 result;
    // columns
    FLOAT3X3 TOther = other.Transpose();
    // dot products
    result.a = {a * TOther.a,a * TOther.b,a * TOther.c};
    result.b = {b * TOther.a,b * TOther.b,b * TOther.c};
    result.c = {c * TOther.a,c * TOther.b,c * TOther.c};
    return result;
  };
  inline FLOAT3X3 Transpose() const{
    FLOAT3 na = {a.x, b.x, c.x};
    FLOAT3 nb = {a.y, b.y, c.y};
    FLOAT3 nc = {a.z, b.z, c.z};
    return FLOAT3X3(na, nb, nc);
  };
  float Determinant() {
    float xd = CofactorHelp(b, c, 0) * a.x;
    float yd = CofactorHelp(b, c, 1) * a.y;
    float zd = CofactorHelp(b, c, 2) * a.z;
    return xd - yd + zd;
  };
};

inline float CofactorHelp(FLOAT4 a, FLOAT4 b, FLOAT4 c, uint8_t ColumnIgnore) {
  FLOAT3X3 result;
  switch (ColumnIgnore) {
  case (0):
    result.a = {a.y, a.z, a.w};
    result.b = {b.y, b.z, b.w};
    result.c = {c.y, c.z, c.w};
    break;
  case (1):
    result.a = {a.x, a.z, a.w};
    result.b = {b.x, b.z, b.w};
    result.c = {c.x, c.z, c.w};
    break;
  case (2):
    result.a = {a.x, a.y, a.w};
    result.b = {b.x, b.y, b.w};
    result.c = {c.x, c.y, c.w};
    break;
  case (3):
    result.a = {a.x, a.y, a.z};
    result.b = {b.x, b.y, b.z};
    result.c = {c.x, c.y, c.z};
    break;
  }
  return result.Determinant();
};


struct FLOAT4X4 {

  FLOAT4 a;
  FLOAT4 b;
  FLOAT4 c;
  FLOAT4 d;

  FLOAT4X4() = default;
  FLOAT4X4(const FLOAT4X4 &old) = default;
  FLOAT4X4(FLOAT4 a, FLOAT4 b, FLOAT4 c, FLOAT4 d) : a(a), b(b), c(c), d(d) {};
  FLOAT4X4(float a0, float b0, float c0, float d0, float a1, float b1, float c1,
           float d1, float a2, float b2, float c2, float d2, float a3, float b3,
           float c3, float d3)
      : a({a0, b0, c0, d0}), b({a1, b1, c1, d1}), c({a2, b2, c2, d2}),
        d({a3, b3, c3, d3}) {};

  // dot product of matrix
  FLOAT4X4 operator*(const FLOAT4X4 &other) const {
    FLOAT4X4 result;
    // columns
    FLOAT4X4 TOther = other.Transpose();
    // dot products
    result.a = {a * TOther.a,a * TOther.b,a * TOther.c,a * TOther.d};
    result.b = {b * TOther.a,b * TOther.b,b * TOther.c,b * TOther.d};
    result.c = {c * TOther.a,c * TOther.b,c * TOther.c,c * TOther.d};
    result.d = {d * TOther.a,d * TOther.b,d * TOther.c,d * TOther.d};
    return result;
  };
  FLOAT4X4 operator*(const float &other) const {
    FLOAT4X4 result;
    result.a = a * other;
    result.b = b * other;
    result.c = c * other;
    result.d = d * other;
    return result;
  };
  FLOAT4X4 operator/(const float &other) const {
    FLOAT4X4 result;
    auto inverse = 1 / other;
    result.a = a * inverse;
    result.b = b * inverse;
    result.c = c * inverse;
    result.d = d * inverse;
    return result;
  };
  float Determinant() const {
    float xd = CofactorHelp(b, c, d, 0) * a.x;
    float yd = CofactorHelp(b, c, d, 1) * a.y;
    float zd = CofactorHelp(b, c, d, 2) * a.z;
    float wd = CofactorHelp(b, c, d, 3) * a.w;
    return xd - yd + zd - wd;
  };
  static inline FLOAT4X4 Rotation(const FLOAT4& Rotation){
    FLOAT4X4 result;
    float xx, yy, zz;
    xx = Rotation.x * Rotation.x;
    yy = Rotation.y * Rotation.y;
    zz = Rotation.z * Rotation.z;

    result.a = {1.f - 2.f * yy - 2.f * zz,
                2.f * Rotation.x * Rotation.y + 2.f * Rotation.z * Rotation.w,
                2.f * Rotation.x * Rotation.z - 2.f * Rotation.y * Rotation.w,
                0.f};
    result.b = {2.f * Rotation.x * Rotation.y - 2.f * Rotation.z * Rotation.w,
                1.f - 2.f * xx - 2.f * zz,
                2.f * Rotation.y * Rotation.z + 2.f * Rotation.x * Rotation.w,
                0.f};
    result.c = {2.f * Rotation.x * Rotation.z + 2.f * Rotation.y * Rotation.w,
                2.f * Rotation.y * Rotation.z - 2.f * Rotation.x * Rotation.w,
                1.f - 2.f * xx - 2.f * yy,
                Rotation.z};
    result.d = {0.f,
                0.f,
                0.f,
                1.0f};
    return result;
  }
  static inline FLOAT4X4 Translation(const FLOAT3& translate){
    FLOAT4X4 result;
    result.a = {1,0,0,translate.x};
    result.b = {0,1,0,translate.y};
    result.c = {0,0,1,translate.z};
    result.d = {0,0, 0, 1.0f};
    return result;
  }
  inline FLOAT4X4 Transpose() const{
    FLOAT4 na = {a.x, b.x, c.x, d.x};
    FLOAT4 nb = {a.y, b.y, c.y, d.y};
    FLOAT4 nc = {a.z, b.z, c.z, d.z};
    FLOAT4 nd = {a.w, b.w, c.w, d.w};
    return FLOAT4X4(na, nb, nc, nd);
  };


  FLOAT4X4 Cofactor() const {
    FLOAT4 Cofacta = {CofactorHelp(b, c, d, 0), -CofactorHelp(b, c, d, 1),
                      CofactorHelp(b, c, d, 2), -CofactorHelp(b, c, d, 3)};
    FLOAT4 Cofactb = {-CofactorHelp(a, c, d, 0), CofactorHelp(a, c, d, 1),
                      -CofactorHelp(a, c, d, 2), CofactorHelp(a, c, d, 3)};
    FLOAT4 Cofactc = {CofactorHelp(a, b, d, 0), -CofactorHelp(a, b, d, 1),
                      CofactorHelp(a, b, d, 2), -CofactorHelp(a, b, d, 3)};
    FLOAT4 Cofactd = {-CofactorHelp(a, b, c, 0), CofactorHelp(a, b, c, 1),
                      -CofactorHelp(a, b, c, 2), CofactorHelp(a, b, c, 3)};
    return FLOAT4X4(Cofacta, Cofactb, Cofactc, Cofactd);
  };

  FLOAT4X4 Inverse() const {
    float determinant = Determinant();
    if (determinant == 0) {
      FLOAT4 Nanstruc = {NAN, NAN, NAN, NAN};
      return FLOAT4X4(Nanstruc, Nanstruc, Nanstruc, Nanstruc);
    } else {
      return Cofactor().Transpose() / determinant;
    }
  };
  bool operator==(const FLOAT4X4& other)const{
    return other.a == a && other.b == b && other.c == c && other.d == d;
  };
  // Only accepts normal vectors
  
};

struct CBVData {
  FLOAT4X4 cbvMatrix;
  uint32_t Texture = 0;
  // do not use
  // UINT Padding[3];
};


// transformation ???
inline FLOAT4 operator*(const FLOAT4 &b, const FLOAT4X4 &a) {
  FLOAT4 result;
  result = (FLOAT4{a.a.x, a.b.x, a.c.x, a.d.x} * b.x);
  result = result + (FLOAT4{a.a.y, a.b.y, a.c.y, a.d.y} * b.y);
  result = result + (FLOAT4{a.a.z, a.b.z, a.c.z, a.d.z} * b.z);
  result = result + (FLOAT4{a.a.w, a.b.w, a.c.w, a.d.w} * b.w);
  return result;
};


static FLOAT4X4 LookTo(const FLOAT4 &EyePos, const FLOAT4 &EyeDir,
                         const FLOAT4 &UpDir) {
    FLOAT4X4 Result;
    // Impliment crossproduct
    FLOAT4 CrosspUpDirEyeDir = UpDir.CrossP(EyeDir).Normal();
    FLOAT4 CrossEyeDirCrossP = (EyeDir).CrossP(CrosspUpDirEyeDir);
    float DotP0 = CrosspUpDirEyeDir * EyePos;
    float DotP1 = CrossEyeDirCrossP * EyePos;
    float DotP2 =  (EyeDir) * EyePos;
    Result.a = CrosspUpDirEyeDir;
    Result.a.w = DotP0;
    Result.b = CrossEyeDirCrossP;
    Result.b.w = DotP1;
    Result.c = EyeDir;
    Result.c.w = DotP2;
    Result.d = {0, 0, 0, 1};
    return Result.Transpose();
  };

//Add intersection detection etc etc
struct SphereCollider {
  FLOAT3 Center{0, 0, 0};
  float Radius = 1.f;
};

struct WRect {
  struct WindowRect {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
  };
  std::atomic<bool> Updated;
  WindowRect wr;
  std::mutex Mtx;
};



static inline float fDistance(FLOAT3& pos1, FLOAT3& pos2) {
    FLOAT3 Result{ 0,0,0};
    Result = pos2 - pos1;
    return sqrt(Result * Result);
}

static inline FLOAT3 fDirection(FLOAT3& pos1, FLOAT3& pos2) {
    FLOAT3 Result{ 0,0,0 };
    Result = pos2 - pos1;
    float dir = Result * Result;
    if (dir != 0) {
        return Result / sqrt(dir);
    }
    return { 0,0,0};
}

inline FLOAT4X4 strToMatrix(std::istringstream &rawmatri) {
  FLOAT4X4 float4x4;
  float x, y, z, w;
  int caser = 0;
  while (rawmatri >> x >> y >> z >> w) {
    switch (caser) {
    case (0):
      float4x4.a = {x, y, z, w};
      break;
    case (1):
      float4x4.b = {x, y, z, w};
      break;
    case (2):
      float4x4.c = {x, y, z, w};
      break;
    case (3):
      float4x4.d = {x, y, z, w};
      break;
    }

    caser++;
  }
  return float4x4;
}




#endif