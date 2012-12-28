#include "tools.h"
#include <new>

pool::pool(void)
{
  blocks = 0;
  allocnext(POOLSIZE);
  for (int i = 0; i<MAXBUCKETS; i++) reuse[i] = NULL;
}

void *pool::alloc(size_t size)
{
  if (size>MAXREUSESIZE)
    return malloc(size);
  else {
    size = bucket(size);
    void **r = (void **)reuse[size];
    if (r) {
      reuse[size] = *r;
      return (void *)r;
    } else {
      size <<= PTRBITS;
      if (left<size) allocnext(POOLSIZE);
      char *r = p;
      p += size;
      left -= size;
      return r;
    }
  }
}

void pool::dealloc(void *p, size_t size)
{
  if (size>MAXREUSESIZE)
    free(p);
  else {
    size = bucket(size);
    if (size) {    // only needed for 0-size free, are there any?
      *((void **)p) = reuse[size];
      reuse[size] = p;
    }
  }
}

void *pool::realloc(void *p, size_t oldsize, size_t newsize)
{
  void *np = alloc(newsize);
  if (!oldsize) return np;
  memcpy(np, p, newsize>oldsize ? oldsize : newsize);
  dealloc(p, oldsize);
  return np;
}

void pool::dealloc_block(void *b)
{
  if (b) {
    dealloc_block(*((char **)b));
    free(b);
  }
}

void pool::allocnext(size_t allocsize)
{
  char *b = (char *)malloc(allocsize+PTRSIZE);
  *((char **)b) = blocks;
  blocks = b;
  p = b+PTRSIZE;
  left = allocsize;
}

char *pool::string(const char *s, size_t l)
{
  char *b = (char *)alloc(l+1);
  strncpy(b,s,l);
  b[l] = 0;
  return b;
}

/* useful for global buffers that need to be initialisation
 * order independant
 */
pool *gp(void)
{
  static pool *p = NULL;
  return p ? p : (p = new pool());
}

char *path(char *s)
{
  for (char *t = s; (t = strpbrk(t, "/\\")) != 0; *t++ = PATHDIV);
  return s;
}

