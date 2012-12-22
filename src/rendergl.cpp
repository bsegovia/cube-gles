#include "cube.h"
#include <GL/gl.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

// XXX
int xtraverts;
bool hasoverbright = false;

namespace renderer
{
  extern int curvert;

  void purgetextures(void);

  int glmaxtexsize = 256;

  void sphere(GLdouble radius, int slices, int stacks)
  {
    const int cache_size = 240;
    float sinCache1a[cache_size];
    float cosCache1a[cache_size];
    float sinCache2a[cache_size];
    float cosCache2a[cache_size];
    float sinCache1b[cache_size];
    float cosCache1b[cache_size];
    float sinCache2b[cache_size];
    float cosCache2b[cache_size];
    float angle;
    float zLow, zHigh;
    float sintemp1 = 0.0, sintemp2 = 0.0, sintemp3 = 0.0, sintemp4 = 0.0;
    float costemp3 = 0.0, costemp4 = 0.0;
    int start, finish;
    if (slices >= cache_size) slices = cache_size-1;
    if (stacks >= cache_size) stacks = cache_size-1;
    if (slices < 2 || stacks < 1 || radius < 0.0) return;

    for (int i = 0; i < slices; i++) {
      angle = 2 * PI * i / slices;
      sinCache1a[i] = sinf(angle);
      cosCache1a[i] = cosf(angle);
      sinCache2a[i] = sinCache1a[i];
      cosCache2a[i] = cosCache1a[i];
    }

    for (int j = 0; j <= stacks; j++) {
      angle = PI * j / stacks;
      sinCache2b[j] = -sinf(angle);
      cosCache2b[j] = -cosf(angle);
      sinCache1b[j] = radius * sinf(angle);
      cosCache1b[j] = radius * cosf(angle);
    }

    sinCache1b[0] = 0;
    sinCache1b[stacks] = 0;
    sinCache1a[slices] = sinCache1a[0];
    cosCache1a[slices] = cosCache1a[0];
    sinCache2a[slices] = sinCache2a[0];
    cosCache2a[slices] = cosCache2a[0];

    start = 0;
    finish = stacks;
    for (int j = start; j < finish; j++) {
      zLow = cosCache1b[j];
      zHigh = cosCache1b[j+1];
      sintemp1 = sinCache1b[j];
      sintemp2 = sinCache1b[j+1];
      sintemp3 = sinCache2b[j];
      costemp3 = cosCache2b[j];
      sintemp4 = sinCache2b[j+1];
      costemp4 = cosCache2b[j+1];

      glBegin(GL_TRIANGLE_STRIP);
      for (int i = 0; i <= slices; i++) {
        glNormal3f(sinCache2a[i]*sintemp3, cosCache2a[i]*sintemp3, costemp3);
        glTexCoord2f(1 - (float) i / slices, 1 - (float) j / stacks);
        glVertex3f(sintemp1*sinCache1a[i], sintemp1*cosCache1a[i], zLow);
        glNormal3f(sinCache2a[i]*sintemp4, cosCache2a[i]*sintemp4, costemp4);
        glTexCoord2f(1 - (float) i / slices, 1 - (float) (j+1) / stacks);
        glVertex3f(sintemp2*sinCache1a[i], sintemp2*cosCache1a[i], zHigh);
      }
      glEnd();
    }
  }

