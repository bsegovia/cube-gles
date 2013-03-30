// generic useful stuff for any C++ program
#ifndef __CUBE_TOOLS_HPP__
#define __CUBE_TOOLS_HPP__

#ifdef _MSC_VER
#undef NOINLINE
#define NOINLINE        __declspec(noinline)
#define INLINE          __forceinline
#define RESTRICT        __restrict
#define THREAD          __declspec(thread)
#define ALIGNED(...)    __declspec(align(__VA_ARGS__))
//#define __FUNCTION__  __FUNCTION__
#define DEBUGBREAK()    __debugbreak()
#else
#undef NOINLINE
#undef INLINE
#define NOINLINE        __attribute__((noinline))
#define INLINE          inline __attribute__((always_inline))
#define RESTRICT        __restrict
#define THREAD          __thread
#define ALIGNED(...)    __attribute__((aligned(__VA_ARGS__)))
#define __FUNCTION__    __PRETTY_FUNCTION__
#define DEBUGBREAK()    asm ("int $3")
#endif

#ifdef __GNUC__
#define gamma __gamma
#endif

#ifdef _MSC_VER
#pragma warning( 3 : 4189 )
#pragma warning (disable : 4244)
#include <malloc.h>
#endif

#define _USE_MATH_DEFINES
#include <cmath>

#ifdef __GNUC__
#undef gamma
#endif

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <climits>
#include <cassert>
#include <cfloat>
#ifdef __GNUC__
#include <new>
#else
#include <new.h>
#endif

#ifdef NULL
#undef NULL
#endif
#define NULL 0

namespace cube {

  typedef unsigned char uchar;
  typedef unsigned short ushort;
  typedef unsigned int uint;

#if defined(WIN32)
    #undef min
    #undef max
#if defined(_MSC_VER)
    INLINE bool finite (float x) {return _finite(x) != 0;}
#endif
#endif

#define rnd(max) (rand()%(max))
#define rndreset() (srand(1))
#define rndtime() {loopi(lastmillis&0xF) rnd(i+1);}
#define loop(v,m) for(int v = 0; v<(m); v++)
#define loopi(m) loop(i,m)
#define loopj(m) loop(j,m)
#define loopk(m) loop(k,m)
#define loopl(m) loop(l,m)
#define loopv(v)    for(int i = 0; i<(v).length(); ++i)
#define loopvrev(v) for(int i = (v).length()-1; i>=0; --i)

  /* integer types */
#if defined(__MSVC__)
  typedef          __int64  int64;
  typedef unsigned __int64 uint64;
  typedef          __int32  int32;
  typedef unsigned __int32 uint32;
  typedef          __int16  int16;
  typedef unsigned __int16 uint16;
  typedef          __int8    int8;
  typedef unsigned __int8   uint8;
#else
  typedef          long long  int64;
  typedef unsigned long long uint64;
  typedef                int  int32;
  typedef unsigned       int uint32;
  typedef              short  int16;
  typedef unsigned     short uint16;
  typedef               char   int8;
  typedef unsigned      char  uint8;
#endif

#ifdef WIN32
#define PATHDIV '\\'
#else
#define __cdecl
#define _vsnprintf vsnprintf
#define PATHDIV '/'
#endif


  /* easy safe strings */
#define _MAXDEFSTR 260
  typedef char string[_MAXDEFSTR];

  INLINE void strn0cpy(char *d, const char *s, size_t m) { strncpy(d,s,m); d[(m)-1] = 0; }
  INLINE void strcpy_s(char *d, const char *s) { strn0cpy(d,s,_MAXDEFSTR); }
  INLINE void strcat_s(char *d, const char *s) { size_t n = strlen(d); strn0cpy(d+n,s,_MAXDEFSTR-n); }
  INLINE void formatstring(char *d, const char *fmt, va_list v) {
    _vsnprintf(d, _MAXDEFSTR, fmt, v);
    d[_MAXDEFSTR-1] = 0;
  }

