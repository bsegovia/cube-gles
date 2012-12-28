#include "cube.h"
#include <SDL/SDL.h>
#include <zlib.h>

// lookup from map entities above to strings
extern const char *entnames[];

// XXX move that
int sfactor, ssize, cubicsize, mipsize;
header hdr;
sqr *map = NULL;
sqr *mmip[LARGEST_FACTOR*2];
extern bool hasoverbright;

namespace world
{
  ///////////////////////////////////////////////////////////////////////////
  // World core management routines
  ///////////////////////////////////////////////////////////////////////////

  /*! Set all cubes with "tag" to space, if tag is 0 then reset ALL tagged cubes
   *  according to type
   */
  static void settag(int tag, int type)
  {
    int maxx = 0, maxy = 0, minx = ssize, miny = ssize;
    loop(x, ssize) loop(y, ssize) {
      sqr *s = S(x, y);
      if (s->tag) {
        if (tag) {
          if (tag==s->tag)
            s->type = SPACE;
          else
            continue;
        } else
          s->type = type ? SOLID : SPACE;
        if (x>maxx) maxx = x;
        if (y>maxy) maxy = y;
        if (x<minx) minx = x;
        if (y<miny) miny = y;
      }
    }
    block b = {minx, miny, maxx-minx+1, maxy-miny+1};
    if (maxx) // remip minimal area of changed geometry
      remip(b);
  }

  void resettagareas(void) { settag(0, 0); }
  void settagareas(void)
  {
    settag(0, 1);
    loopv(ents)
      if (ents[i].type==CARROT)
        entities::setspawn(i, true);
  }

  void trigger(int tag, int type, bool savegame)
  {
    if (!tag)
      return;
    settag(tag, type);
    if (!savegame && type!=3)
      sound::play(S_RUMBLE);
    sprintf_sd(aliasname)("level_trigger_%d", tag);
    if (cmd::identexists(aliasname))
      cmd::execute(aliasname);
    if (type==2)
     monster::endsp(false);
  }

  void remip(block &b, int level)
  {
    if (level>=SMALLEST_FACTOR)
      return;
    int lighterr = cmd::getvar("lighterror")*3;
    sqr *w = mmip[level];
    sqr *v = mmip[level+1];
    int ws = ssize>>level;
    int vs = ssize>>(level+1);
    block s = b;
    if (s.x&1) { s.x--; s.xs++; };
    if (s.y&1) { s.y--; s.ys++; };
    s.xs = (s.xs+1)&~1;
    s.ys = (s.ys+1)&~1;
    for (int x = s.x; x<s.x+s.xs; x+=2) {
      for (int y = s.y; y<s.y+s.ys; y+=2) {
        sqr *o[4];
        o[0] = SWS(w,x,y,ws); // the 4 constituent cubes
        o[1] = SWS(w,x+1,y,ws);
        o[2] = SWS(w,x+1,y+1,ws);
        o[3] = SWS(w,x,y+1,ws);
        sqr *r = SWS(v,x/2,y/2,vs); // the target cube in the higher mip level
        *r = *o[0];
        uchar nums[MAXTYPE];
        loopi(MAXTYPE) nums[i] = 0;
        loopj(4) nums[o[j]->type]++;
        // cube contains both solid and space, treated specially in the rdr
        r->type = SEMISOLID;
        loopk(MAXTYPE) if (nums[k]==4) r->type = k;
        if (!SOLID(r)) {
          int floor = 127, ceil = -128, num = 0;
          loopi(4) if (!SOLID(o[i])) {
            num++;
            int fh = o[i]->floor;
            int ch = o[i]->ceil;
            if (r->type==SEMISOLID) {
              // crap hack, needed for rendering large mips next to hfs
              if (o[i]->type==FHF) fh -= o[i]->vdelta/4+2;
              // FIXME: needs to somehow take into account middle vertices on
              // higher mips
              if (o[i]->type==CHF) ch += o[i]->vdelta/4+2;
            }
            // take lowest floor and highest ceil, so we never have to see
            // missing lower/upper from the side
            if (fh<floor) floor = fh;
            if (ch>ceil) ceil = ch;
          }
          r->floor = floor;
          r->ceil = ceil;
        }
        // special case: don't ever split even if textures etc are different
        if (r->type==CORNER) goto mip;
        r->defer = 1;
        if (SOLID(r)) {
          // on an all solid cube, only thing that needs to be equal for a
          // perfect mip is the wall texture
          loopi(3)
            if (o[i]->wtex!=o[3]->wtex) goto c;
        } else {
          loopi(3) {
            if (o[i]->type!=o[3]->type
                || o[i]->floor!=o[3]->floor
                || o[i]->ceil!=o[3]->ceil
                || o[i]->ftex!=o[3]->ftex
                || o[i]->ctex!=o[3]->ctex
                || abs(o[i+1]->r-o[0]->r)>lighterr
                || abs(o[i+1]->g-o[0]->g)>lighterr
                || abs(o[i+1]->b-o[0]->b)>lighterr
                || o[i]->utex!=o[3]->utex
                || o[i]->wtex!=o[3]->wtex) goto c;
          }
          // can make a perfect mip out of a hf if slopes lie on one line
          if (r->type==CHF || r->type==FHF)
            if (o[0]->vdelta-o[1]->vdelta != o[1]->vdelta-SWS(w,x+2,y,ws)->vdelta
                || o[0]->vdelta-o[2]->vdelta != o[2]->vdelta-SWS(w,x+2,y+2,ws)->vdelta
                || o[0]->vdelta-o[3]->vdelta != o[3]->vdelta-SWS(w,x,y+2,ws)->vdelta
                || o[3]->vdelta-o[2]->vdelta != o[2]->vdelta-SWS(w,x+2,y+1,ws)->vdelta
                || o[1]->vdelta-o[2]->vdelta != o[2]->vdelta-SWS(w,x+1,y+2,ws)->vdelta) goto c;
        }
        // if any of the constituents is not perfect, then this one isn't either
        loopi(4) if (o[i]->defer) goto c;
        mip:
          r->defer = 0;
        c:;
      }
    }
    s.x /= 2;
    s.y /= 2;
    s.xs /= 2;
    s.ys /= 2;
    remip(s, level+1);
  }

