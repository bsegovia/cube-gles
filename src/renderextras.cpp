#include "cube.h"
#include "ogl.hpp"
#include <GL/gl.h>

// XXX
const char *entnames[] =
{
  "none?", "light", "playerstart",
  "shells", "bullets", "rockets", "riflerounds",
  "health", "healthboost", "greenarmour", "yellowarmour", "quaddamage",
  "teleport", "teledest",
  "mapmodel", "monster", "trigger", "jumppad",
  "?", "?", "?", "?", "?"
};

namespace rdr
{
  void line(int x1, int y1, float z1, int x2, int y2, float z2)
  {
    const vvec<3> verts[] =
    {
      vvec<3>(float(x1), z1, float(y1)),
      vvec<3>(float(x1), z1, float(y1)+0.01f),
      vvec<3>(float(x2), z2, float(y2)),
      vvec<3>(float(x2), z2, float(y2)+0.01f)
    };
    ogl::bindshader(ogl::COLOR_ONLY);
    ogl::immdraw(GL_TRIANGLE_STRIP, 3, 0, 0, 4, &verts[0][0]);
    ogl::xtraverts += 4;
  }

  void linestyle(float width, int r, int g, int b)
  {
    OGL(LineWidth, width);
    OGL(VertexAttrib3f,ogl::COL,float(r)/255.f,float(g)/255.f,float(b)/255.f);
  }

  void box(const block &b, float z1, float z2, float z3, float z4)
  {
    const vvec<3> verts[] =
    {
      vvec<3>(float(b.x),      z1, float(b.y)),
      vvec<3>(float(b.x+b.xs), z2, float(b.y)),
      vvec<3>(float(b.x+b.xs), z3, float(b.y+b.ys)),
      vvec<3>(float(b.x),      z4, float(b.y+b.ys))
    };
    ogl::bindshader(ogl::COLOR_ONLY);
    ogl::immdraw(GL_LINE_LOOP, 3, 0, 0, 4, &verts[0][0]);
    ogl::xtraverts += 4;
  }

  void dot(int x, int y, float z)
  {
    const float DOF = 0.1f;
    const vvec<3> verts[] =
    {
      vvec<3>(x-DOF, float(z), y-DOF),
      vvec<3>(x+DOF, float(z), y-DOF),
      vvec<3>(x+DOF, float(z), y+DOF),
      vvec<3>(x-DOF, float(z), y+DOF)
    };
    ogl::bindshader(ogl::COLOR_ONLY);
    ogl::immdraw(GL_LINE_LOOP, 3, 0, 0, 4, &verts[0][0]);
    ogl::xtraverts += 4;
  }

