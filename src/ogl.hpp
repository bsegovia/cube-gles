#ifndef __CUBE_OGL_HPP__
#define __CUBE_OGL_HPP__

namespace ogl
{
  enum {POS0=0, POS1=1, TEX=2, NOR=3, COL=4}; /* vertex attributes */

  /* quick, dirty and super simple uber-shader system */
  static const uint COLOR_ONLY = 0;
  static const uint FOG = 1<<0;
  static const uint KEYFRAME = 1<<1;
  static const uint DIFFUSETEX = 1<<2;
  static const int subtypen = 3;
  static const int shadern = 1<<subtypen;
  void bindshader(uint flags);

  void init(int w, int h);
  void clean(void);
  void drawframe(int w, int h, float curfps);
  bool installtex(int id, const char *name, int &xs, int &ys, bool clamp = false);
  void vertf(float v1, float v2, float v3, sqr *ls, float t1, float t2);
  void addstrip(int tex, int start, int n);
  int lookuptex(int tex, int &xs, int &ys);
  void draw(int mode, int pos, int tex, size_t n, const float *data);
  void drawarrays(int mode, int first, int count);
  void drawelements(int mode, int count, int type, const void *indices);
  void rendermd2(const float *pos0, const float *pos1, float lerp, int n);
  void drawsphere(void);

  /* matrix interface (mostly OGL like) */
  enum {MODELVIEW=0, PROJECTION=1, MATRIX_MODE=2};
  void matrixmode(int mode);
  void identity(void);
  void rotate(float angle, const vec3f &axis);
  void perspective(const mat4x4f &m, float fovy, float aspect, float znear, float zfar);
  void translate(const vec3f &v);
  void mulmatrix(const mat4x4f &m);
  void pushmatrix(void);
  void popmatrix(void);
  void loadmatrix(const mat4x4f &m);
  void ortho(float left, float right, float bottom, float top, float znear, float zfar);
  void scale(const vec3f &s);
  const mat4x4f &matrix(int mode);

  extern int xtraverts; /* number of transformed vertices */
} /* namespace ogl */

#endif /* __CUBE_OGL_HPP__ */

