#ifndef __CUBE_RENDERER_HPP__
#define __CUBE_RENDERER_HPP__

/*! XXX move that to namespace */
extern int xtraverts;

namespace rdr
{
  /* virtual screen width for text & HUD */
  static const int VIRTW = 2400;
  /* virtual screen height for text & HUD */
  static const int VIRTH = 1800;
  /* font height */
  static const int FONTH = 64;
  /* tabulation size in pixels */
  static const int PIXELTAB = VIRTW / 12;

  /* rendergl */
  namespace ogl
  {
    enum {POS0=0,POS1=1,TEX=2,NOR=3}; /* attributes */
    void init(int w, int h);
    void clean(void);
    void drawframe(int w, int h, float curfps);
    bool installtex(int id, const char *name, int &xs, int &ys, bool clamp = false);
    void vertf(float v1, float v2, float v3, sqr *ls, float t1, float t2);
    void addstrip(int tex, int start, int n);
    int lookuptex(int tex, int &xs, int &ys);
    void drawarray(int mode, size_t pos, size_t tex, size_t n, const float *data);
    void rendermd2(const float *pos0, const float *pos1, float lerp, int n);
  } /* namespace ogl */

  /* rendercubes */
  void resetcubes(void);
  void render_flat(int tex, int x, int y, int size, int h, sqr *l1, sqr *l2, sqr *l3, sqr *l4, bool isceil);
  void render_flatdelta(int wtex, int x, int y, int size, float h1, float h2, float h3, float h4, sqr *l1, sqr *l2, sqr *l3, sqr *l4, bool isceil);
  void render_square(int wtex, float floor1, float floor2, float ceil1, float ceil2, int x1, int y1, int x2, int y2, int size, sqr *l1, sqr *l2, bool topleft);
  void render_tris(int x, int y, int size, bool topleft, sqr *h1, sqr *h2, sqr *s, sqr *t, sqr *u, sqr *v);
  void addwaterquad(int x, int y, int size);
  int renderwater(float hf);
  void finishstrips(void);
  void setarraypointers(void);
  void mipstats(int a, int b, int c);

  /* rendertext */
  void draw_text(const char *str, int left, int top, int gl_num);
  void draw_textf(const char *fstr, int left, int top, int gl_num, ...);
  int text_width(const char *str);
  void draw_envbox(int t, int fogdist);

  /* renderextras */
  void line(int x1, int y1, float z1, int x2, int y2, float z2);
  void box(const block &b, float z1, float z2, float z3, float z4);
  void dot(int x, int y, float z);
  void linestyle(float width, int r, int g, int b);
  void newsphere(vec &o, float max, int type);
  void renderspheres(int time);
  void drawhud(int w, int h, int curfps, int nquads, int curvert, bool underwater);
  void readdepth(int w, int h);
  void blendbox(int x1, int y1, int x2, int y2, bool border);
  void damageblend(int n);
  void renderents(void);

  /* renderparticles */
  void setorient(vec &r, vec &u);
  void particle_splash(int type, int num, int fade, vec &p);
  void particle_trail(int type, int fade, vec &from, vec &to);
  void render_particles(int time);

  /* rendermd2 */
  void rendermodel(const char *mdl, int frame, int range, int tex, float rad, float x, float y, float z, float yaw, float pitch, bool teammate, float scale, float speed, int snap = 0, int basetime = 0);
  mapmodelinfo &getmminfo(int i);
} /* namespace rdr */

#endif /* __CUBE_RENDERER_HPP__ */

