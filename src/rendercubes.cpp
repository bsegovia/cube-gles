#include "cube.h"
#include "ogl.hpp"
#include <GL/gl.h>

namespace rdr
{
  vertex *verts = NULL;
  int curvert;
  int curmaxverts = 10000;

  void setarraypointers(void)
  {
    OGL(VertexAttribPointer, ogl::POS0, 3, GL_FLOAT, 0, sizeof(vertex), &verts[0].x);
    OGL(VertexAttribPointer, ogl::COL, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(vertex), &verts[0].r);
    OGL(VertexAttribPointer, ogl::TEX, 2, GL_FLOAT, 0, sizeof(vertex), &verts[0].u);
  }

  void reallocv(void)
  {
    verts = (vertex *)realloc(verts, (curmaxverts *= 2)*sizeof(vertex));
    curmaxverts -= 10;
    if (!verts) fatal("no vertex memory!");
    setarraypointers();
  }

/* generating the actual vertices is done dynamically every frame and sits at
 * the leaves of all these functions, and are part of the cpu bottleneck on
 * really slow machines, hence the macros
 */
#define vertcheck() { if (curvert>=curmaxverts) reallocv(); }

#define vertf(v1, v2, v3, ls, t1, t2) { vertex &v = verts[curvert++]; \
      v.u = t1; v.v = t2; \
      v.x = v1; v.y = v2; v.z = v3; \
      v.r = ls->r; v.g = ls->g; v.b = ls->b; v.a = 255; }

#define vert(v1, v2, v3, ls, t1, t2) { vertf((float)(v1), (float)(v2), (float)(v3), ls, t1, t2); }

  static const float TEXTURESCALE = 32.0f;
  static int nquads;
  static bool floorstrip = false, deltastrip = false;
  static int oh, oy, ox, ogltex; /* the o* vars are used by the stripification */
  static int ol3r, ol3g, ol3b, ol4r, ol4g, ol4b;
  static int firstindex;
  static bool showm = false;

  void showmip() { showm = !showm; }
  void mipstats(int a, int b, int c)
  {
    if (showm) console::out("1x1/2x2/4x4: %d / %d / %d", a, b, c);
  }

  COMMAND(showmip, ARG_NONE);