  void remipmore(block &b, int level)
  {
    block bb = b;
    if (bb.x>1) bb.x--;
    if (bb.y>1) bb.y--;
    if (bb.xs<ssize-3) bb.xs++;
    if (bb.ys<ssize-3) bb.ys++;
    remip(bb, level);
  }

  int closestent(void)
  {
    if (edit::noteditmode())
      return -1;
    int best = 0;
    float bdist = 99999;
    loopv(ents) {
      entity &e = ents[i];
      if (e.type==NOTUSED) continue;
      const vec v = {float(e.x), float(e.y), float(e.z)};
      vdist(dist, t, player1->o, v);
      if (dist<bdist) {
        best = i;
        bdist = dist;
      }
    }
    return bdist==99999 ? -1 : best;
  }

  void entproperty(int prop, int amount)
  {
    const int e = closestent();
    if (e < 0) return;
    switch (prop) {
      case 0: ents[e].attr1 += amount; break;
      case 1: ents[e].attr2 += amount; break;
      case 2: ents[e].attr3 += amount; break;
      case 3: ents[e].attr4 += amount; break;
    }
  }

  static void delent(void)
  {
    const int e = closestent();
    if (e < 0) {
      console::out("no more entities");
      return;
    }
    const int t = ents[e].type;
    console::out("%s entity deleted", entnames[t]);
    ents[e].type = NOTUSED;
    client::addmsg(1, 10, SV_EDITENT, e, NOTUSED, 0, 0, 0, 0, 0, 0, 0);
    if (t==LIGHT)
      calclight();
  }

  static int findtype(const char *what)
  {
    loopi(MAXENTTYPES)
      if (strcmp(what, entnames[i])==0)
        return i;
    console::out("unknown entity type \"%s\"", what);
    return NOTUSED;
  }

  entity *newentity(int x, int y, int z, char *what, int v1, int v2, int v3, int v4)
  {
    const int type = findtype(what);
    persistent_entity e = {short(x), short(y), short(z),short(v1),
      uchar(type), uchar(v2), uchar(v3), uchar(v4)};
    switch (type) {
      case LIGHT:
        if (v1>32) v1 = 32;
        if (!v1) e.attr1 = 16;
        if (!v2 && !v3 && !v4) e.attr2 = 255;
      break;
      case MAPMODEL:
        e.attr4 = e.attr3;
        e.attr3 = e.attr2;
      case MONSTER:
      case TELEDEST:
        e.attr2 = (uchar)e.attr1;
      case PLAYERSTART:
        e.attr1 = (int)player1->yaw;
      break;
    }
    client::addmsg(1, 10, SV_EDITENT, ents.length(), type, e.x, e.y, e.z, e.attr1, e.attr2, e.attr3, e.attr4);
    ents.add(*((entity *)&e)); // unsafe!
    if (type==LIGHT)
      calclight();
    return &ents.last();
  }

  static void clearents(char *name)
  {
    int type = findtype(name);
    if (edit::noteditmode() || client::multiplayer())
      return;
    loopv(ents) {
      entity &e = ents[i];
      if (e.type==type) e.type = NOTUSED;
    }
    if (type==LIGHT) calclight();
  }

  void scalecomp(uchar &c, int intens)
  {
    int n = c*intens/100;
    if (n>255) n = 255;
    c = n;
  }

  void scalelights(int f, int intens)
  {
    loopv(ents) {
      entity &e = ents[i];
      if (e.type!=LIGHT) continue;
      e.attr1 = e.attr1*f/100;
      if (e.attr1<2) e.attr1 = 2;
      if (e.attr1>32) e.attr1 = 32;
      if (intens) {
        scalecomp(e.attr2, intens);
        scalecomp(e.attr3, intens);
        scalecomp(e.attr4, intens);
      }
    }
    calclight();
  }

  int findentity(int type, int index)
  {
    for (int i = index; i<ents.length(); i++)
      if (ents[i].type==type)
        return i;
    loopj(index)
      if (ents[j].type==type)
        return j;
    return -1;
  }

  void setup(int factor)
  {
    ssize = 1<<(sfactor = factor);
    cubicsize = ssize*ssize;
    mipsize = cubicsize*134/100;
    sqr *w = map = (sqr *)alloc(mipsize*sizeof(sqr));
    loopi(LARGEST_FACTOR*2) {
      mmip[i] = w;
      w += cubicsize>>(i*2);
    }
  }

  void empty(int factor, bool force)
  {
    if (!force && edit::noteditmode()) return;
    cleardlights();
    edit::pruneundos();
    sqr *oldmap = map;
    bool copy = false;
    if (oldmap && factor<0) {
      factor = sfactor+1;
      copy = true;
    }
    if (factor<SMALLEST_FACTOR)
      factor = SMALLEST_FACTOR;
    if (factor>LARGEST_FACTOR)
      factor = LARGEST_FACTOR;
    setup(factor);

    loop(x,ssize)
    loop(y,ssize) {
      sqr *s = S(x,y);
      s->r = s->g = s->b = 150;
      s->ftex = DEFAULT_FLOOR;
      s->ctex = DEFAULT_CEIL;
      s->wtex = s->utex = DEFAULT_WALL;
      s->type = SOLID;
      //s->type = SPACE;
      s->floor = 0;
      s->ceil = 127;
      s->vdelta = 0;
      s->defer = 0;
    }

    strncpy(hdr.head, "CUBE", 4);
    hdr.version = MAPVERSION;
    hdr.headersize = sizeof(header);
    hdr.sfactor = sfactor;

    if (copy) {
      loop(x,ssize/2) loop(y,ssize/2)
        *S(x+ssize/4, y+ssize/4) = *SWS(oldmap, x, y, ssize/2);
      loopv(ents) {
        ents[i].x += ssize/4;
        ents[i].y += ssize/4;
      }
    } else {
      strn0cpy(hdr.maptitle, "Untitled Map by Unknown", 128);
      hdr.waterlevel = -100000;
      loopi(15) hdr.reserved[i] = 0;
      loopk(3) loopi(256) hdr.texlists[k][i] = i;
      ents.setsize(0);
      block b = {8, 8, ssize-16, ssize-16};
      edit::edittypexy(SPACE, b);
    }

    calclight();
    game::startmap("base/unnamed");
    if (oldmap) {
      free(oldmap);
      edit::toggleedit();
      string exec_str = "fullbright 1";
      cmd::execute(exec_str);
    }
  }

  void mapenlarge(void) { empty(-1, false); }
  void newmap(int i)    { empty(i, false); }

#define NUMRAYS 512