char *loadfile(char *fn, int *size)
{
  FILE *f = fopen(fn, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  unsigned int len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = (char *)malloc(len+1);
  if (!buf) return NULL;
  buf[len] = 0;
  size_t rlen = fread(buf, 1, len, f);
  fclose(f);
  if (len!=rlen || len<=0) {
    free(buf);
    return NULL;
  }
  if (size!=NULL) *size = len;
  return buf;
}

/* little indians as storage format */
void endianswap(void *memory, int stride, int length)
{
  if (*((char *)&stride)) return;
  loop(w, length) loop(i, stride/2) {
    uchar *p = (uchar *)memory+w*stride;
    uchar t = p[i];
    p[i] = p[stride-i-1];
    p[stride-i-1] = t;
  }
}

static void identity(double m[16])
{
  m[0+4*0] = 1; m[0+4*1] = 0; m[0+4*2] = 0; m[0+4*3] = 0;
  m[1+4*0] = 0; m[1+4*1] = 1; m[1+4*2] = 0; m[1+4*3] = 0;
  m[2+4*0] = 0; m[2+4*1] = 0; m[2+4*2] = 1; m[2+4*3] = 0;
  m[3+4*0] = 0; m[3+4*1] = 0; m[3+4*2] = 0; m[3+4*3] = 1;
}

void perspective(double m[16], double fovy, double aspect, double zNear, double zFar)
{
  double sine, cotangent, deltaZ;
  double radians = fovy / 2 * M_PI / 180;

  deltaZ = zFar - zNear;
  sine = sin(radians);
  identity(m);
  if ((deltaZ == 0) || (sine == 0) || (aspect == 0)) return;
  cotangent = cos(radians) / sine;
  m[4*0+0] = cotangent / aspect;
  m[4*1+1] = cotangent;
  m[4*2+2] = -(zFar + zNear) / deltaZ;
  m[4*2+3] = -1;
  m[4*3+2] = -2 * zNear * zFar / deltaZ;
  m[4*3+3] = 0;
}

void mul(const double a[16], const double b[16], double r[16])
{
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      r[i*4+j] = a[i*4+0]*b[0*4+j] +
                 a[i*4+1]*b[1*4+j] +
                 a[i*4+2]*b[2*4+j] +
                 a[i*4+3]*b[3*4+j];
}

/* Contributed by David Moore (See Mesa bug #6748) */
int invert(const double m[16], double invOut[16])
{
  double inv[16], det;
  inv[0] =   m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15]
           + m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
  inv[4] =  -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15]
           - m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
  inv[8] =   m[4]*m[9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15]
           + m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
  inv[12] = -m[4]*m[9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14]
           - m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
  inv[1] =  -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15]
           - m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
  inv[5] =   m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15]
           + m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
  inv[9] =  -m[0]*m[9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15]
           - m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
  inv[13] =  m[0]*m[9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14]
           + m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
  inv[2] =   m[1]*m[6]*m[15] - m[1]*m[7]*m[14] - m[5]*m[2]*m[15]
           + m[5]*m[3]*m[14] + m[13]*m[2]*m[7] - m[13]*m[3]*m[6];
  inv[6] =  -m[0]*m[6]*m[15] + m[0]*m[7]*m[14] + m[4]*m[2]*m[15]
           - m[4]*m[3]*m[14] - m[12]*m[2]*m[7] + m[12]*m[3]*m[6];
  inv[10] =  m[0]*m[5]*m[15] - m[0]*m[7]*m[13] - m[4]*m[1]*m[15]
           + m[4]*m[3]*m[13] + m[12]*m[1]*m[7] - m[12]*m[3]*m[5];
  inv[14] = -m[0]*m[5]*m[14] + m[0]*m[6]*m[13] + m[4]*m[1]*m[14]
           - m[4]*m[2]*m[13] - m[12]*m[1]*m[6] + m[12]*m[2]*m[5];
  inv[3] =  -m[1]*m[6]*m[11] + m[1]*m[7]*m[10] + m[5]*m[2]*m[11]
           - m[5]*m[3]*m[10] - m[9]*m[2]*m[7] + m[9]*m[3]*m[6];
  inv[7] =   m[0]*m[6]*m[11] - m[0]*m[7]*m[10] - m[4]*m[2]*m[11]
           + m[4]*m[3]*m[10] + m[8]*m[2]*m[7] - m[8]*m[3]*m[6];
  inv[11] = -m[0]*m[5]*m[11] + m[0]*m[7]*m[9] + m[4]*m[1]*m[11]
           - m[4]*m[3]*m[9] - m[8]*m[1]*m[7] + m[8]*m[3]*m[5];
  inv[15] =  m[0]*m[5]*m[10] - m[0]*m[6]*m[9] - m[4]*m[1]*m[10]
           + m[4]*m[2]*m[9] + m[8]*m[1]*m[6] - m[8]*m[2]*m[5];

  det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
  if (det == 0) return 0;
  det = 1.0 / det;
  for (int i = 0; i < 16; i++) invOut[i] = inv[i] * det;
  return 1;
}

int unproject(double winx, double winy, double winz,
              const double modelMatrix[16],
              const double projMatrix[16],
              const int viewport[4],
              double *objx, double *objy, double *objz)
{
  double finalMatrix[16];
  double in[4];
  double out[4];

  mul(modelMatrix, projMatrix, finalMatrix);
  if (!invert(finalMatrix, finalMatrix)) return 0;

  in[0]=winx;
  in[1]=winy;
  in[2]=winz;
  in[3]=1.0;

  /* Map x and y from window coordinates */
  in[0] = (in[0] - viewport[0]) / viewport[2];
  in[1] = (in[1] - viewport[1]) / viewport[3];

  /* Map to range -1 to 1 */
  in[0] = in[0] * 2 - 1;
  in[1] = in[1] * 2 - 1;
  in[2] = in[2] * 2 - 1;

  v4mul(finalMatrix, in, out);
  if (out[3] == 0.0) return 0;
  out[0] /= out[3];
  out[1] /= out[3];
  out[2] /= out[3];
  *objx = out[0];
  *objy = out[1];
  *objz = out[2];
  return 1;
}