  void gl_init(int w, int h)
  {
    glViewport(0, 0, w, h);
    glClearDepth(1.0);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);

    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_DENSITY, 0.25);
    glHint(GL_FOG_HINT, GL_NICEST);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-3.0, -3.0);

    glCullFace(GL_FRONT);
    glEnable(GL_CULL_FACE);

    const char *exts = (const char *)glGetString(GL_EXTENSIONS);

    if (strstr(exts, "GL_EXT_texture_env_combine")) hasoverbright = true;
    else console::out("WARNING: cannot use overbright lighting, using old lighting model!");

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &glmaxtexsize);

    purgetextures();

    glNewList(1, GL_COMPILE);
    sphere(1, 12, 6);
    glEndList();
  }

  void cleangl() {}

  bool installtex(int tnum, const char *texname, int &xs, int &ys, bool clamp)
  {
    SDL_Surface *s = IMG_Load(texname);
    if (!s) {
      console::out("couldn't load texture %s", texname);
      return false;
    } else if (s->format->BitsPerPixel!=24) {
      console::out("texture must be 24bpp: %s", texname);
      return false;
    }
    glBindTexture(GL_TEXTURE_2D, tnum);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    xs = s->w;
    ys = s->h;
    if (xs>glmaxtexsize || ys>glmaxtexsize)
      fatal("texture dimensions are too large");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, xs, ys, 0, GL_RGB, GL_UNSIGNED_BYTE, s->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    SDL_FreeSurface(s);
    return true;
  }

  // management of texture slots each texture slot can have multople texture
  // frames, of which currently only the first is used additional frames can be
  // used for various shaders
  const int MAXTEX = 1000;
  int texx[MAXTEX];            // ( loaded texture ) -> ( name, size )
  int texy[MAXTEX];
  string texname[MAXTEX];
  int curtex = 0;
  const int FIRSTTEX = 1000;   // opengl id = loaded id + FIRSTTEX
  // std 1+, sky 14+, mdls 20+

  const int MAXFRAMES = 2;     // increase to allow more complex shader defs
  int mapping[256][MAXFRAMES]; // ( cube texture, frame ) -> ( opengl id, name )
  string mapname[256][MAXFRAMES];

  void purgetextures() { loopi(256) loop(j,MAXFRAMES) mapping[i][j] = 0; }

  int curtexnum = 0;

  void texturereset() { curtexnum = 0; }

  void texture(char *aframe, char *name)
  {
    int num = curtexnum++, frame = atoi(aframe);
    if (num<0 || num>=256 || frame<0 || frame>=MAXFRAMES) return;
    mapping[num][frame] = 1;
    char *n = mapname[num][frame];
    strcpy_s(n, name);
    path(n);
  }

  COMMAND(texturereset, ARG_NONE);
  COMMAND(texture, ARG_2STR);

  int lookuptexture(int tex, int &xs, int &ys)
  {
    int frame = 0;                      // other frames?
    int tid = mapping[tex][frame];

    if (tid>=FIRSTTEX)
    {
      xs = texx[tid-FIRSTTEX];
      ys = texy[tid-FIRSTTEX];
      return tid;
    }

    xs = ys = 16;
    if (!tid) return 1;                  // crosshair :)

    loopi(curtex)       // lazily happens once per "texture" command, basically
    {
      if (strcmp(mapname[tex][frame], texname[i])==0)
      {
        mapping[tex][frame] = tid = i+FIRSTTEX;
        xs = texx[i];
        ys = texy[i];
        return tid;
      }
    }

    if (curtex==MAXTEX) fatal("loaded too many textures");

    int tnum = curtex+FIRSTTEX;
    strcpy_s(texname[curtex], mapname[tex][frame]);

    sprintf_sd(name)("packages%c%s", PATHDIV, texname[curtex]);

    if (installtex(tnum, name, xs, ys))
    {
      mapping[tex][frame] = tnum;
      texx[curtex] = xs;
      texy[curtex] = ys;
      curtex++;
      return tnum;
    }
    else
    {
      return mapping[tex][frame] = FIRSTTEX;  // temp fix
    }
  }

  void setupworld()
  {
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    setarraypointers();

    if (hasoverbright) {
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
      glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE);
      glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE);
      glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PRIMARY_COLOR_EXT);
    }
  }

  int skyoglid;

  struct strip { int tex, start, num; };
  vector<strip> strips;

  void renderstripssky()
  {
    glBindTexture(GL_TEXTURE_2D, skyoglid);
    loopv(strips) if (strips[i].tex==skyoglid) glDrawArrays(GL_TRIANGLE_STRIP, strips[i].start, strips[i].num);
  }

  void renderstrips(void)
  {
    int lasttex = -1;
    loopv(strips) if (strips[i].tex!=skyoglid) {
      if (strips[i].tex!=lasttex) {
        glBindTexture(GL_TEXTURE_2D, strips[i].tex);
        lasttex = strips[i].tex;
      }
      glDrawArrays(GL_TRIANGLE_STRIP, strips[i].start, strips[i].num);
    }
  }

  void overbright(float amount) { if (hasoverbright) glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, amount ); }

  void addstrip(int tex, int start, int n)
  {
    strip &s = strips.add();
    s.tex = tex;
    s.start = start;
    s.num = n;
  }

  void transplayer()
  {
    glLoadIdentity();
    glRotated(player1->roll,0.0,0.0,1.0);
    glRotated(player1->pitch,-1.0,0.0,0.0);
    glRotated(player1->yaw,0.0,1.0,0.0);
    glTranslated(-player1->o.x, (player1->state==CS_DEAD ? player1->eyeheight-0.2f : 0)-player1->o.z, -player1->o.y);
  }

  VARP(fov, 10, 105, 120);
  VAR(fog, 64, 180, 1024);
  VAR(fogcolour, 0, 0x8099B3, 0xFFFFFF);
  VARP(hudgun,0,1,1);

  static const char *hudgunnames[] = {
    "hudguns/fist",
    "hudguns/shotg",
    "hudguns/chaing",
    "hudguns/rocket",
    "hudguns/rifle"
  };

  void drawhudmodel(int start, int end, float speed, int base)
  {
    rendermodel(hudgunnames[player1->gunselect], start, end, 0, 1.0f, player1->o.x, player1->o.z, player1->o.y, player1->yaw+90, player1->pitch, false, 1.0f, speed, 0, base);
  }

  void drawhudgun(float fovy, float aspect, int farplane)
  {
    double p[16];
    if (!hudgun /*|| !player1->gunselect*/) return;

    glEnable(GL_CULL_FACE);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    perspective(p, fovy, aspect, 0.3f, farplane);
    glMultMatrixd(p);
    glMatrixMode(GL_MODELVIEW);

    //glClear(GL_DEPTH_BUFFER_BIT);
    const int rtime = weapon::reloadtime(player1->gunselect);
    if (player1->lastaction && player1->lastattackgun==player1->gunselect && lastmillis-player1->lastaction<rtime)
      drawhudmodel(7, 18, rtime/18.0f, player1->lastaction);
    else
      drawhudmodel(6, 1, 100, 0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    perspective(p, fovy, aspect, 0.15f, farplane);
    glMultMatrixd(p);
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_CULL_FACE);
  }

  void gl_drawframe(int w, int h, float curfps)
  {
    float hf = hdr.waterlevel-0.3f;
    float fovy = (float)fov*h/w;
    float aspect = w/(float)h;
    bool underwater = player1->o.z<hf;
    double p[16];

    glFogi(GL_FOG_START, (fog+64)/8);
    glFogi(GL_FOG_END, fog);
    float fogc[4] = { (fogcolour>>16)/256.0f, ((fogcolour>>8)&255)/256.0f, (fogcolour&255)/256.0f, 1.0f };
    glFogfv(GL_FOG_COLOR, fogc);
    glClearColor(fogc[0], fogc[1], fogc[2], 1.0f);

    if (underwater)
    {
      fovy += (float)sin(lastmillis/1000.0)*2.0f;
      aspect += (float)sin(lastmillis/1000.0+PI)*0.1f;
      glFogi(GL_FOG_START, 0);
      glFogi(GL_FOG_END, (fog+96)/8);
    }

    glClear((player1->outsidemap ? GL_COLOR_BUFFER_BIT : 0) | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    int farplane = fog*5/2;
    perspective(p, fovy, aspect, 0.15f, farplane);
    glMultMatrixd(p);
    glMatrixMode(GL_MODELVIEW);

    transplayer();

    glEnable(GL_TEXTURE_2D);

    int xs, ys;
    skyoglid = lookuptexture(DEFAULT_SKY, xs, ys);

    resetcubes();

    curvert = 0;
    strips.setsize(0);

    world::render(player1->o.x, player1->o.y, player1->o.z,
        (int)player1->yaw, (int)player1->pitch, (float)fov, w, h);
    finishstrips();

    setupworld();

    renderstripssky();

    glLoadIdentity();
    glRotated(player1->pitch, -1.0, 0.0, 0.0);
    glRotated(player1->yaw,   0.0, 1.0, 0.0);
    glRotated(90.0, 1.0, 0.0, 0.0);
    glColor3f(1.0f, 1.0f, 1.0f);
    glDisable(GL_FOG);
    glDepthFunc(GL_GREATER);
    draw_envbox(14, fog*4/3);
    glDepthFunc(GL_LESS);
    glEnable(GL_FOG);

    transplayer();

    overbright(2);

    renderstrips();

    xtraverts = 0;

    game::renderclients();
    monster::monsterrender();

    entities::renderentities();

    renderspheres(curtime);
    renderer::renderents();

    glDisable(GL_CULL_FACE);

    drawhudgun(fovy, aspect, farplane);

    overbright(1);
    int nquads = renderwater(hf);

    overbright(2);
    render_particles(curtime);
    overbright(1);

    glDisable(GL_FOG);

    glDisable(GL_TEXTURE_2D);

    gl_drawhud(w, h, (int)curfps, nquads, curvert, underwater);

    glEnable(GL_CULL_FACE);
    glEnable(GL_FOG);
  }
} /* namespace renderer */