  static float rdist[NUMRAYS];
  static bool ocull = true;
  static float odist = 256;
  static void toggleocull() { ocull = !ocull; };

  /* world occlusions */

  /* constructs occlusion map: cast rays in all directions on the 2d plane and
   * record distance. done exactly once per frame
   */
  void computeraytable(float vx, float vy)
  {
    if (!ocull) return;

    odist = cmd::getvar("fog")*1.5f;

    float apitch = (float)fabs(player1->pitch);
    float af = cmd::getvar("fov")/2+apitch/1.5f+3;
    float byaw = (player1->yaw-90+af)/360*PI2;
    float syaw = (player1->yaw-90-af)/360*PI2;

    loopi(NUMRAYS) {
      float angle = i*PI2/NUMRAYS;
      if ((apitch>45 /* must be bigger if fov>120 */
       || (angle<byaw && angle>syaw)
       || (angle<byaw-PI2 && angle>syaw-PI2)
       || (angle<byaw+PI2 && angle>syaw+PI2))
          && !OUTBORD(vx, vy)
          /* try to avoid tracing ray if outside of frustrum */
          && !SOLID(S(fast_f2nat(vx), fast_f2nat(vy))))
      {
        float ray = float(i)*8.f/float(NUMRAYS);
        float dx, dy;
        if (ray>1 && ray<3) {
          dx = -(ray-2);
          dy = 1;
        } else if (ray>=3 && ray<5) {
          dx = -1;
          dy = -(ray-4);
        } else if (ray>=5 && ray<7) {
          dx = ray-6;
          dy = -1;
        } else {
          dx = 1;
          dy = ray>4 ? ray-8 : ray;
        }
        float sx = vx;
        float sy = vy;
        for (;;) {
          sx += dx;
          sy += dy;
#if 0
          if (sx<0.f || sy<0.f || sx>=float(ssize) || sy>=float(ssize)) {
            rdist[i] = 1e6f;
            break;
          }
#endif
          if (SOLID(S(fast_f2nat(sx), fast_f2nat(sy)))) {
            rdist[i] = (float)(fabs(sx-vx)+fabs(sy-vy));
            break;
          }
        }
      } else
        rdist[i] = 2.f;
    }
  }

  // test occlusion for a cube... one of the most computationally expensive
  // functions in the engine as its done for every cube and entity, but its
  // effect is more than worth it!
  INLINE float ca(float x, float y) { return x>y ? y/x : 2-x/y; };
  INLINE float ma(float x, float y) { return x==0 ? (y>0 ? 2 : -2) : y/x; };

  int isoccluded(float vx, float vy, float cx, float cy, float csize)
  {
    if (!ocull)
      return 0;
    // n = point on the border of the cube that is closest to v
    float nx = vx, ny = vy;
    if (nx<cx)
      nx = cx;
    else if (nx>cx+csize)
      nx = cx+csize;
    if (ny<cy)
      ny = cy;
    else if (ny>cy+csize)
      ny = cy+csize;
    const float xdist = (float)fabs(nx-vx);
    const float ydist = (float)fabs(ny-vy);
    if (xdist>odist || ydist>odist)
      return 2;
    const float dist = xdist+ydist-1; // 1 needed?

    // ABC
    // D E
    // FGH

    // - check middle cube? BG
    // find highest and lowest angle in the occlusion map that this cube
    // spans, based on its most left and right points on the border from the
    // viewer pov... I see no easier way to do this than this silly code below
    float h, l;
    if (cx<=vx) { // ABDFG
      if (cx+csize<vx) { // ADF
        if (cy<=vy) { // AD
          if (cy+csize<vy) {
            h = ca(-(cx-vx), -(cy+csize-vy))+4;
            l = ca(-(cx+csize-vx), -(cy-vy))+4;\
          } else { // A
            h = ma(-(cx+csize-vx), -(cy+csize-vy))+4;
            l = ma(-(cx+csize-vx), -(cy-vy))+4;
          }  // D
        } else {
          h = ca(cy+csize-vy, -(cx+csize-vx))+2;
          l = ca(cy-vy, -(cx-vx))+2;
        } // F
      } else { // BG
        if (cy<=vy) {
          if (cy+csize<vy) {
            h = ma(-(cy+csize-vy), cx-vx)+6;
            l = ma(-(cy+csize-vy), cx+csize-vx)+6;
          } // B
          else
            return 0;
        } else {
          h = ma(cy-vy, -(cx+csize-vx))+2;
          l = ma(cy-vy, -(cx-vx))+2;
        } // G
      }
    } else { // CEH
      if (cy<=vy) { // CE
        if (cy+csize<vy) {
          h = ca(-(cy-vy), cx-vx)+6;
          l = ca(-(cy+csize-vy), cx+csize-vx)+6;
        } else { // C
          h = ma(cx-vx, cy-vy);
          l = ma(cx-vx, cy+csize-vy);
        } // E
      } else {
        h = ca(cx+csize-vx, cy-vy);
        l = ca(cx-vx, cy+csize-vy);
      } // H
    }

    // get indexes into occlusion map from angles
    const int si = fast_f2nat(h*(NUMRAYS/8))+NUMRAYS;
    int ei = fast_f2nat(l*(NUMRAYS/8))+NUMRAYS+1;
    if (ei<=si) ei += NUMRAYS;

    for (int i = si; i<=ei; i++)
      // if any value in this segment of the occlusion map is further away
      // then cube is not occluded
      if (dist<rdist[i&(NUMRAYS-1)])
        return 0;

    return 1; // cube is entirely occluded
  }

  ///////////////////////////////////////////////////////////////////////////
  // World lighting
  ///////////////////////////////////////////////////////////////////////////

  VAR(lightscale,1,4,100);

