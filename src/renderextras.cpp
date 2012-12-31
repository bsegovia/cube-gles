#include "cube.h"
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
    const vvec<3> verts[] = {
      vvec<3>(float(x1), z1, float(y1)),
      vvec<3>(float(x1), z1, float(y1)+0.01f),
      vvec<3>(float(x2), z2, float(y2)),
      vvec<3>(float(x2), z2, float(y2)+0.01f)
    };
    ogl::drawarray(GL_TRIANGLE_STRIP, 3, 0, 4, &verts[0][0]);
    xtraverts += 4;
  }

  void linestyle(float width, int r, int g, int b)
  {
    OGL(LineWidth, width);
    glColor3ub(r,g,b);
  }

  void box(const block &b, float z1, float z2, float z3, float z4)
  {
    const vvec<3> verts[] = {
      vvec<3>(float(b.x),      z1, float(b.y)),
      vvec<3>(float(b.x+b.xs), z2, float(b.y)),
      vvec<3>(float(b.x+b.xs), z3, float(b.y+b.ys)),
      vvec<3>(float(b.x),      z4, float(b.y+b.ys))
    };
    ogl::drawarray(GL_LINE_LOOP, 3, 0, 4, &verts[0][0]);
    xtraverts += 4;
  }

  void dot(int x, int y, float z)
  {
    const float DOF = 0.1f;
    const vvec<3> verts[] = {
      vvec<3>(x-DOF, float(z), y-DOF),
      vvec<3>(x+DOF, float(z), y-DOF),
      vvec<3>(x-DOF, float(z), y+DOF),
      vvec<3>(x+DOF, float(z), y+DOF)
    };
    ogl::drawarray(GL_TRIANGLE_STRIP, 3, 0, 4, &verts[0][0]);
    xtraverts += 4;
  }

  void blendbox(int x1, int y1, int x2, int y2, bool border)
  {
    OGL(DepthMask, GL_FALSE);
    OGL(Disable, GL_TEXTURE_2D);
    OGL(BlendFunc, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
    if (border)
      glColor3f(.5f, .3f, .4f);
    else
      glColor3f(1.f, 1.f, 1.f);
    const vvec<2> verts0[] = {
      vvec<2>(float(x1), float(y1)),
      vvec<2>(float(x2), float(y1)),
      vvec<2>(float(x1), float(y2)),
      vvec<2>(float(x2), float(y2))
    };
    ogl::drawarray(GL_TRIANGLE_STRIP, 2, 0, 4, &verts0[0][0]);

    OGL(Disable, GL_BLEND);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glColor3f(.2f, .7f, .4f);
    const vvec<2> verts1[] = {
      vvec<2>(float(x1), float(y1)),
      vvec<2>(float(x2), float(y1)),
      vvec<2>(float(x2), float(y2)),
      vvec<2>(float(x1), float(y2))
    };
    ogl::drawarray(GL_LINE_LOOP, 2, 0, 4, &verts1[0][0]);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    xtraverts += 8;
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
      glColor4f(1.0f, 1.0f, 1.0f, 1.0f-size);
      ogl::translate(vec3f(p->o.x, p->o.z, p->o.y));
      ogl::rotate(lastmillis/5.0f, vec3f(one));
      ogl::scale(vec3f(p->size));
      ogl::drawsphere();
      ogl::scale(vec3f(0.8f));
      ogl::drawsphere();
      ogl::popmatrix();
      xtraverts += 12*6*2;

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
    int e = world::closestent();
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
  static GLint viewport[4];
  static GLdouble mm[16], pm[16];

  static void readmatrices()
  {
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetDoublev(GL_MODELVIEW_MATRIX, mm);
    glGetDoublev(GL_PROJECTION_MATRIX, pm);
    /* XXX clean up that */
    const mat4x4f &modelview = ogl::matrix(ogl::MODELVIEW);
    const mat4x4f &projection = ogl::matrix(ogl::PROJECTION);
    loopi(16) mm[i] = (&modelview[0][0])[i];
    loopi(16) pm[i] = (&projection[0][0])[i];
  }

  // stupid function to cater for stupid ATI linux drivers that return incorrect depth values

  float depthcorrect(float d)
  {
    return (d<=1/256.0f) ? d*256 : d;
  }

  // find out the 3d target of the crosshair in the world easily and very
  // acurately. sadly many very old cards and drivers appear to fuck up on
  // glReadPixels() and give false coordinates, making shooting and such
  // impossible. also hits map entities which is unwanted.  could be replaced
  // by a more acurate version of monster.cpp los() if needed
  void readdepth(int w, int h)
  {
    glReadPixels(w/2, h/2, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &cursordepth);
    double worldx = 0, worldy = 0, worldz = 0;
    unproject(w/2, h/2, depthcorrect(cursordepth), mm, pm, viewport, &worldx, &worldz, &worldy);
    worldpos.x = (float)worldx;
    worldpos.y = (float)worldy;
    worldpos.z = (float)worldz;
    const vec r = { (float)mm[0], (float)mm[4], (float)mm[8] };
    const vec u = { (float)mm[1], (float)mm[5], (float)mm[9] };
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
    ogl::drawarray(GL_TRIANGLE_STRIP, 2, 2, 4, &verts[0][0]);
    xtraverts += 4;
  }

  void invertperspective(void)
  {
    /* Generates a valid inverse matrix for matrices generated by
     * gluPerspective
     */
    float inv[16];
    memset(inv, 0, sizeof(inv));
    inv[0*4+0] = 1.0/pm[0*4+0];
    inv[1*4+1] = 1.0/pm[1*4+1];
    inv[2*4+3] = 1.0/pm[3*4+2];
    inv[3*4+2] = -1.0;
    inv[3*4+3] = pm[2*4+2]/pm[3*4+2];
    ogl::loadmatrix(*(const mat4x4f*)inv);
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
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      edit::cursorupdate();
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    OGL(Disable, GL_DEPTH_TEST);
    invertperspective();
    ogl::pushmatrix();
    ogl::ortho(0, VIRTW, VIRTH, 0, -1, 1);
    OGL(Enable, GL_BLEND);

    OGL(DepthMask, GL_FALSE);

    if (dblend || underwater) {
      OGL(BlendFunc, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
      if (dblend)
        glColor3f(0.0f, 0.9f, 0.9f);
      else
        glColor3f(0.9f, 0.5f, 0.0f);
      const vvec<2> verts[] = {
        vvec<2>(0.f, 0.f),
        vvec<2>(float(VIRTW), 0.f),
        vvec<2>(0.f, float(VIRTH)),
        vvec<2>(float(VIRTW), float(VIRTH))
      };
      ogl::drawarray(GL_TRIANGLE_STRIP, 2, 0, 4, &verts[0][0]);
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
      glColor3ub(255,255,255);
      if (crosshairfx) {
        if (player1->gunwait)
          glColor3ub(128,128,128);
        else if (player1->health<=25)
          glColor3ub(255,0,0);
        else if (player1->health<=50)
          glColor3ub(255,128,0);
      }
      const float csz = float(crosshairsize);
      const vvec<4> verts[] = {
        vvec<4>(0.f, 0.f, float(VIRTW/2) - csz, float(VIRTH/2) - csz),
        vvec<4>(1.f, 0.f, float(VIRTW/2) + csz, float(VIRTH/2) - csz),
        vvec<4>(0.f, 1.f, float(VIRTW/2) - csz, float(VIRTH/2) + csz),
        vvec<4>(1.f, 1.f, float(VIRTW/2) + csz, float(VIRTH/2) + csz)
      };
      ogl::drawarray(GL_TRIANGLE_STRIP, 2, 2, 4, &verts[0][0]);
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
      draw_textf("evt %d", 3200, 2600, 2, xtraverts);
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