#define stripend() {\
  if (floorstrip || deltastrip) {\
    ogl::addstrip(ogltex, firstindex, curvert-firstindex);\
    floorstrip = deltastrip = false;\
  }\
}

  void finishstrips(void) { stripend(); }

  sqr sbright, sdark;
  VAR(lighterror,1,8,100);

  void render_flat(int wtex, int x, int y, int size, int h, sqr *l1, sqr *l2, sqr *l3, sqr *l4, bool isceil)  // floor/ceil quads
  {
    vertcheck();
    if (showm) { l3 = l1 = &sbright; l4 = l2 = &sdark; }

    int sx, sy;
    const int gltex = ogl::lookuptex(wtex, sx, sy);
    const float xf = TEXTURESCALE/sx;
    const float yf = TEXTURESCALE/sy;
    const float xs = size*xf;
    const float ys = size*yf;
    const float xo = xf*x;
    const float yo = yf*y;
    const bool first = !floorstrip || y!=oy+size || ogltex!=gltex || h!=oh || x!=ox;

    if (first) { /* start strip here */
      stripend();
      firstindex = curvert;
      ogltex = gltex;
      oh = h;
      ox = x;
      floorstrip = true;
      if (isceil) {
        vert(x+size, h, y, l2, xo+xs, yo);
        vert(x,      h, y, l1, xo, yo);
      } else {
        vert(x,      h, y, l1, xo,    yo);
        vert(x+size, h, y, l2, xo+xs, yo);
      }
      ol3r = l1->r;
      ol3g = l1->g;
      ol3b = l1->b;
      ol4r = l2->r;
      ol4g = l2->g;
      ol4b = l2->b;
    } else { /* continue strip */
      const int lighterr = lighterror*2;
      /* skip vertices if light values are close enough */
      if ((abs(ol3r-l3->r)<lighterr && abs(ol4r-l4->r)<lighterr
            &&  abs(ol3g-l3->g)<lighterr && abs(ol4g-l4->g)<lighterr
            &&  abs(ol3b-l3->b)<lighterr && abs(ol4b-l4->b)<lighterr) || !wtex) {
        curvert -= 2;
        nquads--;
      } else {
        uchar *p3 = (uchar *)(&verts[curvert-1].r);
        ol3r = p3[0];
        ol3g = p3[1];
        ol3b = p3[2];
        uchar *p4 = (uchar *)(&verts[curvert-2].r);
        ol4r = p4[0];
        ol4g = p4[1];
        ol4b = p4[2];
      }
    }

    if (isceil) {
      vert(x+size, h, y+size, l3, xo+xs, yo+ys);
      vert(x,      h, y+size, l4, xo,    yo+ys);
    } else {
      vert(x,      h, y+size, l4, xo,    yo+ys);
      vert(x+size, h, y+size, l3, xo+xs, yo+ys);
    }

    oy = y;
    nquads++;
  }

  void render_flatdelta(int wtex, int x, int y, int size, float h1, float h2, float h3, float h4, sqr *l1, sqr *l2, sqr *l3, sqr *l4, bool isceil)  // floor/ceil quads on a slope
  {
    vertcheck();
    if (showm) { l3 = l1 = &sbright; l4 = l2 = &sdark; }

    int sx, sy;
    const int gltex = ogl::lookuptex(wtex, sx, sy);
    const float xf = TEXTURESCALE/sx;
    const float yf = TEXTURESCALE/sy;
    const float xs = size*xf;
    const float ys = size*yf;
    const float xo = xf*x;
    const float yo = yf*y;
    const bool first = !deltastrip || y!=oy+size || ogltex!=gltex || x!=ox;

    if (first)
    {
      stripend();
      firstindex = curvert;
      ogltex = gltex;
      ox = x;
      deltastrip = true;
      if (isceil) {
        vertf((float)x+size, h2, (float)y,      l2, xo+xs, yo);
        vertf((float)x,      h1, (float)y,      l1, xo,    yo);
      } else {
        vertf((float)x,      h1, (float)y,      l1, xo,    yo);
        vertf((float)x+size, h2, (float)y,      l2, xo+xs, yo);
      }
      ol3r = l1->r;
      ol3g = l1->g;
      ol3b = l1->b;
      ol4r = l2->r;
      ol4g = l2->g;
      ol4b = l2->b;
    }

    if (isceil) {
      vertf((float)x+size, h3, (float)y+size, l3, xo+xs, yo+ys);
      vertf((float)x,      h4, (float)y+size, l4, xo,    yo+ys);
    } else {
      vertf((float)x,      h4, (float)y+size, l4, xo,    yo+ys);
      vertf((float)x+size, h3, (float)y+size, l3, xo+xs, yo+ys);
    }

    oy = y;
    nquads++;
  }

  void render_2tris(sqr *h, sqr *s, int x1, int y1, int x2, int y2, int x3, int y3, sqr *l1, sqr *l2, sqr *l3)   // floor/ceil tris on a corner cube
  {
      stripend();
      vertcheck();

      int sx, sy;
      int gltex = ogl::lookuptex(h->ftex, sx, sy);
      float xf = TEXTURESCALE/sx;
      float yf = TEXTURESCALE/sy;

      vertf((float)x1, h->floor, (float)y1, l1, xf*x1, yf*y1);
      vertf((float)x2, h->floor, (float)y2, l2, xf*x2, yf*y2);
      vertf((float)x3, h->floor, (float)y3, l3, xf*x3, yf*y3);
      ogl::addstrip(gltex, curvert-3, 3);

      gltex = ogl::lookuptex(h->ctex, sx, sy);
      xf = TEXTURESCALE/sx;
      yf = TEXTURESCALE/sy;

      vertf((float)x3, h->ceil, (float)y3, l3, xf*x3, yf*y3);
      vertf((float)x2, h->ceil, (float)y2, l2, xf*x2, yf*y2);
      vertf((float)x1, h->ceil, (float)y1, l1, xf*x1, yf*y1);
      ogl::addstrip(gltex, curvert-3, 3);
      nquads++;
  }

  void render_tris(int x, int y, int size, bool topleft,
                   sqr *h1, sqr *h2, sqr *s, sqr *t, sqr *u, sqr *v)
  {
    if (topleft) {
      if (h1) render_2tris(h1, s, x+size, y+size, x, y+size, x, y, u, v, s);
      if (h2) render_2tris(h2, s, x, y, x+size, y, x+size, y+size, s, t, v);
    } else {
      if (h1) render_2tris(h1, s, x, y, x+size, y, x, y+size, s, t, u);
      if (h2) render_2tris(h2, s, x+size, y, x+size, y+size, x, y+size, t, u, v);
    }
  }

  void render_square(int wtex, float floor1, float floor2, float ceil1, float ceil2, int x1, int y1, int x2, int y2, int size, sqr *l1, sqr *l2, bool flip)   // wall quads
  {
    stripend();
    vertcheck();
    if (showm) { l1 = &sbright; l2 = &sdark; }

    int sx, sy;
    const int gltex = ogl::lookuptex(wtex, sx, sy);
    const float xf = TEXTURESCALE/sx;
    const float yf = TEXTURESCALE/sy;
    const float xs = size*xf;
    const float xo = xf*(x1==x2 ? min(y1,y2) : min(x1,x2));

    if (!flip) {
      vertf((float)x2, ceil2,  (float)y2, l2, xo+xs, -yf*ceil2);
      vertf((float)x1, ceil1,  (float)y1, l1, xo,    -yf*ceil1);
      vertf((float)x2, floor2, (float)y2, l2, xo+xs, -floor2*yf);
      vertf((float)x1, floor1, (float)y1, l1, xo,    -floor1*yf);
    } else {
      vertf((float)x1, ceil1,  (float)y1, l1, xo,    -yf*ceil1);
      vertf((float)x2, ceil2,  (float)y2, l2, xo+xs, -yf*ceil2);
      vertf((float)x1, floor1, (float)y1, l1, xo,    -floor1*yf);
      vertf((float)x2, floor2, (float)y2, l2, xo+xs, -floor2*yf);
    }

    nquads++;
    ogl::addstrip(gltex, curvert-4, 4);
  }

  static int wx1, wy1, wx2, wy2; /* water bounding rectangle */
  static bool watervbobuilt = false;
  static GLuint watervbo = 0u, wateribo = 0u;
  static uint watervertn = 0u;

  VARF(watersubdiv, 1, 4, 64, watervbobuilt = false)
  VARF(waterlevel, -128, -128, 127, if (!edit::noteditmode()) hdr.waterlevel = waterlevel);

  INLINE void vertw(int v1, float v2, int v3, sqr *c, float t1, float t2, float t)
  {
    vertcheck();
    vertf(float(v1), v2, float(v3), c, t1, t2);
  }

  static void buildwatervbo(const vec2f &xyf)
  {
    //static int num = 0;
    wx1 &= ~(watersubdiv-1);
    wy1 &= ~(watersubdiv-1);
    int xn = (wx2-wx1)/watersubdiv;
    int yn = (wy2-wy1)/watersubdiv;
    xn += (wx2-wx1)%watersubdiv?1:0;
    yn += (wy2-wy1)%watersubdiv?1:0;
//    printf("dd %i\n", ++num);

    /* build vertices and upload them to the vbo */
    vvec<5> *vertices = new vvec<5>[(xn+1)*(yn+1)];
    int vertn = 0;
    yn = 0;
    for (int yy=wy1; yy<=wy2; yy+=watersubdiv, ++yn) {
      xn = 0;
      for (int xx=wx1; xx<=wx2; xx+=watersubdiv, ++vertn, ++xn) {
        const float x = float(xx), y = float(yy);
        const float u = x*xyf.x, v = y*xyf.y;
        vertices[vertn] = vvec<5>(u,v,x,0.f,y);
      }
    }
    //assert(vertn == (xn+1)*(yn+1));
    if (watervbo == 0u) OGL(GenBuffers, 1, &watervbo);
    OGL(BindBuffer, GL_ARRAY_BUFFER, watervbo);
    OGL(BufferData, GL_ARRAY_BUFFER, vertn*sizeof(vvec<5>), &vertices[0], GL_STATIC_DRAW);
    OGL(BindBuffer, GL_ARRAY_BUFFER, 0);
    delete [] vertices;

    /* build the index buffer */
    vector<uint> indices;
    for (int yy=wy1, yyi=0; yy<wy2; yy+=watersubdiv, ++yyi) {
      const int row0=yyi*xn, row1=(yyi+1)*xn;
      const int twotriangles[]={row0,row0+1,row1,row0+1,row1+1,row1};
      for (int xx=wx1, xxi=0; xx<wx2; xx+=watersubdiv, ++xxi)
        loopi(6) indices.add(twotriangles[i]+xxi);
    }
    if (wateribo == 0u) OGL(GenBuffers, 1, &wateribo);
    OGL(BindBuffer, GL_ELEMENT_ARRAY_BUFFER, wateribo);
    OGL(BufferData, GL_ELEMENT_ARRAY_BUFFER, indices.length()*sizeof(uint), &indices[0], GL_STATIC_DRAW);
    OGL(BindBuffer, GL_ELEMENT_ARRAY_BUFFER, 0);

    /* we are done */
    watervertn = indices.length();
    watervbobuilt = true;
  }