  void blendbox(int x1, int y1, int x2, int y2, bool border)
  {
    OGL(DepthMask, GL_FALSE);
    OGL(Disable, GL_TEXTURE_2D);
    OGL(BlendFunc, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
    ogl::bindshader(ogl::COLOR_ONLY);
    if (border)
      OGL(VertexAttrib3f, ogl::COL, .5f, .3f, .4f);
    else
      OGL(VertexAttrib3f, ogl::COL, 1.f, 1.f, 1.f);

    const vvec<2> verts0[] =
    {
      vvec<2>(float(x1), float(y1)),
      vvec<2>(float(x2), float(y1)),
      vvec<2>(float(x1), float(y2)),
      vvec<2>(float(x2), float(y2))
    };
    ogl::immdraw(GL_TRIANGLE_STRIP, 2, 0, 0, 4, &verts0[0][0]);

    OGL(Disable, GL_BLEND);
    OGL(VertexAttrib3f, ogl::COL, .2f, .7f, .4f);
    const vvec<2> verts1[] =
    {
      vvec<2>(float(x1), float(y1)),
      vvec<2>(float(x2), float(y1)),
      vvec<2>(float(x2), float(y2)),
      vvec<2>(float(x1), float(y2))
    };
    ogl::immdraw(GL_LINE_LOOP, 2, 0, 0, 4, &verts1[0][0]);

    ogl::xtraverts += 8;
    OGL(Enable, GL_BLEND);
    OGL(Enable, GL_TEXTURE_2D);
    OGL(DepthMask, GL_TRUE);
  }

  static const int MAXSPHERES = 50;
  struct sphere { vec o; float size, max; int type; sphere *next; };
  static sphere spheres[MAXSPHERES], *slist = NULL, *sempty = NULL;
  static bool sinit = false;

  void newsphere(vec &o, float max, int type)
  {
    if (!sinit) {
      loopi(MAXSPHERES) {
        spheres[i].next = sempty;
        sempty = &spheres[i];
      }
      sinit = true;
    }
    if (sempty) {
      sphere *p = sempty;
      sempty = p->next;
      p->o = o;
      p->max = max;
      p->size = 1;
      p->type = type;
      p->next = slist;
      slist = p;
    }
  }

  void renderspheres(int time)
  {
    OGL(DepthMask, GL_FALSE);
    OGL(Enable, GL_BLEND);
    OGL(BlendFunc, GL_SRC_ALPHA, GL_ONE);
    OGL(BindTexture, GL_TEXTURE_2D, 4);

    for (sphere *p, **pp = &slist; (p = *pp);) {
      const float size = p->size/p->max;
      ogl::pushmatrix();
      OGL(VertexAttrib4f, ogl::COL, 1.0f, 1.0f, 1.0f, 1.0f-size);
      ogl::translate(vec3f(p->o.x, p->o.z, p->o.y));
      ogl::rotate(lastmillis/5.0f, vec3f(one));
      ogl::scale(vec3f(p->size));
      ogl::drawsphere();
      ogl::scale(vec3f(0.8f));
      ogl::drawsphere();
      ogl::popmatrix();
      ogl::xtraverts += 12*6*2;

      if (p->size>p->max) {
        *pp = p->next;
        p->next = sempty;
        sempty = p;
      } else {
        p->size += time/100.0f;
        pp = &p->next;
      }
    }

    OGL(Disable, GL_BLEND);
    OGL(DepthMask, GL_TRUE);
  }

  static string closeent;

  /* show sparkly thingies for map entities in edit mode */
  void renderents(void)
  {
    closeent[0] = 0;
    if (!editmode) return;
    loopv(ents) {
      entity &e = ents[i];
      if (e.type==NOTUSED) continue;
      vec v = { float(e.x), float(e.y), float(e.z) };
      particle_splash(2, 2, 40, v);
    }
    const int e = world::closestent();
    if (e>=0) {
      entity &c = ents[e];
      sprintf_s(closeent)("closest entity = %s (%d, %d, %d, %d), selection = (%d, %d)", entnames[c.type], c.attr1, c.attr2, c.attr3, c.attr4, cmd::getvar("selxs"), cmd::getvar("selys"));
    }
  }

  void loadsky(char *basename)
  {
    static string lastsky = "";
    if (strcmp(lastsky, basename)==0) return;
    const char *side[] = { "ft", "bk", "lf", "rt", "dn", "up" };
    const int texnum = 14;
    loopi(6) {
      sprintf_sd(name)("packages/%s_%s.jpg", basename, side[i]);
      int xs, ys;
      if (!ogl::installtex(texnum+i, path(name), xs, ys, true))
        console::out("could not load sky textures");
    }
    strcpy_s(lastsky, basename);
  }

  COMMAND(loadsky, ARG_1STR);

  static float cursordepth = 0.9f;
  static vec4i viewport;
  static mat4x4f readmm, readpm;

  static void readmatrices()
  {
    OGL(GetIntegerv, GL_VIEWPORT, &viewport.x);
    readmm = ogl::matrix(ogl::MODELVIEW);
    readpm = ogl::matrix(ogl::PROJECTION);
  }

  static float depthcorrect(float d) { return (d<=1/256.0f) ? d*256 : d; }

  void readdepth(int w, int h)
  {
    glReadPixels(w/2, h/2, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &cursordepth);
    const vec3f screenp(float(w)/2.f,float(h)/2.f,depthcorrect(cursordepth));
    const vec3f v = unproject(screenp, readmm, readpm, viewport);
    worldpos = vec(v.x,v.z,v.y);
    const vec r(readmm.vx.x,readmm.vy.x,readmm.vz.x);
    const vec u(readmm.vx.y,readmm.vy.y,readmm.vz.y);
    setorient(r, u);
  }

  void drawicon(float tx, float ty, int x, int y)
  {
    const float o = 1/3.0f;
    const int s = 120;
    tx /= 192;
    ty /= 192;
    OGL(BindTexture, GL_TEXTURE_2D, 5);
    const vvec<4> verts[] = {
      vvec<4>(tx,   ty,   x,   y),
      vvec<4>(tx+o, ty,   x+s, y),
      vvec<4>(tx,   ty+o, x,   y+s),
      vvec<4>(tx+o, ty+o, x+s, y+s)
    };
    ogl::bindshader(ogl::DIFFUSETEX);
    ogl::immdraw(GL_TRIANGLE_STRIP, 2, 2, 0, 4, &verts[0][0]);
    ogl::xtraverts += 4;
  }

  static void invertperspective(void)
  {
    const mat4x4f &pm = ogl::matrix(ogl::PROJECTION); /* must be perspective */
    mat4x4f inv(zero);
    inv.vx.x = 1.f/pm.vx.x;
    inv.vy.y = 1.f/pm.vy.y;
    inv.vz.w = 1.f/pm.vw.z;
    inv.vw.z = -1.f;
    inv.vw.w = pm.vz.z/pm.vw.z;
    ogl::loadmatrix(inv);
  }

  VARP(crosshairsize, 0, 15, 50);

  static int dblend = 0;
  void damageblend(int n) { dblend += n; }

  VAR(hidestats, 0, 0, 1);
  VARP(crosshairfx, 0, 1, 1);

  void drawhud(int w, int h, int curfps, int nquads, int curvert, bool underwater)
  {
    readmatrices();
    if (editmode) {
      if (cursordepth==1.0f) worldpos = player1->o;
      edit::cursorupdate();
    }

    OGL(Disable, GL_DEPTH_TEST);
    invertperspective();
    ogl::pushmatrix();
    ogl::ortho(0, VIRTW, VIRTH, 0, -1, 1);
    OGL(Enable, GL_BLEND);

    OGL(DepthMask, GL_FALSE);
    if (dblend || underwater) {
      OGL(BlendFunc, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
      const vvec<2> verts[] = {
        vvec<2>(0.f,          0.f),
        vvec<2>(float(VIRTW), 0.f),
        vvec<2>(0.f,          float(VIRTH)),
        vvec<2>(float(VIRTW), float(VIRTH))
      };
      if (dblend)
        OGL(VertexAttrib3f,ogl::COL,0.0f,0.9f,0.9f);
      else
        OGL(VertexAttrib3f,ogl::COL,0.9f,0.5f,0.0f);
      ogl::bindshader(ogl::COLOR_ONLY);
      ogl::immdraw(GL_TRIANGLE_STRIP,2,0,0,4,&verts[0][0]);
      dblend -= curtime/3;
      if (dblend<0) dblend = 0;
    }
    OGL(Enable, GL_TEXTURE_2D);

    const char *command = console::getcurcommand();
    const char *player = weapon::playerincrosshair();
    if (command)
      draw_textf("> %s_", 20, 1570, 2, command);
    else if (closeent[0])
      draw_text(closeent, 20, 1570, 2);
    else if (player)
      draw_text(player, 20, 1570, 2);

    game::renderscores();
    if (!menu::render()) {
      OGL(BlendFunc, GL_SRC_ALPHA, GL_SRC_ALPHA);
      OGL(BindTexture, GL_TEXTURE_2D, 1);
      OGL(VertexAttrib3f,ogl::COL,1.f,1.f,1.f);
      if (crosshairfx) {
        if (player1->gunwait)
          OGL(VertexAttrib3f,ogl::COL,0.5f,0.5f,0.5f);
        else if (player1->health<=25)
          OGL(VertexAttrib3f,ogl::COL,1.0f,0.0f,0.0f);
        else if (player1->health<=50)
          OGL(VertexAttrib3f,ogl::COL,1.0f,0.5f,0.0f);
      }
      const float csz = float(crosshairsize);
      const vvec<4> verts[] = {
        vvec<4>(0.f, 0.f, float(VIRTW/2) - csz, float(VIRTH/2) - csz),
        vvec<4>(1.f, 0.f, float(VIRTW/2) + csz, float(VIRTH/2) - csz),
        vvec<4>(0.f, 1.f, float(VIRTW/2) - csz, float(VIRTH/2) + csz),
        vvec<4>(1.f, 1.f, float(VIRTW/2) + csz, float(VIRTH/2) + csz)
      };
      ogl::bindshader(ogl::DIFFUSETEX);
      ogl::immdraw(GL_TRIANGLE_STRIP, 2, 2, 0, 4, &verts[0][0]);
    }

    ogl::popmatrix();

    ogl::pushmatrix();
    ogl::ortho(0.f,VIRTW*4.f/3.f,VIRTH*4.f/3.f,0.f,-1.f,1.f);
    console::render();

    if (!hidestats) {
      ogl::popmatrix();
      ogl::pushmatrix();
      ogl::ortho(0.f,VIRTW*3.f/2.f,VIRTH*3.f/2.f,0.f,-1.f,1.f);
      draw_textf("fps %d", 3200, 2390, 2, curfps);
      draw_textf("wqd %d", 3200, 2460, 2, nquads);
      draw_textf("wvt %d", 3200, 2530, 2, curvert);
      draw_textf("evt %d", 3200, 2600, 2, ogl::xtraverts);
    }

    ogl::popmatrix();

    if (player1->state==CS_ALIVE) {
      ogl::pushmatrix();
      ogl::ortho(0, VIRTW/2, VIRTH/2, 0, -1, 1);
      draw_textf("%d",  90, 827, 2, player1->health);
      if (player1->armour) draw_textf("%d", 390, 827, 2, player1->armour);
      draw_textf("%d", 690, 827, 2, player1->ammo[player1->gunselect]);
      ogl::popmatrix();
      ogl::pushmatrix();
      ogl::ortho(0, VIRTW, VIRTH, 0, -1, 1);
      OGL(Disable, GL_BLEND);
      drawicon(128, 128, 20, 1650);
      if (player1->armour) drawicon((float)(player1->armourtype*64), 0, 620, 1650);
      int g = player1->gunselect;
      int r = 64;
      if (g>2) { g -= 3; r = 128; }
      drawicon((float)(g*64), (float)r, 1220, 1650);
      ogl::popmatrix();
    }

    OGL(DepthMask, GL_TRUE);
    OGL(Disable, GL_BLEND);
    OGL(Disable, GL_TEXTURE_2D);
    OGL(Enable, GL_DEPTH_TEST);
  }

} /* namespace rdr */