  // done in realtime, needs to be fast
  void lightray(float bx, float by, persistent_entity &light)
  {
    float lx = light.x;//+(rnd(21)-10)*0.1f;
    float ly = light.y;//+(rnd(21)-10)*0.1f;
    float dx = bx-lx;
    float dy = by-ly;
    float dist = (float)sqrt(dx*dx+dy*dy);
    if (dist<1.0f) return;
    int reach = light.attr1;
    int steps = (int)(reach*reach*1.6f/dist); // can change this for speedup/quality?
    const int PRECBITS = 12;
    const float PRECF = 4096.0f;
    int x = (int)(lx*PRECF);
    int y = (int)(ly*PRECF);
    int l = light.attr2<<PRECBITS;
    int stepx = (int)(dx/(float)steps*PRECF);
    int stepy = (int)(dy/(float)steps*PRECF);
    // incorrect: light will fade quicker if near edge of the world
    int stepl = fast_f2nat(l/(float)steps);

    if (hasoverbright) {
      l /= lightscale;
      stepl /= lightscale;
      // coloured light version, special case because most lights are white
      if (light.attr3 || light.attr4) {
        int dimness = rnd((255-(light.attr2+light.attr3+light.attr4)/3)/16+1);
        x += stepx*dimness;
        y += stepy*dimness;

        if (OUTBORD(x>>PRECBITS, y>>PRECBITS))
          return;

        int g = light.attr3<<PRECBITS;
        int stepg = fast_f2nat(g/(float)steps);
        int b = light.attr4<<PRECBITS;
        int stepb = fast_f2nat(b/(float)steps);
        g /= lightscale;
        stepg /= lightscale;
        b /= lightscale;
        stepb /= lightscale;
        loopi(steps)
        {
          sqr *s = S(x>>PRECBITS, y>>PRECBITS);
          int tl = (l>>PRECBITS)+s->r;
          s->r = tl>255 ? 255 : tl;
          tl = (g>>PRECBITS)+s->g;
          s->g = tl>255 ? 255 : tl;
          tl = (b>>PRECBITS)+s->b;
          s->b = tl>255 ? 255 : tl;
          if (SOLID(s)) return;
          x += stepx;
          y += stepy;
          l -= stepl;
          g -= stepg;
          b -= stepb;
          stepl -= 25;
          stepg -= 25;
          stepb -= 25;
        }
      } else { // white light, special optimized version
        int dimness = rnd((255-light.attr2)/16+1);
        x += stepx*dimness;
        y += stepy*dimness;

        if (OUTBORD(x>>PRECBITS, y>>PRECBITS)) return;

        loopi(steps)
        {
          sqr *s = S(x>>PRECBITS, y>>PRECBITS);
          int tl = (l>>PRECBITS)+s->r;
          s->r = s->g = s->b = tl>255 ? 255 : tl;
          if (SOLID(s)) return;
          x += stepx;
          y += stepy;
          l -= stepl;
          stepl -= 25;
        }
      }
    }
    // the old (white) light code, here for the few people with old video
    // cards that don't support overbright
    else
      loopi(steps) {
        sqr *s = S(x>>PRECBITS, y>>PRECBITS);
        int light = l>>PRECBITS;
        if (light>s->r) s->r = s->g = s->b = (uchar)light;
        if (SOLID(s)) return;
        x += stepx;
        y += stepy;
        l -= stepl;
      }
  }

  void calclightsource(persistent_entity &l)
  {
    int reach = l.attr1;
    const int sx = l.x-reach;
    const int ex = l.x+reach;
    const int sy = l.y-reach;
    const int ey = l.y+reach;
    rndreset();

    const float s = 0.8f;
    for (float sx2 = (float)sx; sx2<=ex; sx2+=s*2) {
      lightray(sx2, (float)sy, l);
      lightray(sx2, (float)ey, l);
    }
    for (float sy2 = sy+s; sy2<=ey-s; sy2+=s*2) {
      lightray((float)sx, sy2, l);
      lightray((float)ex, sy2, l);
    }

    rndtime();
  }

  /* median filter, smooths out random noise in light
   * and makes it more mipable
   */
  void postlightarea(block &a)
  {
    loop(x,a.xs)
    loop(y,a.ys) { // assumes area not on edge of world
      sqr *s = S(x+a.x,y+a.y);
#define MEDIAN(m) s->m = (s->m*2 + SW(s,1,0)->m*2  + SW(s,0,1)->m*2 \
    + SW(s,-1,0)->m*2 + SW(s,0,-1)->m*2 \
    + SW(s,1,1)->m    + SW(s,1,-1)->m \
    + SW(s,-1,1)->m   + SW(s,-1,-1)->m)/14;  // median is 4/2/1 instead
      MEDIAN(r);
      MEDIAN(g);
      MEDIAN(b);
    }
#undef MEDIAN