#if 1
  /* renders water for bounding rect area that contains water... simple but very
   * inefficient
   */
  int renderwater(float hf, uint uxyf)
  {
    if (wx1<0) return nquads;

    OGL(DepthMask, GL_FALSE);
    OGL(Enable, GL_BLEND);
    OGL(BlendFunc, GL_ONE, GL_SRC_COLOR);
    int sx, sy;
    OGL(BindTexture, GL_TEXTURE_2D, ogl::lookuptex(DEFAULT_LIQUID, sx, sy));

    wx1 &= ~(watersubdiv-1);
    wy1 &= ~(watersubdiv-1);

    const float xf = TEXTURESCALE/sx;
    const float yf = TEXTURESCALE/sy;
    const float xs = watersubdiv*xf;
    const float ys = watersubdiv*yf;
    const float t1 = lastmillis/300.0f;
    const float t2 = lastmillis/4000.0f;

    //if (!watervbobuilt) buildwatervbo(vec2f(xf,yf));

    sqr dl;
    dl.r = dl.g = dl.b = 255;

    const vec2f xyf(xf*t2, yf*t2);
    OGL(Uniform2fv, uxyf, 1, &xyf.x);
    for (int xx = wx1; xx<wx2; xx += watersubdiv) {
      for (int yy = wy1; yy<wy2; yy += watersubdiv) {
        const float xo = xf*xx;
        const float yo = yf*yy;
        if (yy==wy1) {
          vertw(xx,             0.f, yy,             &dl, xo,    yo, t1);
          vertw(xx+watersubdiv, 0.f, yy,             &dl, xo+xs, yo, t1);
        }
        vertw(xx,             0.f, yy+watersubdiv, &dl, xo,    yo+ys, t1);
        vertw(xx+watersubdiv, 0.f, yy+watersubdiv, &dl, xo+xs, yo+ys, t1);
      }
      int n = (wy2-wy1-1)/watersubdiv;
      nquads += n;
      n = (n+2)*2;
      ogl::drawarrays(GL_TRIANGLE_STRIP, curvert -= n, n);
    }
    OGL(Disable, GL_BLEND);
    OGL(DepthMask, GL_TRUE);

    return nquads;
  }
