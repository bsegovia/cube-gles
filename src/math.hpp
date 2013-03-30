#ifndef __CUBE_MATH_HPP__
#define __CUBE_MATH_HPP__

#include "tools.h"
#include <cassert>
#include <cmath>
#include <cfloat>

namespace cube {

/* polymorphic constant values */
#define CONSTANT_TYPE(TYPE,VALUE,NUM)\
static const struct TYPE\
{\
  INLINE TYPE(){}\
  INLINE operator double(void) const { return NUM; }\
  INLINE operator float (void) const { return NUM; }\
  INLINE operator int64 (void) const { return NUM; }\
  INLINE operator uint64(void) const { return NUM; }\
  INLINE operator int32 (void) const { return NUM; }\
  INLINE operator uint32(void) const { return NUM; }\
  INLINE operator int16 (void) const { return NUM; }\
  INLINE operator uint16(void) const { return NUM; }\
  INLINE operator int8  (void) const { return NUM; }\
  INLINE operator uint8 (void) const { return NUM; }\
} VALUE;
CONSTANT_TYPE(zerotype,zero,0);
CONSTANT_TYPE(onetype,one,1);
CONSTANT_TYPE(twotype,two,2);
#undef CONSTANT_TYPE

/* make the code terser */
#define op operator
#define v2 vec2<T>
#define v3 vec3<T>
#define v4 vec4<T>
#define m33 mat3x3<T>
#define m43 mat4x3<T>
#define m44 mat4x4<T>
#define v2arg const vec2<T>&
#define v3arg const vec3<T>&
#define v4arg const vec4<T>&
#define m33arg const mat3x3<T>&
#define m43arg const mat4x3<T>&
#define m44arg const mat4x4<T>&
#define TINLINE template <typename T> INLINE
#define UINLINE template <typename U> INLINE

/* all useful math functions */
INLINE int abs    (int x)   {return ::abs(x);}
INLINE float sign (float x) {return x<0?-1.0f:1.0f;}
INLINE float rcp  (float x) {return 1.0f/x;}
INLINE float rsqrt(float x) {return 1.0f/::sqrtf(x);}
INLINE float abs  (float x) {return ::fabsf(x);}
INLINE float acos (float x) {return ::acosf (x);}
INLINE float asin (float x) {return ::asinf (x);}
INLINE float atan (float x) {return ::atanf (x);}
INLINE float cos  (float x) {return ::cosf  (x);}
INLINE float cosh (float x) {return ::coshf (x);}
INLINE float exp  (float x) {return ::expf  (x);}
INLINE float log  (float x) {return ::logf  (x);}
INLINE float log2 (float x) {return ::log(x) / 0.69314718055994530941723212145818f;}
INLINE float log10(float x) {return ::log10f(x);}
INLINE float sin  (float x) {return ::sinf  (x);}
INLINE float sinh (float x) {return ::sinhf (x);}
INLINE float sqrt (float x) {return ::sqrtf (x);}
INLINE float tan  (float x) {return ::tanf  (x);}
INLINE float tanh (float x) {return ::tanhf (x);}
INLINE float floor(float x) {return ::floorf (x);}
INLINE float ceil (float x) {return ::ceilf (x);}
INLINE float atan2(float y, float x) {return ::atan2f(y, x);}
INLINE float fmod (float x, float y) {return ::fmodf (x, y);}
INLINE float pow  (float x, float y) {return ::powf  (x, y);}
INLINE double abs  (double x) {return ::fabs(x);}
INLINE double sign (double x) {return x<0?-1.0:1.0;}
INLINE double log2 (double x) {return ::log(x) / 0.69314718055994530941723212145818;}
INLINE double log10(double x) {return ::log10(x);}
INLINE double rcp  (double x) {return 1.0/x;}
INLINE double rsqrt(double x) {return 1.0/::sqrt(x);}
TINLINE T max (T a, T b) {return a<b? b:a;}
TINLINE T min (T a, T b) {return a<b? a:b;}
TINLINE T min (T a, T b, T c) {return min(min(a,b),c);}
TINLINE T max (T a, T b, T c) {return max(max(a,b),c);}
TINLINE T max (T a, T b, T c, T d) {return max(max(a,b),max(c,d));}
TINLINE T min (T a, T b, T c, T d) {return min(min(a,b),min(c,d));}
TINLINE T min (T a, T b, T c, T d, T e) {return min(min(min(a,b),min(c,d)),e);}
TINLINE T max (T a, T b, T c, T d, T e) {return max(max(max(a,b),max(c,d)),e);}
TINLINE T clamp (T x, T lower = T(zero), T upper = T(one)) {return max(lower, min(x,upper));}
TINLINE T deg2rad (T x) {return x * T(1.74532925199432957692e-2f);}
TINLINE T rad2deg (T x) {return x * T(5.72957795130823208768e1f);}
TINLINE T sin2cos (T x) {return sqrt(max(T(zero),T(one)-x*x));}
TINLINE T cos2sin (T x) {return sin2cos(x);}

/* 2d vector */
template<typename T> struct vec2
{
  T x, y;
  typedef T scalar;
  INLINE vec2(void) {}
  INLINE vec2(const vec2& v) {x = v.x; y = v.y;}
  UINLINE vec2 (const vec2<U>& a) : x(T(a.x)), y(T(a.y)) {}
  UINLINE vec2& op= (const vec2<U>& v) {x = v.x; y = v.y; return *this;}
  INLINE explicit vec2(const T &a) : x(a), y(a) {}
  INLINE explicit vec2(const T &x, const T &y) : x(x), y(y) {}
  INLINE explicit vec2(const T *a, int stride = 1) : x(a[0]), y(a[stride]) {}
  INLINE vec2(zerotype) : x(zero),y(zero) {}
  INLINE vec2(onetype) : x(one),y(one) {}
  INLINE const T& op[](int axis) const {return (&x)[axis];}
  INLINE T& op[](int axis) {return (&x)[axis];}
};

TINLINE v2 op+ (v2arg a)  {return v2(+a.x, +a.y);}
TINLINE v2 op- (v2arg a)  {return v2(-a.x, -a.y);}
TINLINE v2 abs (v2arg a)  {return v2(abs(a.x), abs(a.y));}
TINLINE v2 rcp (v2arg a)  {return v2(rcp(a.x), rcp(a.y));}
TINLINE v2 sqrt (v2arg a) {return v2(sqrt(a.x), sqrt(a.y));}
TINLINE v2 rsqrt(v2arg a) {return v2(rsqrt(a.x), rsqrt(a.y));}
TINLINE v2 op+ (v2arg a, v2arg b) {return v2(a.x+b.x, a.y+b.y);}
TINLINE v2 op- (v2arg a, v2arg b) {return v2(a.x-b.x, a.y-b.y);}
TINLINE v2 op* (v2arg a, v2arg b) {return v2(a.x*b.x, a.y*b.y);}
TINLINE v2 op* (const T &a, v2arg b)  {return v2(a*b.x, a*b.y);}
TINLINE v2 op* (v2arg a, const T &b)  {return v2(a.x*b, a.y*b);}
TINLINE v2 op/ (v2arg a, v2arg b) {return v2(a.x/b.x, a.y/b.y);}
TINLINE v2 op/ (v2arg a, const T &b)  {return v2(a.x/b, a.y /b);}
TINLINE v2 op/ (const T &a, v2arg b)  {return v2(a/b.x, a/b.y);}
TINLINE v2 min(v2arg a, v2arg b)  {return v2(min(a.x,b.x), min(a.y,b.y));}
TINLINE v2 max(v2arg a, v2arg b)  {return v2(max(a.x,b.x), max(a.y,b.y));}
TINLINE T dot(v2arg a, v2arg b)   {return a.x*b.x + a.y*b.y;}
TINLINE T length(v2arg a)      {return sqrt(dot(a,a));}
TINLINE v2 normalize(v2arg a)  {return a*rsqrt(dot(a,a));}
TINLINE T distance (v2arg a, v2arg b) {return length(a-b);}
TINLINE v2& op+= (v2& a, v2arg b) {a.x+=b.x; a.y+=b.y; return a;}
TINLINE v2& op-= (v2& a, v2arg b) {a.x-=b.x; a.y-=b.y; return a;}
TINLINE v2& op*= (v2& a, const T &b)  {a.x*=b; a.y*=b; return a;}
TINLINE v2& op/= (v2& a, const T &b)  {a.x/=b; a.y/=b; return a;}
TINLINE T reduce_add(v2arg a) {return a.x+a.y;}
TINLINE T reduce_mul(v2arg a) {return a.x*a.y;}
TINLINE T reduce_min(v2arg a) {return min(a.x, a.y);}
TINLINE T reduce_max(v2arg a) {return max(a.x, a.y);}
TINLINE bool op== (v2arg a, v2arg b) {return a.x==b.x && a.y==b.y;}
TINLINE bool op!= (v2arg a, v2arg b) {return a.x!=b.x || a.y!=b.y;}
TINLINE bool op<  (v2arg a, v2arg b) {
  if (a.x != b.x) return a.x < b.x;
  if (a.y != b.y) return a.y < b.y;
  return false;
}
TINLINE v2 select (bool s, v2arg t, v2arg f) {
  return v2(select(s,t.x,f.x), select(s,t.y,f.y));
}

/* 3d vector */
template<typename T> struct vec3
{
  T x, y, z;
  typedef T scalar;
  INLINE vec3(void) {}
  INLINE vec3(const vec3& v) {x = v.x; y = v.y; z = v.z;}
  UINLINE vec3(const vec3<U>& a) : x(T(a.x)), y(T(a.y)), z(T(a.z)) {}
  UINLINE vec3& op= (const vec3<U>& v) {x = v.x; y = v.y; z = v.z; return *this;}
  INLINE explicit vec3(const T &a) : x(a), y(a), z(a) {}
  INLINE explicit vec3(const T &x, const T &y, const T &z) : x(x), y(y), z(z) {}
  INLINE explicit vec3(const T* a, int stride = 1) : x(a[0]), y(a[stride]), z(a[2*stride]) {}
  INLINE vec3 (zerotype) : x(zero),y(zero),z(zero) {}
  INLINE vec3 (onetype)  : x(one),y(one),z(one) {}
  INLINE const T& op[](int axis) const {return (&x)[axis];}
  INLINE T& op[](int axis) {return (&x)[axis];}
};

TINLINE v3 abs (v3arg a) {return v3(abs(a.x), abs(a.y), abs(a.z));}
TINLINE v3 rcp (v3arg a) {return v3(rcp(a.x), rcp(a.y), rcp(a.z));}
TINLINE v3 sqrt (v3arg a) {return v3(sqrt (a.x), sqrt (a.y), sqrt(a.z));}
TINLINE v3 rsqrt (v3arg a) {return v3(rsqrt(a.x), rsqrt(a.y), rsqrt(a.z));}
TINLINE v3 op+ (v3arg a) {return v3(+a.x, +a.y, +a.z);}
TINLINE v3 op- (v3arg a) {return v3(-a.x, -a.y, -a.z);}
TINLINE v3 op+ (v3arg a, v3arg b) {return v3(a.x+b.x, a.y+b.y, a.z+b.z);}
TINLINE v3 op- (v3arg a, v3arg b) {return v3(a.x-b.x, a.y-b.y, a.z-b.z);}
TINLINE v3 op* (v3arg a, v3arg b) {return v3(a.x*b.x, a.y*b.y, a.z*b.z);}
TINLINE v3 op/ (v3arg a, v3arg b) {return v3(a.x/b.x, a.y/b.y, a.z/b.z);}
TINLINE v3 op* (v3arg a, const T &b) {return v3(a.x*b, a.y*b, a.z*b);}
TINLINE v3 op/ (v3arg a, const T &b) {return v3(a.x/b, a.y/b, a.z/b);}
TINLINE v3 op* (const T &a, v3arg b) {return v3(a*b.x, a*b.y, a*b.z);}
TINLINE v3 op/ (const T &a, v3arg b) {return v3(a/b.x, a/b.y, a/b.z);}
TINLINE v3 op+= (v3& a, v3arg b) {a.x+=b.x; a.y+=b.y; a.z+=b.z; return a;}
TINLINE v3 op-= (v3& a, v3arg b) {a.x-=b.x; a.y-=b.y; a.z-=b.z; return a;}
TINLINE v3 op*= (v3& a, const T &b) {a.x*=b; a.y*=b; a.z*=b; return a;}
TINLINE v3 op/= (v3& a, const T &b) {a.x/=b; a.y/=b; a.z/=b; return a;}
TINLINE bool op== (v3arg a, v3arg b) {return a.x==b.x && a.y==b.y && a.z==b.z;}
TINLINE bool op!= (v3arg a, v3arg b) {return a.x!=b.x || a.y!=b.y || a.z!=b.z;}
TINLINE bool op<  (v3arg a, v3arg b) {
  if (a.x != b.x) return a.x < b.x;
  if (a.y != b.y) return a.y < b.y;
  if (a.z != b.z) return a.z < b.z;
  return false;
}
TINLINE T reduce_add (v3arg a) {return a.x + a.y + a.z;}
TINLINE T reduce_mul (v3arg a) {return a.x * a.y * a.z;}
TINLINE T reduce_min (v3arg a) {return min(a.x, a.y, a.z);}
TINLINE T reduce_max (v3arg a) {return max(a.x, a.y, a.z);}
TINLINE v3 min (v3arg a, v3arg b) {return v3(min(a.x,b.x), min(a.y,b.y), min(a.z,b.z));}
TINLINE v3 max (v3arg a, v3arg b) {return v3(max(a.x,b.x), max(a.y,b.y), max(a.z,b.z));}
TINLINE v3 select (bool s, v3arg t, v3arg f) {
  return v3(select(s,t.x,f.x), select(s,t.y,f.y), select(s,t.z,f.z));
}
TINLINE T length (v3arg a) {return sqrt(dot(a,a));}
TINLINE T dot (v3arg a, v3arg b) {return a.x*b.x + a.y*b.y + a.z*b.z;}
TINLINE T distance (v3arg a, v3arg b) {return length(a-b);}
TINLINE v3 normalize(v3arg a) {return a*rsqrt(dot(a,a));}
TINLINE v3 cross(v3arg a, v3arg b) {
  return v3(a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x);
}

/* 4d vector */
template<typename T> struct vec4
{
  T x, y, z, w;
  typedef T scalar;
  INLINE vec4(void) {}
  INLINE vec4(const vec4& v) {x = v.x; y = v.y; z = v.z; w = v.w;}
  UINLINE vec4(const vec4<U>& a) : x(T(a.x)), y(T(a.y)), z(T(a.z)), w(T(a.w)) {}
  UINLINE vec4& op= (const vec4<U>& v) {x = v.x; y = v.y; z = v.z; w = v.w; return *this;}
  INLINE explicit vec4(const T &a) : x(a), y(a), z(a), w(a) {}
  INLINE explicit vec4(const T &x, const T &y, const T &z, const T &w) : x(x), y(y), z(z), w(w) {}
  INLINE explicit vec4(const T *a, int stride = 1) : x(a[0]), y(a[stride]), z(a[2*stride]), w(a[3*stride]) {}
  INLINE vec4 (zerotype) : x(zero),y(zero),z(zero),w(zero) {}
  INLINE vec4 (onetype)  : x(one),y(one),z(one),w(one) {}
  INLINE const T& op[](int axis) const {return (&x)[axis];}
  INLINE T& op[](int axis) {return (&x)[axis];}
};

TINLINE v4 op+ (v4arg a) {return v4(+a.x, +a.y, +a.z, +a.w);}
TINLINE v4 op- (v4arg a) {return v4(-a.x, -a.y, -a.z, -a.w);}
TINLINE v4 op+ (v4arg a, v4arg b) {return v4(a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w);}
TINLINE v4 op- (v4arg a, v4arg b) {return v4(a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w);}
TINLINE v4 op* (v4arg a, v4arg b) {return v4(a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w);}
TINLINE v4 op/ (v4arg a, v4arg b) {return v4(a.x/b.x, a.y/b.y, a.z/b.z, a.w/b.w);}
TINLINE v4 op* (v4arg a, const T &b) {return v4(a.x*b, a.y*b, a.z*b, a.w*b);}
TINLINE v4 op/ (v4arg a, const T &b) {return v4(a.x/b, a.y/b, a.z/b, a.w/b);}
TINLINE v4 op* (const T &a, v4arg b) {return v4(a*b.x, a*b.y, a*b.z, a*b.w);}
TINLINE v4 op/ (const T &a, v4arg b) {return v4(a/b.x, a/b.y, a/b.z, a/b.w);}
TINLINE v4& op+= (v4& a, v4arg b) {a.x+=b.x; a.y+=b.y; a.z+=b.z; a.w+=b.w; return a;}
TINLINE v4& op-= (v4& a, v4arg b) {a.x-=b.x; a.y-=b.y; a.z-=b.z; a.w-=b.w; return a;}
TINLINE v4& op*= (v4& a, const T &b) {a.x*=b; a.y*=b; a.z*=b; a.w*=b; return a;}
TINLINE v4& op/= (v4& a, const T &b) {a.x/=b; a.y/=b; a.z/=b; a.w/=b; return a;}
TINLINE bool op== (v4arg a, v4arg b) {return a.x==b.x && a.y==b.y && a.z==b.z && a.w==b.w;}
TINLINE bool op!= (v4arg a, v4arg b) {return a.x!=b.x || a.y!=b.y || a.z!=b.z || a.w!=b.w;}
TINLINE bool op<  (v4arg a, v4arg b) {
  if (a.x != b.x) return a.x < b.x;
  if (a.y != b.y) return a.y < b.y;
  if (a.z != b.z) return a.z < b.z;
  if (a.w != b.w) return a.w < b.w;
  return false;
}
TINLINE T reduce_add(v4arg a) {return a.x + a.y + a.z + a.w;}
TINLINE T reduce_mul(v4arg a) {return a.x * a.y * a.z * a.w;}
TINLINE T reduce_min(v4arg a) {return min(a.x, a.y, a.z, a.w);}
TINLINE T reduce_max(v4arg a) {return max(a.x, a.y, a.z, a.w);}
TINLINE v4 min(v4arg a, v4arg b) {return v4(min(a.x,b.x), min(a.y,b.y), min(a.z,b.z), min(a.w,b.w));}
TINLINE v4 max(v4arg a, v4arg b) {return v4(max(a.x,b.x), max(a.y,b.y), max(a.z,b.z), max(a.w,b.w));}
TINLINE v4 abs (v4arg a) {return v4(abs(a.x), abs(a.y), abs(a.z), abs(a.w));}
TINLINE v4 rcp (v4arg a) {return v4(rcp(a.x), rcp(a.y), rcp(a.z), rcp(a.w));}
TINLINE v4 sqrt (v4arg a) {return v4(sqrt (a.x), sqrt (a.y), sqrt (a.z), sqrt (a.w));}
TINLINE v4 rsqrt(v4arg a) {return v4(rsqrt(a.x), rsqrt(a.y), rsqrt(a.z), rsqrt(a.w));}
TINLINE T length(v4arg a) {return sqrt(dot(a,a));}
TINLINE T dot (v4arg a, v4arg b) {return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;}
TINLINE v4 normalize(v4arg a) {return a*rsqrt(dot(a,a));}
TINLINE T distance (v4arg a, v4arg b) {return length(a-b);}
TINLINE v4 select (bool s, v4arg t, v4arg f) {
  return v4(select(s,t.x,f.x), select(s,t.y,f.y), select(s,t.z,f.z), select(s,t.w,f.w));
}

/* 3x3 matrix (linear transformation) */
template<typename T> struct mat3x3
{
  vec3<T> vx,vy,vz;
  INLINE mat3x3(void) {}
  INLINE mat3x3(const mat3x3 &m) {vx = m.vx; vy = m.vy; vz = m.vz;}
  INLINE mat3x3& op= (const mat3x3 &m) {vx = m.vx; vy = m.vy; vz = m.vz; return *this;}
  INLINE mat3x3(v3arg n) {
    vy = n;
    if (abs(n.x) >= abs(n.y)){
      const float inv = rcp(sqrt(n.x*n.x + n.z*n.z));
      vx = vec3f(-n.z*inv, 0.f, n.x*inv);
    } else {
      const float inv = rcp(sqrt(n.y*n.y + n.z*n.z));
      vx = vec3f(0.f, n.z*inv, -n.y*inv);
    }
    vx = normalize(vx);
    vy = normalize(vy);
    vz = cross(vy,vx);
  }
  UINLINE explicit mat3x3(const mat3x3<U> &s) : vx(s.vx), vy(s.vy), vz(s.vz) {}
  INLINE mat3x3(v3arg vx, v3arg vy, v3arg vz) : vx(vx), vy(vy), vz(vz) {}
  INLINE mat3x3(T m00, T m01, T m02,
                T m10, T m11, T m12,
                T m20, T m21, T m22) :
    vx(m00,m10,m20), vy(m01,m11,m21), vz(m02,m12,m22) {}
  INLINE mat3x3(zerotype) : vx(zero),vy(zero),vz(zero) {}
  INLINE mat3x3(onetype)  : vx(one,zero,zero),vy(zero,one,zero),vz(zero,zero,one) {}
  INLINE mat3x3 adjoint(void) const {
    return mat3x3(cross(vy,vz), cross(vz,vx), cross(vx,vy)).transposed();
  }
  INLINE mat3x3 transposed(void) const {
    return mat3x3(vx.x,vx.y,vx.z, vy.x,vy.y,vy.z, vz.x,vz.y,vz.z);
  }
  INLINE mat3x3 inverse(void) const {return rcp(det())*adjoint();}
  INLINE T det(void) const {return dot(vx,cross(vy,vz));}
  static INLINE mat3x3 scale(v3arg s) {return mat3x3(s.x,0,0,0,s.y,0,0,0,s.z);}
  static INLINE mat3x3 rotate(v3arg _u, T r) {
    const v3 u = normalize(_u);
    const T s = sin(r), c = cos(r);
    return mat3x3(u.x*u.x+(1-u.x*u.x)*c,u.x*u.y*(1-c)-u.z*s,  u.x*u.z*(1-c)+u.y*s,
                  u.x*u.y*(1-c)+u.z*s,  u.y*u.y+(1-u.y*u.y)*c,u.y*u.z*(1-c)-u.x*s,
                  u.x*u.z*(1-c)-u.y*s,  u.y*u.z*(1-c)+u.x*s,  u.z*u.z+(1-u.z*u.z)*c);
  }
};

TINLINE m33 op- (m33arg a) {return m33(-a.vx,-a.vy,-a.vz);}
TINLINE m33 op+ (m33arg a) {return m33(+a.vx,+a.vy,+a.vz);}
TINLINE m33 rcp (m33arg a) {return a.inverse();}
TINLINE m33 op+ (m33arg a, m33arg b) {return m33(a.vx+b.vx,a.vy+b.vy,a.vz+b.vz);}
TINLINE m33 op- (m33arg a, m33arg b) {return m33(a.vx-b.vx,a.vy-b.vy,a.vz-b.vz);}
TINLINE m33 op* (m33arg a, m33arg b) {return m33(a*b.vx, a*b.vy, a*b.vz);}
TINLINE m33 op/ (m33arg a, m33arg b) {return a*rcp(b);}
TINLINE v3  op* (m33arg a, v3arg  b) {return b.x*a.vx + b.y*a.vy + b.z*a.vz;}
TINLINE m33 op* (T a, m33arg b)   {return m33(a*b.vx, a*b.vy, a*b.vz);}
TINLINE m33 op/ (m33arg a, T b)   {return a * rcp(b);}
TINLINE m33& op/= (m33& a, T b)   {return a = a/b;}
TINLINE m33& op*= (m33& a, T b)   {return a = a*b;}
TINLINE m33& op*= (m33& a, m33 b) {return a = a*b;}
TINLINE m33& op/= (m33& a, m33 b) {return a = a/b;}
TINLINE bool op== (m33arg a, m33arg b)  {return (a.vx==b.vx) && (a.vy == b.vy) && (a.vz == b.vz);}
TINLINE bool op!= (m33arg a, m33arg b)  {return (a.vx!=b.vx) || (a.vy != b.vy) || (a.vz != b.vz);}
TINLINE v3 xfmpoint (m33arg s, v3arg a) {return a.x*s.vx + a.y*s.vy + a.z*s.vz;}
TINLINE v3 xfmvector(m33arg s, v3arg a) {return a.x*s.vx + a.y*s.vy + a.z*s.vz;}
TINLINE v3 xfmnormal(m33arg s, v3arg a) {return xfmvector(s.inverse().transposed(),a);}
TINLINE m33 frame(v3arg N) {
  const v3 dx0 = cross(v3(T(one),T(zero),T(zero)),N);
  const v3 dx1 = cross(v3(T(zero),T(one),T(zero)),N);
  const v3 dx = normalize(select(dot(dx0,dx0) > dot(dx1,dx1),dx0,dx1));
  const v3 dy = normalize(cross(N,dx));
  return m33(dx,dy,N);
}

/* 4x3 matrix (affine transformation) */
template<typename T> struct mat4x3
{
  mat3x3<T> l;
  vec3<T> p;
  INLINE mat4x3(void) {}
  INLINE mat4x3(const mat4x3 &m) {l = m.l; p = m.p;}
  INLINE mat4x3(v3arg vx, v3arg vy, v3arg vz, v3arg p) : l(vx,vy,vz), p(p) {}
  INLINE mat4x3(m33arg l, v3arg p) : l(l), p(p) {}
  INLINE mat4x3& op= (const mat4x3 &m) {l=m.l; p=m.p; return *this;}
  UINLINE explicit mat4x3(const mat4x3<U>& s) : l(s.l), p(s.p) {}
  INLINE mat4x3(zerotype) : l(zero),p(zero) {}
  INLINE mat4x3(onetype)  : l(one),p(zero) {}

  static INLINE mat4x3 scale(v3arg s) {return mat4x3(m33::scale(s),zero);}
  static INLINE mat4x3 translate(v3arg p) {return mat4x3(one,p);}
  static INLINE mat4x3 rotate(v3arg u, T r) {return mat4x3(m33::rotate(u,r),zero);}
  static INLINE mat4x3 rotate(v3arg p, v3arg u, T r) {return translate(p)*rotate(u,r)*translate(-p);}
  static INLINE mat4x3 lookat(v3arg eye, v3arg point, v3arg up) {
    const v3 z = normalize(point-eye);
    const v3 u = normalize(cross(up,z));
    const v3 v = normalize(cross(z,u));
    return mat4x3(m33(u,v,z),eye);
  }
};

TINLINE m43 op- (m43arg a) {return m43(-a.l,-a.p);}
TINLINE m43 op+ (m43arg a) {return m43(+a.l,+a.p);}
TINLINE m43 rcp (m43arg a) {m33 il = rcp(a.l); return m43(il,-il*a.p);}
TINLINE m43 op+ (m43arg a, m43arg b)  {return m43(a.l+b.l,a.p+b.p);}
TINLINE m43 op- (m43arg a, m43arg b)  {return m43(a.l-b.l,a.p-b.p);}
TINLINE m43 op* (T a, m43 b)          {return m43(a*b.l,a*b.p);}
TINLINE m43 op* (m43arg a, m43arg b)  {return m43(a.l*b.l,a.l*b.p+a.p);}
TINLINE m43 op/ (m43arg a, m43arg b)  {return a * rcp(b);}
TINLINE m43 op/ (m43arg a, T b) {return a * rcp(b);}
TINLINE m43& op*= (m43& a, T b) {return a = a*b;}
TINLINE m43& op/= (m43& a, T b) {return a = a/b;}
TINLINE m43& op*= (m43& a, m43arg b) {return a = a*b;}
TINLINE m43& op/= (m43& a, m43arg b) {return a = a/b;}
TINLINE bool op== (m43arg a, m43arg b)  {return (a.l==b.l) && (a.p==b.p);}
TINLINE bool op!= (m43arg a, m43arg b)  {return (a.l!=b.l) || (a.p!=b.p);}
TINLINE v3 xfmpoint (m43arg m, v3arg p) {return xfmpoint(m.l,p) + m.p;}
TINLINE v3 xfmvector(m43arg m, v3arg v) {return xfmvector(m.l,v);}
TINLINE v3 xfmnormal(m43arg m, v3arg n) {return xfmnormal(m.l,n);}

/* 4x4 matrix (homogenous transformation) */
template<typename T> struct mat4x4
{
  vec4<T> vx,vy,vz,vw;
  INLINE mat4x4(void) {}
  INLINE mat4x4(m44arg m) {vx=m.vx; vy=m.vy; vz=m.vz; vw=m.vw;}
  UINLINE mat4x4(const mat4x4<U> &m) :
    vx(v4(m.vx)), vy(v4(m.vy)), vz(v4(m.vz)), vw(v4(m.vw)) {}
  UINLINE explicit mat4x4 (U s) {
    vx = v4(T(s), zero, zero, zero);
    vy = v4(zero, T(s), zero, zero);
    vz = v4(zero, zero, T(s), zero);
    vw = v4(zero, zero, zero, T(s));
  }
  INLINE mat4x4(T s) {
    vx = v4(s, zero, zero, zero);
    vy = v4(zero, s, zero, zero);
    vz = v4(zero, zero, s, zero);
    vw = v4(zero, zero, zero, s);
  }
  INLINE mat4x4(v4arg c0, v4arg c1, v4arg c2, v4arg c3) :
    vx(c0), vy(c1), vz(c2), vw(c3) {}
  INLINE mat4x4 (m33arg m) {
    vx = v4(m.vx, zero);
    vy = v4(m.vy, zero);
    vz = v4(m.vz, zero);
    vw = v4(zero, zero, zero, one);
  }
  INLINE mat4x4 (m43arg m) {
    vx = v4(m.vx, zero);
    vy = v4(m.vy, zero);
    vz = v4(m.vz, zero);
    vw = v4(m.vw, one);
  }
  INLINE mat4x4(zerotype) : vx(zero),vy(zero),vz(zero),vw(zero) {}
  INLINE mat4x4(onetype) {
    vx = v4(one, zero, zero, zero);
    vy = v4(zero, one, zero, zero);
    vz = v4(zero, zero, one, zero);
    vw = v4(zero, zero, zero, one);
  }
  m44& op= (const m44 &m) {vx=m.vx;vy=m.vy;vz=m.vz;vw=m.vw;return *this;}
  INLINE m44 inverse(void) const;
  INLINE v4& op[] (int i) {return (&vx)[i];}
  INLINE const v4& op[] (int i) const {return (&vx)[i];}
};

TINLINE m44 op- (m44arg a) {return m44(-a.vx,-a.vy,-a.vz,-a.vw);}
TINLINE m44 op+ (m44arg a) {return m44(+a.vx,+a.vy,+a.vz,+a.vw);}
TINLINE m44 op+ (m44arg m, T s) {return m44(m.vx+s, m.vy+s, m.vz+s, m.vw+s);}
TINLINE m44 op- (m44arg m, T s) {return m44(m.vx-s, m.vy-s, m.vz-s, m.vw-s);}
TINLINE m44 op* (m44arg m, T s) {return m44(m.vx*s, m.vy*s, m.vz*s, m.vw*s);}
TINLINE m44 op/ (m44arg m, T s) {return m44(m.vx/s, m.vy/s, m.vz/s, m.vw/s);}
TINLINE m44 op+ (T s, m44arg m) {return m44(m.vx+s, m.vy+s, m.vz+s, m.vw+s);}
TINLINE m44 op- (T s, m44arg m) {return m44(m.vx-s, m.vy-s, m.vz-s, m.vw-s);}
TINLINE m44 op* (T s, m44arg m) {return m44(m.vx*s, m.vy*s, m.vz*s, m.vw*s);}
TINLINE m44 op+ (m44arg m, m44arg n) {return m44(m.vx+n.vx, m.vy+n.vy, m.vz+n.vz, m.vw+n.vw);}
TINLINE m44 op- (m44arg m, m44arg n) {return m44(m.vx-n.vx, m.vy-n.vy, m.vz-n.vz, m.vw-n.vw);}
TINLINE v4  op* (m44arg m, v4arg v) {
  return v4(m.vx.x*v.x + m.vy.x*v.y + m.vz.x*v.z + m.vw.x*v.w,
            m.vx.y*v.x + m.vy.y*v.y + m.vz.y*v.z + m.vw.y*v.w,
            m.vx.z*v.x + m.vy.z*v.y + m.vz.z*v.z + m.vw.z*v.w,
            m.vx.w*v.x + m.vy.w*v.y + m.vz.w*v.z + m.vw.w*v.w);
}
TINLINE v4 op* (v4arg v, m44arg m) {
  return V(m.vx.x*v.x + m.vx.y*v.y + m.vx.z*v.z + m.vx.w*v.w,
           m.vy.x*v.x + m.vy.y*v.y + m.vy.z*v.z + m.vy.w*v.w,
           m.vz.x*v.x + m.vz.y*v.y + m.vz.z*v.z + m.vz.w*v.w,
           m.vw.x*v.x + m.vw.y*v.y + m.vw.z*v.z + m.vw.w*v.w);
}
TINLINE m44 op* (m44arg m, m44arg n) {
  const v4 a0 = m[0], a1 = m[1], a2 = m[2], a3 = m[3];
  const v4 b0 = n[0], b1 = n[1], b2 = n[2], b3 = n[3];
  m44 dst;
  dst.vx = a0*b0[0] + a1*b0[1] + a2*b0[2] + a3*b0[3];
  dst.vy = a0*b1[0] + a1*b1[1] + a2*b1[2] + a3*b1[3];
  dst.vz = a0*b2[0] + a1*b2[1] + a2*b2[2] + a3*b2[3];
  dst.vw = a0*b3[0] + a1*b3[1] + a2*b3[2] + a3*b3[3];
  return dst;
}
TINLINE m44& op*= (m44& a, T b) {return a = a*b;}
TINLINE m44& op/= (m44& a, T b) {return a = a/b;}
TINLINE m44& op*= (m44& a, m44arg b) {return a = a*b;}
TINLINE m44& op/= (m44& a, m44arg b) {return a = a/b;}
TINLINE v4 op/ (m44arg m, v4arg v) {return m.inverse() * v;}
TINLINE v4 op/ (v4arg v, m44arg m) {return v * m.inverse();}
TINLINE m44 op/ (m44arg m, m44arg n) {return m * n.inverse();}
TINLINE bool op== (m44arg m, m44arg n) {return (m.vx==n[0]) && (m.vy==n[1]) && (m.vz==n[2]) && (m.vw==n[3]);}
TINLINE bool op!= (m44arg m, m44arg n) {return (m.vx!=n[0]) || (m.vy!=n[1]) || (m.vz!=n[2]) || (m.vw!=n[3]);}

TINLINE m44 m44::inverse(void) const {
  m44 inv;
  (&inv.vx.x)[0] =   (&vx.x)[5]*(&vx.x)[10]*(&vx.x)[15] - (&vx.x)[5]*(&vx.x)[11]*(&vx.x)[14] - (&vx.x)[9]*(&vx.x)[6]*(&vx.x)[15]
           + (&vx.x)[9]*(&vx.x)[7]*(&vx.x)[14] + (&vx.x)[13]*(&vx.x)[6]*(&vx.x)[11] - (&vx.x)[13]*(&vx.x)[7]*(&vx.x)[10];
  (&inv.vx.x)[4] =  -(&vx.x)[4]*(&vx.x)[10]*(&vx.x)[15] + (&vx.x)[4]*(&vx.x)[11]*(&vx.x)[14] + (&vx.x)[8]*(&vx.x)[6]*(&vx.x)[15]
           - (&vx.x)[8]*(&vx.x)[7]*(&vx.x)[14] - (&vx.x)[12]*(&vx.x)[6]*(&vx.x)[11] + (&vx.x)[12]*(&vx.x)[7]*(&vx.x)[10];
  (&inv.vx.x)[8] =   (&vx.x)[4]*(&vx.x)[9]*(&vx.x)[15] - (&vx.x)[4]*(&vx.x)[11]*(&vx.x)[13] - (&vx.x)[8]*(&vx.x)[5]*(&vx.x)[15]
           + (&vx.x)[8]*(&vx.x)[7]*(&vx.x)[13] + (&vx.x)[12]*(&vx.x)[5]*(&vx.x)[11] - (&vx.x)[12]*(&vx.x)[7]*(&vx.x)[9];
  (&inv.vx.x)[12] = -(&vx.x)[4]*(&vx.x)[9]*(&vx.x)[14] + (&vx.x)[4]*(&vx.x)[10]*(&vx.x)[13] + (&vx.x)[8]*(&vx.x)[5]*(&vx.x)[14]
           - (&vx.x)[8]*(&vx.x)[6]*(&vx.x)[13] - (&vx.x)[12]*(&vx.x)[5]*(&vx.x)[10] + (&vx.x)[12]*(&vx.x)[6]*(&vx.x)[9];
  (&inv.vx.x)[1] =  -(&vx.x)[1]*(&vx.x)[10]*(&vx.x)[15] + (&vx.x)[1]*(&vx.x)[11]*(&vx.x)[14] + (&vx.x)[9]*(&vx.x)[2]*(&vx.x)[15]
           - (&vx.x)[9]*(&vx.x)[3]*(&vx.x)[14] - (&vx.x)[13]*(&vx.x)[2]*(&vx.x)[11] + (&vx.x)[13]*(&vx.x)[3]*(&vx.x)[10];
  (&inv.vx.x)[5] =   (&vx.x)[0]*(&vx.x)[10]*(&vx.x)[15] - (&vx.x)[0]*(&vx.x)[11]*(&vx.x)[14] - (&vx.x)[8]*(&vx.x)[2]*(&vx.x)[15]
           + (&vx.x)[8]*(&vx.x)[3]*(&vx.x)[14] + (&vx.x)[12]*(&vx.x)[2]*(&vx.x)[11] - (&vx.x)[12]*(&vx.x)[3]*(&vx.x)[10];
  (&inv.vx.x)[9] =  -(&vx.x)[0]*(&vx.x)[9]*(&vx.x)[15] + (&vx.x)[0]*(&vx.x)[11]*(&vx.x)[13] + (&vx.x)[8]*(&vx.x)[1]*(&vx.x)[15]
           - (&vx.x)[8]*(&vx.x)[3]*(&vx.x)[13] - (&vx.x)[12]*(&vx.x)[1]*(&vx.x)[11] + (&vx.x)[12]*(&vx.x)[3]*(&vx.x)[9];
  (&inv.vx.x)[13] =  (&vx.x)[0]*(&vx.x)[9]*(&vx.x)[14] - (&vx.x)[0]*(&vx.x)[10]*(&vx.x)[13] - (&vx.x)[8]*(&vx.x)[1]*(&vx.x)[14]
           + (&vx.x)[8]*(&vx.x)[2]*(&vx.x)[13] + (&vx.x)[12]*(&vx.x)[1]*(&vx.x)[10] - (&vx.x)[12]*(&vx.x)[2]*(&vx.x)[9];
  (&inv.vx.x)[2] =   (&vx.x)[1]*(&vx.x)[6]*(&vx.x)[15] - (&vx.x)[1]*(&vx.x)[7]*(&vx.x)[14] - (&vx.x)[5]*(&vx.x)[2]*(&vx.x)[15]
           + (&vx.x)[5]*(&vx.x)[3]*(&vx.x)[14] + (&vx.x)[13]*(&vx.x)[2]*(&vx.x)[7] - (&vx.x)[13]*(&vx.x)[3]*(&vx.x)[6];
  (&inv.vx.x)[6] =  -(&vx.x)[0]*(&vx.x)[6]*(&vx.x)[15] + (&vx.x)[0]*(&vx.x)[7]*(&vx.x)[14] + (&vx.x)[4]*(&vx.x)[2]*(&vx.x)[15]
           - (&vx.x)[4]*(&vx.x)[3]*(&vx.x)[14] - (&vx.x)[12]*(&vx.x)[2]*(&vx.x)[7] + (&vx.x)[12]*(&vx.x)[3]*(&vx.x)[6];
  (&inv.vx.x)[10] =  (&vx.x)[0]*(&vx.x)[5]*(&vx.x)[15] - (&vx.x)[0]*(&vx.x)[7]*(&vx.x)[13] - (&vx.x)[4]*(&vx.x)[1]*(&vx.x)[15]
           + (&vx.x)[4]*(&vx.x)[3]*(&vx.x)[13] + (&vx.x)[12]*(&vx.x)[1]*(&vx.x)[7] - (&vx.x)[12]*(&vx.x)[3]*(&vx.x)[5];
  (&inv.vx.x)[14] = -(&vx.x)[0]*(&vx.x)[5]*(&vx.x)[14] + (&vx.x)[0]*(&vx.x)[6]*(&vx.x)[13] + (&vx.x)[4]*(&vx.x)[1]*(&vx.x)[14]
           - (&vx.x)[4]*(&vx.x)[2]*(&vx.x)[13] - (&vx.x)[12]*(&vx.x)[1]*(&vx.x)[6] + (&vx.x)[12]*(&vx.x)[2]*(&vx.x)[5];
  (&inv.vx.x)[3] =  -(&vx.x)[1]*(&vx.x)[6]*(&vx.x)[11] + (&vx.x)[1]*(&vx.x)[7]*(&vx.x)[10] + (&vx.x)[5]*(&vx.x)[2]*(&vx.x)[11]
           - (&vx.x)[5]*(&vx.x)[3]*(&vx.x)[10] - (&vx.x)[9]*(&vx.x)[2]*(&vx.x)[7] + (&vx.x)[9]*(&vx.x)[3]*(&vx.x)[6];
  (&inv.vx.x)[7] =   (&vx.x)[0]*(&vx.x)[6]*(&vx.x)[11] - (&vx.x)[0]*(&vx.x)[7]*(&vx.x)[10] - (&vx.x)[4]*(&vx.x)[2]*(&vx.x)[11]
           + (&vx.x)[4]*(&vx.x)[3]*(&vx.x)[10] + (&vx.x)[8]*(&vx.x)[2]*(&vx.x)[7] - (&vx.x)[8]*(&vx.x)[3]*(&vx.x)[6];
  (&inv.vx.x)[11] = -(&vx.x)[0]*(&vx.x)[5]*(&vx.x)[11] + (&vx.x)[0]*(&vx.x)[7]*(&vx.x)[9] + (&vx.x)[4]*(&vx.x)[1]*(&vx.x)[11]
           - (&vx.x)[4]*(&vx.x)[3]*(&vx.x)[9] - (&vx.x)[8]*(&vx.x)[1]*(&vx.x)[7] + (&vx.x)[8]*(&vx.x)[3]*(&vx.x)[5];
  (&inv.vx.x)[15] =  (&vx.x)[0]*(&vx.x)[5]*(&vx.x)[10] - (&vx.x)[0]*(&vx.x)[6]*(&vx.x)[9] - (&vx.x)[4]*(&vx.x)[1]*(&vx.x)[10]
           + (&vx.x)[4]*(&vx.x)[2]*(&vx.x)[9] + (&vx.x)[8]*(&vx.x)[1]*(&vx.x)[6] - (&vx.x)[8]*(&vx.x)[2]*(&vx.x)[5];

  return inv / ((&vx.x)[0]*(&inv.vx.x)[0] + (&vx.x)[1]*(&inv.vx.x)[4] + (&vx.x)[2]*(&inv.vx.x)[8] + (&vx.x)[3]*(&inv.vx.x)[12]);
}
TINLINE m44 translate(m44arg m, v3arg v) {
  m44 dst(m);
  dst.vw = m.vx*v.x + m.vy*v.y + m.vz*v.z + m.vw;
  return dst;
}
TINLINE m44 lookat(v3arg eye, v3arg center, v3arg up)
{
  const v3 f = normalize(center - eye);
  const v3 u = normalize(up);
  const v3 s = normalize(cross(f, u));
  const v3 t = cross(s, f);
  m44 dst(one);
  dst[0][0] = s.x; dst[1][0] = s.y; dst[2][0] = s.z;
  dst[0][1] = t.x; dst[1][1] = t.y; dst[2][1] = t.z;
  dst[0][2] =-f.x; dst[1][2] =-f.y; dst[2][2] =-f.z;
  return translate(dst, -eye);
}
TINLINE m44 perspective(T fovy, T aspect, T znear, T zfar)
{
  const T range = tan(deg2rad(fovy / T(two))) * znear;
  const T left = -range * aspect;
  const T right = range * aspect;
  const T bottom = -range;
  const T top = range;
  m44 dst(zero);
  dst.vx.x = (T(two) * znear) / (right - left);
  dst.vy.y = (T(two) * znear) / (top - bottom);
  dst.vz.z = -(zfar + znear) / (zfar - znear);
  dst.vz.w = -T(one);
  dst.vw.z = -(T(two) * zfar * znear) / (zfar - znear);
  return dst;
}
TINLINE m44 rotate(m44arg m, T angle, v3arg v)
{
  m44 rot(zero), dst(zero);
  const T a = deg2rad(angle);
  const T c = cos(a);
  const T s = sin(a);
  const v3 axis = normalize(v);
  const v3 temp = (T(one) - c) * axis;
  rot.vx[0] = c + temp[0]*axis[0];
  rot.vx[1] = T(zero) + temp[0]*axis[1] + s*axis[2];
  rot.vx[2] = T(zero) + temp[0]*axis[2] - s*axis[1];
  rot.vy[0] = T(zero) + temp[1]*axis[0] - s*axis[2];
  rot.vy[1] = c + temp[1]*axis[1];
  rot.vy[2] = T(zero) + temp[1]*axis[2] + s*axis[0];
  rot.vz[0] = T(zero) + temp[2]*axis[0] + s*axis[1];
  rot.vz[1] = T(zero) + temp[2]*axis[1] - s*axis[0];
  rot.vz[2] = c + temp[2]*axis[2];
  dst.vx = m.vx*rot.vx[0] + m.vy*rot.vx[1] + m.vz*rot.vx[2];
  dst.vy = m.vx*rot.vy[0] + m.vy*rot.vy[1] + m.vz*rot.vy[2];
  dst.vz = m.vx*rot.vz[0] + m.vy*rot.vz[1] + m.vz*rot.vz[2];
  dst.vw = m.vw;
  return dst;
}
TINLINE m44 ortho(T left, T right, T bottom, T top, T znear, T zfar)
{
  m44 dst(one);
  dst[0][0] = T(two) / (right - left);
  dst[1][1] = T(two) / (top - bottom);
  dst[2][2] = - T(two) / (zfar - znear);
  dst[3][0] = - (right + left) / (right - left);
  dst[3][1] = - (top + bottom) / (top - bottom);
  dst[3][2] = - (zfar + znear) / (zfar - znear);
  return dst;
}
TINLINE m44 scale(m44arg m, v3arg v)
{
  m44 dst;
  dst[0] = m[0] * v[0];
  dst[1] = m[1] * v[1];
  dst[2] = m[2] * v[2];
  dst[3] = m[3];
  return dst;
}
TINLINE v3 unproject(v3arg win, m44arg model, m44arg proj, const vec4<int> &viewport)
{
  const m44 p = proj*model;
  const m44 final = p.inverse();
  v4 in(win.x, win.y, win.z, T(one));
  in.x = T(two) * (win.x - T(viewport.x)) / T(viewport.z) - T(one);
  in.y = T(two) * (win.y - T(viewport.y)) / T(viewport.w) - T(one);
  in.z = T(two) * win.z - T(one);
  const v4 out = final*in;
  return v3(out.x/out.w,out.y/out.w,out.z/out.w);
}

#if 0 // deactivated for now to have vs 10 support
/* convenient variable size float vector */
template <int n> struct vvec
{
  template <typename... T> INLINE vvec(T... args) { this->set(0,args...); }
  template <typename First, typename... Rest>
  INLINE void set(int index, First first, Rest... rest) {
    this->v[index] = first;
    set(index+1,rest...);
  }
  INLINE void set(int index) {}
  float &operator[] (int index) { return v[index]; }
  const float &operator[] (int index) const { return v[index]; }
  float v[n];
};
#else
/* convenient variable size float vector */
template <int n> struct vvec
{
  INLINE vvec(void) {}
  template <typename T0> INLINE vvec(T0 x0) {
    v[0] = x0;
  }
  template <typename T0, typename T1>
  INLINE vvec(T0 x0, T1 x1) {
    v[0] = x0; v[1] = x1;
  }
  template <typename T0, typename T1, typename T2>
  INLINE vvec(T0 x0, T1 x1, T2 x2) {
    v[0] = x0; v[1] = x1; v[2] = x2;
  }
  template <typename T0, typename T1, typename T2, typename T3>
  INLINE vvec(T0 x0, T1 x1, T2 x2, T3 x3) {
    v[0] = x0; v[1] = x1; v[2] = x2; v[3] = x3;
  }
  template <typename T0, typename T1, typename T2, typename T3, typename T4>
  INLINE vvec(T0 x0, T1 x1, T2 x2, T3 x3, T4 x4) {
    v[0] = x0; v[1] = x1; v[2] = x2; v[3] = x3;
    v[4] = x4;
  }
  template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
  INLINE vvec(T0 x0, T1 x1, T2 x2, T3 x3, T4 x4, T5 x5) {
    v[0] = x0; v[1] = x1; v[2] = x2; v[3] = x3;
    v[4] = x4; v[5] = x5;
  }
  template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
  INLINE vvec(T0 x0, T1 x1, T2 x2, T3 x3, T4 x4, T5 x5, T6 x6) {
    v[0] = x0; v[1] = x1; v[2] = x2; v[3] = x3;
    v[4] = x4; v[5] = x5; v[6] = x6;
  }
  template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
  INLINE vvec(T0 x0, T1 x1, T2 x2, T3 x3, T4 x4, T5 x5, T6 x6, T7 x7) {
    v[0] = x0; v[1] = x1; v[2] = x2; v[3] = x3;
    v[4] = x4; v[5] = x5; v[6] = x6; v[7] = x7;
  }
  INLINE void set(int index) {}
  float &operator[] (int index) { return v[index]; }
  const float &operator[] (int index) const { return v[index]; }
  float v[n];
};

#endif

/* commonly used types */
typedef mat3x3<float> mat3x3f;
typedef mat3x3<double> mat3x3d;
typedef mat4x3<mat3x3f> mat4x3f;
typedef mat4x4<float> mat4x4f;
typedef mat4x4<double> mat4x4d;
typedef vec2<bool> vec2b;
typedef vec2<int> vec2i;
typedef vec2<float> vec2f;
typedef vec2<double> vec2d;
typedef vec3<bool> vec3b;
typedef vec3<int> vec3i;
typedef vec3<float> vec3f;
typedef vec3<double> vec3d;
typedef vec4<bool> vec4b;
typedef vec4<int> vec4i;
typedef vec4<float> vec4f;
typedef vec4<double> vec4d;

#undef TINLINE
#undef UINLINE
#undef op
#undef v2
#undef v3
#undef v4
#undef m33
#undef m43
#undef m44
#undef v2arg
#undef v3arg
#undef v4arg
#undef m33arg
#undef m43arg
#undef m44arg

} /* namespace cube */

#endif /* __CUBE_MATH_HPP__ */

