#include "cube.h"
#include <GL/gl.h>

namespace rdr
{
  static const int MAXPARTICLES = 10500;
  static const int NUMPARTCUTOFF = 20;
  struct particle { vec o, d; int fade, type; int millis; particle *next; };
  static particle particles[MAXPARTICLES], *parlist = NULL, *parempty = NULL;
  static bool parinit = false;

  VARP(maxparticles, 100, 2000, MAXPARTICLES-500);

  static void newparticle(const vec &o, const vec &d, int fade, int type)
  {
    if (!parinit) {
      loopi(MAXPARTICLES) {
        particles[i].next = parempty;
        parempty = &particles[i];
      }
      parinit = true;
    }
    if (parempty) {
      particle *p = parempty;
      parempty = p->next;
      p->o = o;
      p->d = d;
      p->fade = fade;
      p->type = type;
      p->millis = lastmillis;
      p->next = parlist;
      parlist = p;
    }
  }

  VAR(demotracking, 0, 0, 1);
  VARP(particlesize, 20, 100, 500);

  static vec right, up;

  void setorient(vec &r, vec &u) { right = r; up = u; }

  static const struct parttype { float r, g, b; int gr, tex; float sz; } parttypes[] =
  {
    { 0.7f, 0.6f, 0.3f, 2,  3, 0.06f }, // yellow: sparks
    { 0.5f, 0.5f, 0.5f, 20, 7, 0.15f }, // grey:   small smoke
    { 0.2f, 0.2f, 1.0f, 20, 3, 0.08f }, // blue:   edit mode entities
    { 1.0f, 0.1f, 0.1f, 1,  7, 0.06f }, // red:    blood spats
    { 1.0f, 0.8f, 0.8f, 20, 6, 1.2f  }, // yellow: fireball1
    { 0.5f, 0.5f, 0.5f, 20, 7, 0.6f  }, // grey:   big smoke
    { 1.0f, 1.0f, 1.0f, 20, 8, 1.2f  }, // blue:   fireball2
    { 1.0f, 1.0f, 1.0f, 20, 9, 1.2f  }, // green:  fireball3
    { 1.0f, 0.1f, 0.1f, 0,  7, 0.2f  }, // red:    demotrack
  };


  void render_particles(int time)
  {
    if (demoplayback && demotracking) {
      vec nom = { 0, 0, 0 };
      newparticle(player1->o, nom, 100000000, 8);
    }

    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA);
    glDisable(GL_FOG);

    int numrender = 0;

    for (particle *p, **pp = &parlist; (p = *pp);) {
      const parttype *pt = &parttypes[p->type];
      const float sz = pt->sz*particlesize/100.0f;

      glBindTexture(GL_TEXTURE_2D, pt->tex);
      glColor3f(pt->r, pt->g, pt->b);

      const vvec<5> verts[] = {
        vvec<5>(0.f, 1.f, p->o.x+(-right.x+up.x)*sz, p->o.z+(-right.y+up.y)*sz, p->o.y+(-right.z+up.z)*sz),
        vvec<5>(1.f, 1.f, p->o.x+( right.x+up.x)*sz, p->o.z+( right.y+up.y)*sz, p->o.y+( right.z+up.z)*sz),
        vvec<5>(0.f, 0.f, p->o.x+(-right.x-up.x)*sz, p->o.z+(-right.y-up.y)*sz, p->o.y+(-right.z-up.z)*sz),
        vvec<5>(1.f, 0.f, p->o.x+( right.x-up.x)*sz, p->o.z+( right.y-up.y)*sz, p->o.y+( right.z-up.z)*sz)
      };
      ogl::drawarray(GL_TRIANGLE_STRIP, 3, 2, 4, &verts[0][0]);
      xtraverts += 4;

      if (numrender++>maxparticles || (p->fade -= time)<0) {
        *pp = p->next;
        p->next = parempty;
        parempty = p;
      } else {
        if (pt->gr) p->o.z -= ((lastmillis-p->millis)/3.0f)*curtime/(pt->gr*10000);
        vec a = p->d;
        vmul(a,time);
        vdiv(a,20000.0f);
        vadd(p->o, a);
        pp = &p->next;
      }
    }

    glEnable(GL_FOG);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
  }

  void particle_splash(int type, int num, int fade, vec &p)
  {
    loopi(num) {
      const int radius = type==5 ? 50 : 150;
      int x, y, z;
      do {
        x = rnd(radius*2)-radius;
        y = rnd(radius*2)-radius;
        z = rnd(radius*2)-radius;
      } while (x*x+y*y+z*z>radius*radius);
      vec d = { (float)x, (float)y, (float)z };
      newparticle(p, d, rnd(fade*3), type);
    }
  }

  void particle_trail(int type, int fade, vec &s, vec &e)
  {
    vdist(d, v, s, e);
    vdiv(v, d*2+0.1f);
    vec p = s;
    loopi(int(d)*2) {
      vadd(p, v);
      const vec d = { float(rnd(12)-5), float(rnd(11)-5), float(rnd(11)-5) };
      newparticle(p, d, rnd(fade)+fade, type);
    }
  }
} /* namespace rdr */