#else
  /* renders water for bounding rect area that contains water... simple but very
   * inefficient
   */
  int renderwater(float hf, uint uxyf)
  {
    if (wx1<0) return nquads;

    OGL(DepthMask, GL_FALSE);
    OGL(Enable, GL_BLEND);
    OGL(BlendFunc, GL_ONE, GL_SRC_COLOR);
    int sx, sy;
    OGL(BindTexture, GL_TEXTURE_2D, ogl::lookuptex(DEFAULT_LIQUID, sx, sy));

    wx1 &= ~(watersubdiv-1);
    wy1 &= ~(watersubdiv-1);

    const float xf = TEXTURESCALE/sx;
    const float yf = TEXTURESCALE/sy;
    const float t2 = lastmillis/4000.0f;
    if (!watervbobuilt) buildwatervbo(vec2f(xf,yf));

    const vec2f xyf(xf*t2, yf*t2);
    OGL(Uniform2fv, uxyf, 1, &xyf.x);
    OGL(DisableVertexAttribArray, ogl::COL);
    OGL(EnableVertexAttribArray, ogl::POS0);
    OGL(EnableVertexAttribArray, ogl::TEX);
    OGL(BindBuffer, GL_ARRAY_BUFFER, watervbo);
    OGL(VertexAttribPointer, ogl::POS0, 3, GL_FLOAT, 0, sizeof(float[5]), (void*) (2*sizeof(float)));
    OGL(VertexAttribPointer, ogl::TEX, 2, GL_FLOAT, 0, sizeof(float[5]), NULL);
    OGL(BindBuffer, GL_ELEMENT_ARRAY_BUFFER, wateribo);
    OGL(VertexAttrib3f,ogl::COL,1.f,1.f,1.f);
    ogl::drawelements(GL_TRIANGLES, watervertn, GL_UNSIGNED_INT, NULL);
    OGL(BindBuffer, GL_ARRAY_BUFFER, 0);
    OGL(BindBuffer, GL_ELEMENT_ARRAY_BUFFER, 0);
    OGL(Disable, GL_BLEND);
    OGL(DepthMask, GL_TRUE);

    return nquads;
  }

#endif
  /* update bounding rect that contains water */
  void addwaterquad(int x, int y, int size)
  {
    const int x2 = x+size;
    const int y2 = y+size;
    if (wx1<0) {
      wx1 = x;
      wy1 = y;
      wx2 = x2;
      wy2 = y2;
      watervbobuilt = false;
    } else {
      if (x<wx1) {wx1 = x; watervbobuilt = false; }
      if (y<wy1) {wy1 = y; watervbobuilt = false; }
      if (x2>wx2) {wx2 = x2; watervbobuilt = false; }
      if (y2>wy2) {wy2 = y2; watervbobuilt = false; }
    }
  }

  void resetcubes(void)
  {
    if (!verts) reallocv();
    floorstrip = deltastrip = false;
//    if (watervbo) {OGL(DeleteBuffers,1,&watervbo); watervbo=0u;}
//    if (wateribo) {OGL(DeleteBuffers,1,&wateribo); wateribo=0u;}
//    watervbobuilt = false;
    wx1 = -1;
    nquads = 0;
    sbright.r = sbright.g = sbright.b = 255;
    sdark.r = sdark.g = sdark.b = 0;
  }
} /* namespace rdr */