  struct sprintf_s_f
  {
    char *d;
    sprintf_s_f(char *str): d(str) {};
    void operator()(const char* fmt, ...)
    {
      va_list v;
      va_start(v, fmt);
      _vsnprintf(d, _MAXDEFSTR, fmt, v);
      va_end(v);
      d[_MAXDEFSTR-1] = 0;
    }
  };

#define sprintf_s(d) sprintf_s_f((char *)d)
#define sprintf_sd(d) string d; sprintf_s(d)
#define sprintf_sdlv(d,last,fmt) string d; { va_list ap; va_start(ap, last); formatstring(d, fmt, ap); va_end(ap); }
#define sprintf_sdv(d,fmt) sprintf_sdlv(d,fmt,fmt)
#define ATOI(s) strtol(s, NULL, 0) /* supports hexadecimal numbers */

#define fast_f2nat(val) ((int)(val))

  extern char *path(char *s);
  extern char *loadfile(char *fn, int *size);
  extern void endianswap(void *, int, int);

#define PI  (3.1415927f)
#define PI2 (2*PI)

  class noncopyable
  {
    protected:
      INLINE noncopyable(void) {}
      INLINE ~noncopyable(void) {}
    private:
      INLINE noncopyable(const noncopyable&) {}
      INLINE noncopyable& operator= (const noncopyable&) {return *this;}
  };

  /* memory pool that uses buckets and linear allocation for small objects
   * VERY fast, and reasonably good memory reuse
   */
  struct pool
  {
    enum { POOLSIZE = 4096 };   // can be absolutely anything
    enum { PTRSIZE = sizeof(char *) };
    enum { MAXBUCKETS = 65 };   // meaning up to size 256 on 32bit pointer systems
    enum { MAXREUSESIZE = MAXBUCKETS*PTRSIZE-PTRSIZE };
    INLINE size_t bucket(size_t s) { return (s+PTRSIZE-1)>>PTRBITS; };
    enum { PTRBITS = PTRSIZE==2 ? 1 : PTRSIZE==4 ? 2 : 3 };

    char *p;
    size_t left;
    char *blocks;
    void *reuse[MAXBUCKETS];

    pool(void);
    ~pool(void) { dealloc_block(blocks); };

    void *alloc(size_t size);
    void dealloc(void *p, size_t size);
    void *realloc(void *p, size_t oldsize, size_t newsize);

    char *string(const char *s, size_t l);
    char *string(const char *s) { return string(s, strlen(s)); };
    void deallocstr(char *s) { dealloc(s, strlen(s)+1); };
    char *stringbuf(const char *s) { return string(s, _MAXDEFSTR-1); };

    void dealloc_block(void *b);
    void allocnext(size_t allocsize);
  };

  pool *gp();

  template <class T> struct vector : noncopyable
  {
    T *buf;
    int alen;
    int ulen;
    pool *p;

    INLINE vector(void)
    {
      this->p = gp();
      alen = 8;
      buf = (T *)p->alloc(alen*sizeof(T));
      ulen = 0;
    }
    INLINE vector(vector &&other)
    {
      this->buf = other.buf;
      this->alen = other.alen;
      this->ulen = other.ulen;
      this->p = other.p;
    }
    ~vector(void) { setsize(0); p->dealloc(buf, alen*sizeof(T)); }

    INLINE T &add(const T &x)
    {
      if(ulen==alen) realloc();
      new (&buf[ulen]) T(x);
      return buf[ulen++];
    }
    INLINE T &add(void)
    {
      if(ulen==alen) realloc();
      new (&buf[ulen]) T;
      return buf[ulen++];
    }
    INLINE T &pop(void) { return buf[--ulen]; }
    INLINE T &last(void) { return buf[ulen-1]; }
    INLINE bool empty(void) { return ulen==0; }
    INLINE int length(void) const { return ulen; }
    INLINE int size(void) const { return ulen*sizeof(T); }
    INLINE const T &operator[](int i) const { assert(i>=0 && i<ulen); return buf[i]; }
    INLINE T &operator[](int i) { assert(i>=0 && i<ulen); return buf[i]; }
    INLINE T *getbuf(void) { return buf; }
    void setsize(int i) { for(; ulen>i; ulen--) buf[ulen-1].~T(); }
    void sort(void *cf)
    {
      qsort(buf, ulen, sizeof(T), (int (__cdecl *)(const void *,const void *))cf);
    }
    void realloc(void)
    {
      const int olen = alen;
      buf = (T *)p->realloc(buf, olen*sizeof(T), (alen *= 2)*sizeof(T));
    }
    T remove(int i)
    {
      T e = buf[i];
      for(int p = i+1; p<ulen; p++) buf[p-1] = buf[p];
      ulen--;
      return e;
    }
    T &insert(int i, const T &e)
    {
      add(T());
      for(int p = ulen-1; p>i; p--) buf[p] = buf[p-1];
      buf[i] = e;
      return buf[i];
    }
  };

