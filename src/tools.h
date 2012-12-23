// generic useful stuff for any C++ program

#ifndef __CUBE_TOOLS_HPP__
#define __CUBE_TOOLS_HPP__

#ifdef __GNUC__
#define gamma __gamma
#endif

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
#ifdef __GNUC__
#include <new>
#else
#include <new.h>
#endif

#ifdef NULL
#undef NULL
#endif
#define NULL 0

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define rnd(max) (rand()%(max))
#define rndreset() (srand(1))
#define rndtime() { loopi(lastmillis&0xF) rnd(i+1); }
#define loop(v,m) for(int v = 0; v<(m); v++)
#define loopi(m) loop(i,m)
#define loopj(m) loop(j,m)
#define loopk(m) loop(k,m)
#define loopl(m) loop(l,m)

#ifdef WIN32
#pragma warning( 3 : 4189 )
#define PATHDIV '\\'
#else
#define __cdecl
#define _vsnprintf vsnprintf
#define PATHDIV '/'
#endif

// easy safe strings

#define _MAXDEFSTR 260
typedef char string[_MAXDEFSTR];

inline void strn0cpy(char *d, const char *s, size_t m) { strncpy(d,s,m); d[(m)-1] = 0; };
inline void strcpy_s(char *d, const char *s) { strn0cpy(d,s,_MAXDEFSTR); };
inline void strcat_s(char *d, const char *s) { size_t n = strlen(d); strn0cpy(d+n,s,_MAXDEFSTR-n); };

inline void formatstring(char *d, const char *fmt, va_list v)
{
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
    };
};

#define sprintf_s(d) sprintf_s_f((char *)d)
#define sprintf_sd(d) string d; sprintf_s(d)
#define sprintf_sdlv(d,last,fmt) string d; { va_list ap; va_start(ap, last); formatstring(d, fmt, ap); va_end(ap); }
#define sprintf_sdv(d,fmt) sprintf_sdlv(d,fmt,fmt)
#define ATOI(s) strtol(s, NULL, 0) // supports hexadecimal numbers

#define fast_f2nat(val) ((int)(val))

extern char *path(char *s);
extern char *loadfile(char *fn, int *size);
extern void endianswap(void *, int, int);

#define PI  (3.1415927f)
#define PI2 (2*PI)

// memory pool that uses buckets and linear allocation for small objects
// VERY fast, and reasonably good memory reuse
struct pool
{
  enum { POOLSIZE = 4096 };   // can be absolutely anything
  enum { PTRSIZE = sizeof(char *) };
  enum { MAXBUCKETS = 65 };   // meaning up to size 256 on 32bit pointer systems
  enum { MAXREUSESIZE = MAXBUCKETS*PTRSIZE-PTRSIZE };
  inline size_t bucket(size_t s) { return (s+PTRSIZE-1)>>PTRBITS; };
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

template <class T> struct vector
{
  T *buf;
  int alen;
  int ulen;
  pool *p;

  vector(void)
  {
    this->p = gp();
    alen = 8;
    buf = (T *)p->alloc(alen*sizeof(T));
    ulen = 0;
  }

  ~vector(void) { setsize(0); p->dealloc(buf, alen*sizeof(T)); }

  vector(vector<T> &v);
  void operator=(vector<T> &v);

  T &add(const T &x)
  {
    if(ulen==alen) realloc();
    new (&buf[ulen]) T(x);
    return buf[ulen++];
  }

  T &add()
  {
    if(ulen==alen) realloc();
    new (&buf[ulen]) T;
    return buf[ulen++];
  }

  T &pop(void) { return buf[--ulen]; }
  T &last(void) { return buf[ulen-1]; }
  bool empty(void) { return ulen==0; }

  int length(void) { return ulen; }
  T &operator[](int i) { assert(i>=0 && i<ulen); return buf[i]; }
  void setsize(int i) { for(; ulen>i; ulen--) buf[ulen-1].~T(); }
  T *getbuf(void) { return buf; }

  void sort(void *cf) {
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

#define loopv(v)    if(false) {} else for(int i = 0; i<(v).length(); i++)
#define loopvrev(v) if(false) {} else for(int i = (v).length()-1; i>=0; i--)

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

inline char *newstring(const char *s)           { return gp()->string(s); }
inline char *newstring(const char *s, size_t l) { return gp()->string(s, l); }
inline char *newstringbuf(const char *s)        { return gp()->stringbuf(s); }

/* simplistic vector ops */
#define dotprod(u,v) ((u).x * (v).x + (u).y * (v).y + (u).z * (v).z)
#define vmul(u,f)    { (u).x *= (f); (u).y *= (f); (u).z *= (f); }
#define vdiv(u,f)    { (u).x /= (f); (u).y /= (f); (u).z /= (f); }
#define vadd(u,v)    { (u).x += (v).x; (u).y += (v).y; (u).z += (v).z; };
#define vsub(u,v)    { (u).x -= (v).x; (u).y -= (v).y; (u).z -= (v).z; };
#define vdist(d,v,e,s) vec v = s; vsub(v,e); float d = (float)sqrt(dotprod(v,v));
#define vreject(v,u,max) ((v).x>(u).x+(max) || (v).x<(u).x-(max) || (v).y>(u).y+(max) || (v).y<(u).y-(max))
#define vlinterp(v,f,u,g) { (v).x = (v).x*f+(u).x*g; (v).y = (v).y*f+(u).y*g; (v).z = (v).z*f+(u).z*g; }

struct vec { float x, y, z; };

/* convenient variable size float vector */
template <int n> struct vvec {
  template <typename... T> inline vvec(T... args) { this->set(0,args...); }
  template <typename First, typename... Rest>
  inline void set(int index, First first, Rest... rest) {
    this->v[index] = first;
    set(index+1,rest...);
  }
  inline void set(int index) {}
  float &operator[] (int index) { return v[index]; }
  const float &operator[] (int index) const { return v[index]; }
  float v[n];
};

/* simplistic matrix ops */
void perspective(double m[16], double fovy, double aspect, double zNear, double zFar);
void mul(const double a[16], const double b[16], double r[16]);
int invert(const double m[16], double invOut[16]);
int unproject(double winx, double winy, double winz,
              const double model[16],
              const double proj[16],
              const int vp[4],
              double *objx, double *objy, double *objz);

#define v4mul(m, in, out) do { loopi(4)\
  out[i]=in[0]*m[0*4+i] + in[1]*m[1*4+i] + in[2]*m[2*4+i] + in[3]*m[3*4+i];\
} while (0)

/* vertex array format */
struct vertex { float u, v, x, y, z; uchar r, g, b, a; };
typedef vector<char *> cvector;
typedef vector<int> ivector;

/* OpenGL debug function */
#ifndef NDEBUG
#define OGL(NAME, ...) \
  do { \
    gl##NAME(__VA_ARGS__); \
    if (glGetError()) fatal("gl" #NAME " failed"); \
  } while (0)
#define OGLR(RET, NAME, ...) \
  do { \
    RET = gl##NAME(__VA_ARGS__); \
    if (glGetError()) fatal("gl" #NAME " failed"); \
  } while (0)
#else
  #define OGL(NAME, ...) \
  do { \
    gl##NAME->NAME(__VA_ARGS__); \
  } while (0)
  #define OGLR(RET, NAME, ...) \
  do { \
    RET = gl##NAME->NAME(__VA_ARGS__); \
  } while (0)
#endif /* NDEBUG */

void fatal(const char *s, const char *o = "");
void *alloc(int s);
void keyrepeat(bool on);

#endif /* __CUBE_TOOLS_HPP__ */