    world::remip(a);
  }

  void calclight(void)
  {
    loop(x,ssize) loop(y,ssize) {
      sqr *s = S(x,y);
      s->r = s->g = s->b = 10;
    }

    loopv(ents) {
      entity &e = ents[i];
      if (e.type==LIGHT)
        calclightsource(e);
    }

    block b = {1, 1, ssize-2, ssize-2};
    postlightarea(b);
    cmd::setvar("fullbright", 0);
  }

  VARP(dynlight, 0, 16, 32);

  static vector<block *> dlights;

  void cleardlights(void)
  {
    while (!dlights.empty()) {
      block *backup = dlights.pop();
      world::blockpaste(*backup);
      free(backup);
    }
  }

  void dodynlight(vec &vold, vec &v, int reach, int strength, dynent *owner)
  {
    if (!reach)
      reach = dynlight;
    if (owner->monsterstate)
      reach = reach/2;
    if (!reach)
      return;
    if (v.x<0 || v.y<0 || v.x>ssize || v.y>ssize)
      return;

    int creach = reach+16;  // dependant on lightray random offsets!
    block b = { (int)v.x-creach, (int)v.y-creach, creach*2+1, creach*2+1 };

    if (b.x<1) b.x = 1;
    if (b.y<1) b.y = 1;
    if (b.xs+b.x>ssize-2) b.xs = ssize-2-b.x;
    if (b.ys+b.y>ssize-2) b.ys = ssize-2-b.y;

    // backup area before rendering in dynlight
    dlights.add(world::blockcopy(b));
    persistent_entity l = {
      short(v.x), short(v.y), short(v.z),
      short(reach), uchar(LIGHT), uchar(strength), 0, 0
    };
    calclightsource(l);
    postlightarea(b);
  }

  // utility functions also used by editing code
  block *blockcopy(block &s)
  {
    block *b = (block *)alloc(sizeof(block)+s.xs*s.ys*sizeof(sqr));
    *b = s;
    sqr *q = (sqr *)(b+1);
    for (int x = s.x; x<s.xs+s.x; x++) for (int y = s.y; y<s.ys+s.y; y++) *q++ = *S(x,y);
    return b;
  }

  void blockpaste(block &b)
  {
    sqr *q = (sqr *)((&b)+1);
    for (int x = b.x; x<b.xs+b.x; x++) for (int y = b.y; y<b.ys+b.y; y++) *S(x,y) = *q++;
    world::remipmore(b);
  }

  ///////////////////////////////////////////////////////////////////////////
  // World rendering
  ///////////////////////////////////////////////////////////////////////////

  void render_wall(sqr *o, sqr *s, int x1, int y1, int x2, int y2, int mip, sqr *d1, sqr *d2, bool topleft)
  {
      if (SOLID(o) || o->type==SEMISOLID)
      {
          float c1 = s->floor;
          float c2 = s->floor;
          if (s->type==FHF) { c1 -= d1->vdelta/4.0f; c2 -= d2->vdelta/4.0f; };
          float f1 = s->ceil;
          float f2 = s->ceil;
          if (s->type==CHF) { f1 += d1->vdelta/4.0f; f2 += d2->vdelta/4.0f; };
          //if (f1-c1<=0 && f2-c2<=0) return;
          rdr::render_square(o->wtex, c1, c2, f1, f2, x1<<mip, y1<<mip, x2<<mip, y2<<mip, 1<<mip, d1, d2, topleft);
          return;
      };
      {
        float f1 = s->floor;
        float f2 = s->floor;
        float c1 = o->floor;
        float c2 = o->floor;
        if (o->type==FHF && s->type!=FHF) {
          c1 -= d1->vdelta/4.0f;
          c2 -= d2->vdelta/4.0f;
        }
        if (s->type==FHF && o->type!=FHF) {
          f1 -= d1->vdelta/4.0f;
          f2 -= d2->vdelta/4.0f;
        }
        if (f1>=c1 && f2>=c2) goto skip;
        rdr::render_square(o->wtex, f1, f2, c1, c2, x1<<mip, y1<<mip, x2<<mip, y2<<mip, 1<<mip, d1, d2, topleft);
      };
      skip:
      {
          float f1 = o->ceil;
          float f2 = o->ceil;
          float c1 = s->ceil;
          float c2 = s->ceil;
          if (o->type==CHF && s->type!=CHF)
          {
              f1 += d1->vdelta/4.0f;
              f2 += d2->vdelta/4.0f;
          }
          else if (s->type==CHF && o->type!=CHF)
          {
              c1 += d1->vdelta/4.0f;
              c2 += d2->vdelta/4.0f;
          }
          if (c1<=f1 && c2<=f2) return;
          rdr::render_square(o->utex, f1, f2, c1, c2, x1<<mip, y1<<mip, x2<<mip, y2<<mip, 1<<mip, d1, d2, topleft);
      };
  };

  const int MAX_MIP = 5;   // 32x32 unit blocks
  const int MIN_LOD = 2;
  const int LOW_LOD = 25;
  const int MAX_LOD = 1000;

  int lod = 40, lodtop, lodbot, lodleft, lodright;
  int min_lod;

  int stats[LARGEST_FACTOR];

  // detect those cases where a higher mip solid has a visible wall next to lower mip cubes
  // (used for wall rendering below)

  bool issemi(int mip, int x, int y, int x1, int y1, int x2, int y2)
  {
      if (!(mip--)) return true;
      sqr *w = mmip[mip];
      int msize = ssize>>mip;
      x *= 2;
      y *= 2;
      switch (SWS(w, x+x1, y+y1, msize)->type)
      {
          case SEMISOLID: if (issemi(mip, x+x1, y+y1, x1, y1, x2, y2)) return true;
          case CORNER:
          case SOLID: break;
          default: return true;
      };
      switch (SWS(w, x+x2, y+y2, msize)->type)
      {
          case SEMISOLID: if (issemi(mip, x+x2, y+y2, x1, y1, x2, y2)) return true;
          case CORNER:
          case SOLID: break;
          default: return true;
      };
      return false;
  };

  static bool render_floor, render_ceil;

  // the core recursive function, renders a rect of cubes at a certain mip level
  // from a viewer perspective call itself for lower mip levels, on most modern
  // machines however this function will use the higher mip levels only for
  // perfect mips.
  void render_seg_new(float vx, float vy, float vh, int mip, int x, int y, int xs, int ys)
  {
    sqr *w = mmip[mip];
    int sz = ssize>>mip;
    int vxx = ((int)vx+(1<<mip)/2)>>mip;
    int vyy = ((int)vy+(1<<mip)/2)>>mip;
    int lx = vxx-lodleft;   // these mark the rect inside the current rest that we want to render using a lower mip level
    int ly = vyy-lodtop;
    int rx = vxx+lodright;
    int ry = vyy+lodbot;

    float fsize = (float)(1<<mip);
    for (int ox = x; ox<xs; ox++) for (int oy = y; oy<ys; oy++)       // first collect occlusion information for this block
    {
      SWS(w,ox,oy,sz)->occluded = world::isoccluded(player1->o.x, player1->o.y, (float)(ox<<mip), (float)(oy<<mip), fsize);
    };

    int pvx = (int)vx>>mip;
    int pvy = (int)vy>>mip;
    if (pvx>=0 && pvy>=0 && pvx<sz && pvy<sz)
    {
      //SWS(w,vxx,vyy,sz)->occluded = 0;
      SWS(w, pvx, pvy, sz)->occluded = 0;  // player cell never occluded
    };

#define df(x) s->floor-(x->vdelta/4.0f)
#define dc(x) s->ceil+(x->vdelta/4.0f)

    // loop through the rect 3 times (for floor/ceil/walls seperately, to facilitate dynamic stripify)
    // for each we skip occluded cubes (occlusion at higher mip levels is a big time saver!).
    // during the first loop (ceil) we collect cubes that lie within the lower mip rect and are
    // also deferred, and render them recursively. Anything left (perfect mips and higher lods) we
    // render here.

#define LOOPH {for (int xx = x; xx<xs; xx++) for (int yy = y; yy<ys; yy++) { \
  sqr *s = SWS(w,xx,yy,sz); if (s->occluded==1) continue; \
  if (s->defer && !s->occluded && mip && xx>=lx && xx<rx && yy>=ly && yy<ry)

#define LOOPD sqr *t = SWS(s,1,0,sz); \
  sqr *u = SWS(s,1,1,sz); \
  sqr *v = SWS(s,0,1,sz);

      LOOPH   // ceils
      {
          int start = yy;
          sqr *next;
          while (yy<ys-1 && (next = SWS(w,xx,yy+1,sz))->defer && !next->occluded) yy++;    // collect 2xN rect of lower mip
          render_seg_new(vx, vy, vh, mip-1, xx*2, start*2, xx*2+2, yy*2+2);
          continue;
        };
          stats[mip]++;
          LOOPD
          if ((s->type==SPACE || s->type==FHF) && s->ceil>=vh && render_ceil)
              rdr::render_flat(s->ctex, xx<<mip, yy<<mip, 1<<mip, s->ceil, s, t, u, v, true);
          if (s->type==CHF) //if (s->ceil>=vh)
              rdr::render_flatdelta(s->ctex, xx<<mip, yy<<mip, 1<<mip, dc(s), dc(t), dc(u), dc(v), s, t, u, v, true);
      }};

      LOOPH continue;     // floors
          LOOPD
          if ((s->type==SPACE || s->type==CHF) && s->floor<=vh && render_floor)
          {
              rdr::render_flat(s->ftex, xx<<mip, yy<<mip, 1<<mip, s->floor, s, t, u, v, false);
        if (s->floor<hdr.waterlevel && !SOLID(s)) rdr::addwaterquad(xx<<mip, yy<<mip, 1<<mip);
          };
          if (s->type==FHF)
          {
              rdr::render_flatdelta(s->ftex, xx<<mip, yy<<mip, 1<<mip, df(s), df(t), df(u), df(v), s, t, u, v, false);
        if (s->floor-s->vdelta/4.0f<hdr.waterlevel && !SOLID(s)) rdr::addwaterquad(xx<<mip, yy<<mip, 1<<mip);
          };
      }};

      LOOPH continue;     // walls
          LOOPD
          //  w
          // zSt
          //  vu

          sqr *w = SWS(s,0,-1,sz);
          sqr *z = SWS(s,-1,0,sz);
          bool normalwall = true;

          if (s->type==CORNER)
          {
              // cull also
              bool topleft = true;
              sqr *h1 = NULL;
              sqr *h2 = NULL;
              if (SOLID(z))
              {
                  if (SOLID(w))      { render_wall(w, h2 = s, xx+1, yy, xx, yy+1, mip, t, v, false); topleft = false; }
                  else if (SOLID(v)) { render_wall(v, h2 = s, xx, yy, xx+1, yy+1, mip, s, u, false); };
              }
              else if (SOLID(t))
              {
                  if (SOLID(w))      { render_wall(w, h1 = s, xx+1, yy+1, xx, yy, mip, u, s, false); }
                  else if (SOLID(v)) { render_wall(v, h1 = s, xx, yy+1, xx+1, yy, mip, v, t, false); topleft = false; };
              }
              else
              {
                  normalwall = false;
                  bool wv = w->ceil-w->floor < v->ceil-v->floor;
                  if (z->ceil-z->floor < t->ceil-t->floor)
                  {
                      if (wv) { render_wall(h1 = s, h2 = v, xx+1, yy, xx, yy+1, mip, t, v, false); topleft = false; }
                      else   { render_wall(h1 = s, h2 = w, xx, yy, xx+1, yy+1, mip, s, u, false); };
                  }
                  else
                  {
                      if (wv) { render_wall(h2 = s, h1 = v, xx+1, yy+1, xx, yy, mip, u, s, false); }
                      else   { render_wall(h2 = s, h1 = w, xx, yy+1, xx+1, yy, mip, v, t, false); topleft = false; };
                  };
              };
              rdr::render_tris(xx<<mip, yy<<mip, 1<<mip, topleft, h1, h2, s, t, u, v);
          }

          if (normalwall)
          {
              bool inner = xx!=sz-1 && yy!=sz-1;

              if (xx>=vxx && xx!=0 && yy!=sz-1 && !SOLID(z) && (!SOLID(s) || z->type!=CORNER)
                  && (z->type!=SEMISOLID || issemi(mip, xx-1, yy, 1, 0, 1, 1)))
                  render_wall(s, z, xx,   yy,   xx,   yy+1, mip, s, v, true);
              if (xx<=vxx && inner && !SOLID(t) && (!SOLID(s) || t->type!=CORNER)
                  && (t->type!=SEMISOLID || issemi(mip, xx+1, yy, 0, 0, 0, 1)))
                  render_wall(s, t, xx+1, yy,   xx+1, yy+1, mip, t, u, false);
              if (yy>=vyy && yy!=0 && xx!=sz-1 && !SOLID(w) && (!SOLID(s) || w->type!=CORNER)
                  && (w->type!=SEMISOLID || issemi(mip, xx, yy-1, 0, 1, 1, 1)))
                  render_wall(s, w, xx,   yy,   xx+1, yy,   mip, s, t, false);
              if (yy<=vyy && inner && !SOLID(v) && (!SOLID(s) || v->type!=CORNER)
                  && (v->type!=SEMISOLID || issemi(mip, xx, yy+1, 0, 0, 1, 0)))
                  render_wall(s, v, xx,   yy+1, xx+1, yy+1, mip, v, u, true);
          };
      }};

  };

  void distlod(int &low, int &high, int angle, float widef)
  {
      float f = 90.0f/lod/widef;
      low = (int)((90-angle)/f);
      high = (int)(angle/f);
      if (low<min_lod) low = min_lod;
      if (high<min_lod) high = min_lod;
  };

  // does some out of date view frustrum optimisation that doesn't contribute
  // much anymore
  void render(float vx, float vy, float vh, int yaw, int pitch, float fov, int w, int h)
  {
      loopi(LARGEST_FACTOR) stats[i] = 0;
      min_lod = MIN_LOD+abs(pitch)/12;
      yaw = 360-yaw;
      float widef = fov/75.0f;
      int cdist = abs(yaw%90-45);
      if (cdist<7)    // hack to avoid popup at high fovs at 45 yaw
      {
          min_lod = max(min_lod, (int)(MIN_LOD+(10-cdist)/1.0f*widef)); // less if lod worked better
          widef = 1.0f;
      };
      lod = MAX_LOD;
      lodtop = lodbot = lodleft = lodright = min_lod;
      if (yaw>45 && yaw<=135)
      {
          lodleft = lod;
          distlod(lodtop, lodbot, yaw-45, widef);
      }
      else if (yaw>135 && yaw<=225)
      {
          lodbot = lod;
          distlod(lodleft, lodright, yaw-135, widef);
      }
      else if (yaw>225 && yaw<=315)
      {
          lodright = lod;
          distlod(lodbot, lodtop, yaw-225, widef);
      }
      else
      {
          lodtop = lod;
          distlod(lodright, lodleft, yaw<=45 ? yaw+45 : yaw-315, widef);
      }
      float hyfov = fov*h/w/2;
      render_floor = pitch<hyfov;
      render_ceil  = -pitch<hyfov;

      render_seg_new(vx, vy, vh, MAX_MIP, 0, 0, ssize>>MAX_MIP, ssize>>MAX_MIP);
      rdr::mipstats(stats[0], stats[1], stats[2]);
  }

  ///////////////////////////////////////////////////////////////////////////
  // World save / load
  ///////////////////////////////////////////////////////////////////////////

  void backup(char *name, char *backupname)
  {
    remove(backupname);
    rename(name, backupname);
  }

  static string cgzname, bakname, pcfname, mcfname;

  void setnames(const char *name)
  {
    string pakname, mapname;
    const char *slash = strpbrk(name, "/\\");
    if (slash) {
      strn0cpy(pakname, name, slash-name+1);
      strcpy_s(mapname, slash+1);
    } else {
      strcpy_s(pakname, "base");
      strcpy_s(mapname, name);
    }
    sprintf_s(cgzname)("packages/%s/%s.cgz",      pakname, mapname);
    sprintf_s(bakname)("packages/%s/%s_%d.BAK",   pakname, mapname, lastmillis);
    sprintf_s(pcfname)("packages/%s/package.cfg", pakname);
    sprintf_s(mcfname)("packages/%s/%s.cfg",      pakname, mapname);
    path(cgzname);
    path(bakname);
  }

  // the optimize routines below are here to reduce the detrimental effects of
  // messy mapping by setting certain properties (vdeltas and textures) to
  // neighbouring values wherever there is no visible difference. This allows
  // the mipmapper to generate more efficient mips.  the reason it is done on
  // save is to reduce the amount spend in the mipmapper (as that is done in
  // realtime).
  INLINE bool nhf(sqr *s) { return s->type!=FHF && s->type!=CHF; }

  // reset vdeltas on non-hf cubes
  void voptimize(void)
  {
    loop(x, ssize)
    loop(y, ssize) {
      sqr *s = S(x, y);
      if (x && y) {
        if (nhf(s) && nhf(S(x-1, y)) && nhf(S(x-1, y-1)) && nhf(S(x, y-1)))
          s->vdelta = 0;
      }
      else
        s->vdelta = 0;
    }
  }

  void topt(sqr *s, bool &wf, bool &uf, int &wt, int &ut)
  {
    sqr *o[4];
    o[0] = SWS(s,0,-1,ssize);
    o[1] = SWS(s,0,1,ssize);
    o[2] = SWS(s,1,0,ssize);
    o[3] = SWS(s,-1,0,ssize);
    wf = true;
    uf = true;
    if (SOLID(s)) {
      loopi(4) if (!SOLID(o[i])) {
        wf = false;
        wt = s->wtex;
        ut = s->utex;
        return;
      }
    } else {
      loopi(4) if (!SOLID(o[i])) {
        if (o[i]->floor<s->floor) { wt = s->wtex; wf = false; }
        if (o[i]->ceil>s->ceil)   { ut = s->utex; uf = false; }
      }
    }
  }

  // FIXME: only does 2x2, make atleast for 4x4 also
  void toptimize(void)
  {
    bool wf[4], uf[4];
    sqr *s[4];
    for (int x = 2; x<ssize-4; x += 2) {
      for (int y = 2; y<ssize-4; y += 2) {
        s[0] = S(x,y);
        int wt = s[0]->wtex, ut = s[0]->utex;
        topt(s[0], wf[0], uf[0], wt, ut);
        topt(s[1] = SWS(s[0],0,1,ssize), wf[1], uf[1], wt, ut);
        topt(s[2] = SWS(s[0],1,1,ssize), wf[2], uf[2], wt, ut);
        topt(s[3] = SWS(s[0],1,0,ssize), wf[3], uf[3], wt, ut);
        loopi(4) {
          if (wf[i]) s[i]->wtex = wt;
          if (uf[i]) s[i]->utex = ut;
        }
      }
    }
  }

  // these two are used by getmap/sendmap.. transfers compressed maps directly
  void writemap(char *mname, int msize, uchar *mdata)
  {
    setnames(mname);
    backup(cgzname, bakname);
    FILE *f = fopen(cgzname, "wb");
    if (!f) {
      console::out("could not write map to %s", cgzname);
      return;
    }
    fwrite(mdata, 1, msize, f);
    fclose(f);
    console::out("wrote map %s as file %s", mname, cgzname);
  }

  uchar *readmap(const char *mname, int *msize)
  {
    setnames(mname);
    uchar *mdata = (uchar *)loadfile(cgzname, msize);
    if (!mdata) {
      console::out("could not read map %s", cgzname);
      return NULL;
    }
    return mdata;
  }

  // save map as .cgz file. uses 2 layers of compression: first does simple
  // run-length encoding and leaves out data for certain kinds of cubes, then
  // zlib removes the last bits of redundancy. Both passes contribute greatly to
  // the miniscule map sizes.
  void save(const char *mname)
  {
    // wouldn't be able to reproduce tagged areas otherwise
    world::resettagareas();
    voptimize();
    toptimize();
    if (!*mname)
      mname = game::getclientmap();
    setnames(mname);
    backup(cgzname, bakname);
    gzFile f = gzopen(cgzname, "wb9");
    if (!f) {
      console::out("could not write map to %s", cgzname);
      return;
    }
    hdr.version = MAPVERSION;
    hdr.numents = 0;
    loopv(ents) if (ents[i].type!=NOTUSED) hdr.numents++;
    header tmp = hdr;
    endianswap(&tmp.version, sizeof(int), 4);
    endianswap(&tmp.waterlevel, sizeof(int), 16);
    gzwrite(f, &tmp, sizeof(header));
    loopv(ents) {
      if (ents[i].type!=NOTUSED) {
        entity tmp = ents[i];
        endianswap(&tmp, sizeof(short), 4);
        gzwrite(f, &tmp, sizeof(persistent_entity));
      }
    }
    sqr *t = NULL;
    int sc = 0;
#define SPURGE \
  while (sc) { \
    gzputc(f, 255); \
    if (sc>255) { \
      gzputc(f, 255); \
      sc -= 255; \
    } else { \
      gzputc(f, sc); \
      sc = 0; \
    } \
  }
    loopk(cubicsize) {
      sqr *s = &map[k];
#define c(f) (s->f==t->f)
      // 4 types of blocks, to compress a bit:
      // 255 (2): same as previous block + count
      // 254 (3): same as previous, except light // deprecated
      // SOLID (5)
      // anything else (9)

      if (SOLID(s))
      {
        if (t && c(type) && c(wtex) && c(vdelta))
          sc++;
        else {
          SPURGE;
          gzputc(f, s->type);
          gzputc(f, s->wtex);
          gzputc(f, s->vdelta);
        }
      }
      else {
        if (t && c(type) && c(floor) && c(ceil) && c(ctex) && c(ftex) && c(utex) && c(wtex) && c(vdelta) && c(tag))
          sc++;
        else {
          SPURGE;
          gzputc(f, s->type);
          gzputc(f, s->floor);
          gzputc(f, s->ceil);
          gzputc(f, s->wtex);
          gzputc(f, s->ftex);
          gzputc(f, s->ctex);
          gzputc(f, s->vdelta);
          gzputc(f, s->utex);
          gzputc(f, s->tag);
        }
      }
      t = s;
    }
    SPURGE;
#undef SPURGE
    gzclose(f);
    console::out("wrote map file %s", cgzname);
    world::settagareas();
  }

  void load(const char *mname)
  {
    demo::stopifrecording();
    world::cleardlights();
    edit::pruneundos();
    setnames(mname);
    gzFile f = gzopen(cgzname, "rb9");
    if (!f) {
      console::out("could not read map %s", cgzname);
      return;
    }
    gzread(f, &hdr, sizeof(header)-sizeof(int)*16);
    endianswap(&hdr.version, sizeof(int), 4);
    if (strncmp(hdr.head, "CUBE", 4)!=0)
      fatal("while reading map: header malformatted");
    if (hdr.version>MAPVERSION)
      fatal("this map requires a newer version of cube");
    if (sfactor<SMALLEST_FACTOR || sfactor>LARGEST_FACTOR)
      fatal("illegal map size");
    if (hdr.version>=4) {
      gzread(f, &hdr.waterlevel, sizeof(int)*16);
      endianswap(&hdr.waterlevel, sizeof(int), 16);
    } else
      hdr.waterlevel = -100000;
    ents.setsize(0);
    loopi(hdr.numents) {
      entity &e = ents.add();
      gzread(f, &e, sizeof(persistent_entity));
      endianswap(&e, sizeof(short), 4);
      e.spawned = false;
      if (e.type==LIGHT) {
        if (!e.attr2)
          e.attr2 = 255;  // needed for MAPVERSION<=2
        if (e.attr1>32)
          e.attr1 = 32; // 12_03 and below
      }
    }
    free(map);
    world::setup(hdr.sfactor);
    char texuse[256];
    loopi(256) texuse[i] = 0;
    sqr *t = NULL;
    loopk(cubicsize) {
      sqr *s = &map[k];
      int type = gzgetc(f);
      switch (type) {
        case 255:
        {
          int n = gzgetc(f);
          for (int i = 0; i<n; i++, k++) memcpy(&map[k], t, sizeof(sqr));
          k--;
          break;
        }
        case 254: // only in MAPVERSION<=2
        {
          memcpy(s, t, sizeof(sqr));
          s->r = s->g = s->b = gzgetc(f);
          gzgetc(f);
          break;
        }
        case SOLID:
        {
          s->type = SOLID;
          s->wtex = gzgetc(f);
          s->vdelta = gzgetc(f);
          if (hdr.version<=2) {
            gzgetc(f);
            gzgetc(f);
          }
          s->ftex = DEFAULT_FLOOR;
          s->ctex = DEFAULT_CEIL;
          s->utex = s->wtex;
          s->tag = 0;
          s->floor = 0;
          s->ceil = 16;
          break;
        }
        default:
        {
          if (type<0 || type>=MAXTYPE) {
            sprintf_sd(t)("%d @ %d", type, k);
            fatal("while reading map: type out of range: ", t);
          }
          s->type = type;
          s->floor = gzgetc(f);
          s->ceil = gzgetc(f);
          if (s->floor>=s->ceil)
            s->floor = s->ceil-1;  // for pre 12_13
          s->wtex = gzgetc(f);
          s->ftex = gzgetc(f);
          s->ctex = gzgetc(f);
          if (hdr.version<=2) {
            gzgetc(f);
            gzgetc(f);
          }
          s->vdelta = gzgetc(f);
          s->utex = (hdr.version>=2) ? gzgetc(f) : s->wtex;
          s->tag = (hdr.version>=5) ? gzgetc(f) : 0;
          s->type = type;
        }
      }
      s->defer = 0;
      t = s;
      texuse[s->wtex] = 1;
      if (!SOLID(s))
        texuse[s->utex] = texuse[s->ftex] = texuse[s->ctex] = 1;
    }
    gzclose(f);
    world::calclight();
    world::settagareas();
    int xs, ys;

    loopi(256) rdr::ogl::lookuptex(i, xs, ys);
    console::out("read map %s (%d milliseconds)", cgzname, SDL_GetTicks()-lastmillis);
    console::out("%s", hdr.maptitle);
    game::startmap(mname);
    loopl(256) {
      // can this be done smarter?
      sprintf_sd(aliasname)("level_trigger_%d", l);
      if (cmd::identexists(aliasname))
        cmd::alias(aliasname, "");
    }
    cmd::execfile("data/default_map_settings.cfg");
    cmd::execfile(pcfname);
    cmd::execfile(mcfname);
  }

  COMMANDN(savemap, save, ARG_1STR);
  COMMAND(mapenlarge, ARG_NONE);
  COMMAND(newmap, ARG_1INT);
  COMMANDN(recalc, calclight, ARG_NONE);
  COMMAND(delent, ARG_NONE);
  COMMAND(clearents, ARG_1STR);
  COMMAND(entproperty, ARG_2INT);
  COMMAND(trigger, ARG_2INT);
  COMMAND(scalelights, ARG_2INT);
  COMMAND(toggleocull, ARG_NONE);

} /* namespace world */