  template <class T> struct hashtable
  {
    struct chain { chain *next; const char *key; T data; };

    int size;
    int numelems;
    chain **table;
    pool *parent;
    chain *enumc;

    hashtable(void)
    {
      this->size = 1<<10;
      this->parent = gp();
      numelems = 0;
      table = (chain **)parent->alloc(size*sizeof(T));
      for(int i = 0; i<size; i++) table[i] = NULL;
    }

    hashtable(hashtable<T> &v);
    void operator=(hashtable<T> &v);

    T *access(const char *key, const T *data = NULL)
    {
      unsigned int h = 5381;
      for(int i = 0, k; (k = key[i]) != 0; i++) h = ((h<<5)+h)^k; // bernstein k=33 xor
      h = h&(size-1); // primes not much of an advantage
      for(chain *c = table[h]; c; c = c->next) {
        char ch = 0;
        for(const char *p1 = key, *p2 = c->key; (ch = *p1++)==*p2++; ) if(!ch) {
          T *d = &c->data;
          if(data) c->data = *data;
          return d;
        }
      }
      if(data) {
        chain *c = (chain *)parent->alloc(sizeof(chain));
        c->data = *data;
        c->key = key;
        c->next = table[h];
        table[h] = c;
        numelems++;
      }
      return NULL;
    }
  };

#define enumerate(ht,t,e,b) loopi(ht->size)\
    for(ht->enumc = ht->table[i]; ht->enumc; ht->enumc = ht->enumc->next) {\
      t e = &ht->enumc->data; b;\
    }

  INLINE char *newstring(const char *s)           { return gp()->string(s); }
  INLINE char *newstring(const char *s, size_t l) { return gp()->string(s, l); }
  INLINE char *newstringbuf(const char *s)        { return gp()->stringbuf(s); }

#define ARRAY_ELEM_N(X) (sizeof(X) / sizeof(X[0]))

  /* simplistic vector ops XXX remove */
#define dotprod(u,v) ((u).x * (v).x + (u).y * (v).y + (u).z * (v).z)
#define vmul(u,f)    { (u).x *= (f); (u).y *= (f); (u).z *= (f); }
#define vdiv(u,f)    { (u).x /= (f); (u).y /= (f); (u).z /= (f); }
#define vadd(u,v)    { (u).x += (v).x; (u).y += (v).y; (u).z += (v).z; };
#define vsub(u,v)    { (u).x -= (v).x; (u).y -= (v).y; (u).z -= (v).z; };
#define vdist(d,v,e,s) vec v = s; vsub(v,e); float d = (float)sqrt(dotprod(v,v));
#define vreject(v,u,max) ((v).x>(u).x+(max) || (v).x<(u).x-(max) || (v).y>(u).y+(max) || (v).y<(u).y-(max))
#define vlinterp(v,f,u,g) { (v).x = (v).x*f+(u).x*g; (v).y = (v).y*f+(u).y*g; (v).z = (v).z*f+(u).z*g; }

  /* XXX REMOVE */
  struct vec {
    INLINE vec(void) {}
    INLINE vec(float x, float y, float z) : x(x),y(y),z(z) {}
    float x, y, z;
  };

  /* vertex array format */
  struct vertex { float u, v, x, y, z; uchar r, g, b, a; };
  typedef vector<char *> cvector;
  typedef vector<int> ivector;

  void fatal(const char *s, const char *o = "");
  void *alloc(int s);
  void keyrepeat(bool on);

  static const int KB = 1024;
  static const int MB = KB*KB;

} /* namespace cube */

#include "math.hpp"
#endif /* __CUBE_TOOLS_HPP__ */

