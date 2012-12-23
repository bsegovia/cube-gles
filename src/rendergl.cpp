#include "cube.h"
#include <GL/gl.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

// XXX
int xtraverts;
bool hasoverbright = false;

namespace rdr { extern int curvert; }

namespace rdr {
namespace ogl {

  static const GLuint spherevbo = 1; /* contains the sphere data */
  static int spherevertn = 0;

  void sphere(float radius, int slices, int stacks, vector<vvec<5>> &v)
  {
    loopj(stacks) {
      const float angle0 = M_PI * float(j) / float(stacks);
      const float angle1 = M_PI * float(j+1) / float(stacks);
      const float zLow = radius * cosf(angle0);
      const float zHigh = radius * cosf(angle1);
      const float sin1 = radius * sinf(angle0);
      const float sin2 = radius * sinf(angle1);
      // const float sin3 = -sinf(angle0);
      // const float cos3 = -cosf(angle0);
      // const float sin4 = -sinf(angle1);
      // const float cos4 = -cosf(angle1);

      loopi(slices+1) {
        const float angle = 2.f * M_PI * float(i) / float(slices);
        const float sin0 = sinf(angle);
        const float cos0 = cosf(angle);
        const int start = (i==0&&j!=0)?2:1;
        const int end = (i==slices&&j!=stacks-1)?2:1;
        loopk(start) { /* stick the strips together */
          const float s = 1.f-float(i)/slices, t = 1.f-float(j)/stacks;
          const float x = sin1*sin0, y = sin1*cos0, z = zLow;
          // glNormal3f(sin0*sin3, cos0*sin3, cos3);
          v.add(vvec<5>(s, t, x, y, z));
        }

        loopk(end) { /* idem */
          const float s = 1.f-float(i)/slices, t = 1.f-float(j+1)/stacks;
          const float x = sin2*sin0, y = sin2*cos0, z = zHigh;
          // glNormal3f(sin0*sin4, cos0*sin4, cos4);
          v.add(vvec<5>(s, t, x, y, z));
        }
        spherevertn += start+end;
      }
    }
  }

  /* management of texture slots each texture slot can have multople texture
   * frames, of which currently only the first is used additional frames can be
   * used for various shaders
   */
  static const int MAXTEX = 1000;
  static const int FIRSTTEX = 1000; /* opengl id = loaded id + FIRSTTEX */
  static const int MAXFRAMES = 2; /* increase for more complex shader defs */
  static int texx[MAXTEX]; /* (loaded texture) -> (name, size) */
  static int texy[MAXTEX];
  static string texname[MAXTEX];
  static int curtex = 0;
  static int glmaxtexsize = 256;
  static int curtexnum = 0;

  /* std 1+, sky 14+, mdls 20+ */
  static int mapping[256][MAXFRAMES]; /* (texture, frame) -> (oglid, name) */
  static string mapname[256][MAXFRAMES];

  static void purgetextures(void) {
    loopi(256) loop(j,MAXFRAMES) mapping[i][j] = 0;
  }

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

  void init(int w, int h)
  {
    glViewport(0, 0, w, h);
    glClearDepth(1.f);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);

    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_DENSITY, 0.25f);
    glHint(GL_FOG_HINT, GL_NICEST);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glCullFace(GL_FRONT);
    glEnable(GL_CULL_FACE);

    const char *exts = (const char *)glGetString(GL_EXTENSIONS);

    if (strstr(exts, "GL_EXT_texture_env_combine"))
      hasoverbright = true;
    else
      console::out("WARNING: cannot use overbright lighting");

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &glmaxtexsize);

    purgetextures();
    vector<vvec<5>> v;
    sphere(1, 12, 6, v);
    const size_t sz = sizeof(vvec<5>) * v.length();
    OGL(BindBuffer, GL_ARRAY_BUFFER, spherevbo);
    OGL(BufferData, GL_ARRAY_BUFFER, sz, &v[0][0], GL_STATIC_DRAW);
    OGL(BindBuffer, GL_ARRAY_BUFFER, 0);
  }

  void drawsphere(void)
  {
    OGL(BindBuffer, GL_ARRAY_BUFFER, spherevbo);
    drawarray(GL_TRIANGLE_STRIP, 3, 2, spherevertn, NULL);
    OGL(BindBuffer, GL_ARRAY_BUFFER, 0);
  }

  void clean(void) {}

  static void texturereset(void) { curtexnum = 0; }

  static void texture(char *aframe, char *name)
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

  int lookuptex(int tex, int &xs, int &ys)
  {
    int frame = 0;  /* other frames? */
    int tid = mapping[tex][frame];

    if (tid>=FIRSTTEX) {
      xs = texx[tid-FIRSTTEX];
      ys = texy[tid-FIRSTTEX];
      return tid;
    }

    xs = ys = 16;
    if (!tid) return 1;                  // crosshair :)

    loopi(curtex) { /* lazily happens once per "texture" command */
      if (strcmp(mapname[tex][frame], texname[i])==0) {
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

    if (installtex(tnum, name, xs, ys)) {
      mapping[tex][frame] = tnum;
      texx[curtex] = xs;
      texy[curtex] = ys;
      curtex++;
      return tnum;
    } else
      return mapping[tex][frame] = FIRSTTEX;  // temp fix
  }

  static void setupworld(void)
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

  static int skyoglid;
  struct strip { int tex, start, num; };
  static vector<strip> strips;

  static void renderstripssky(void)
  {
    glBindTexture(GL_TEXTURE_2D, skyoglid);
    loopv(strips)
      if (strips[i].tex==skyoglid)
        glDrawArrays(GL_TRIANGLE_STRIP, strips[i].start, strips[i].num);
  }

  static void renderstrips(void)
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

  static void overbright(float amount)
  {
    if (hasoverbright)
      glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, amount);
  }

  void addstrip(int tex, int start, int n)
  {
    strip &s = strips.add();
    s.tex = tex;
    s.start = start;
    s.num = n;
  }

  static void transplayer(void)
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

  static void drawhudmodel(int start, int end, float speed, int base)
  {
    rendermodel(hudgunnames[player1->gunselect], start, end, 0, 1.0f, player1->o.x, player1->o.z, player1->o.y, player1->yaw+90, player1->pitch, false, 1.0f, speed, 0, base);
  }

  static void drawhudgun(float fovy, float aspect, int farplane)
  {
    double p[16];
    if (!hudgun) return;

    glEnable(GL_CULL_FACE);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    perspective(p, fovy, aspect, 0.3f, farplane);
    glMultMatrixd(p);
    glMatrixMode(GL_MODELVIEW);

    const int rtime = weapon::reloadtime(player1->gunselect);
    if (player1->lastaction &&
        player1->lastattackgun==player1->gunselect &&
        lastmillis-player1->lastaction<rtime)
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

  void drawarray(int mode, size_t pos, size_t tex, size_t n, const float *data)
  {
    OGL(DisableClientState, GL_COLOR_ARRAY);
    if (!tex) OGL(DisableClientState, GL_TEXTURE_COORD_ARRAY);
    OGL(VertexPointer, pos, GL_FLOAT, (pos+tex)*sizeof(float), data+tex);
    if (tex) OGL(TexCoordPointer, tex, GL_FLOAT, (pos+tex)*sizeof(float), data);
    OGL(DrawArrays, mode, 0, n);
    if (!tex) OGL(EnableClientState, GL_TEXTURE_COORD_ARRAY);
    OGL(EnableClientState, GL_COLOR_ARRAY);
  }

  void drawframe(int w, int h, float curfps)
  {
    float hf = hdr.waterlevel-0.3f;
    float fovy = (float)fov*h/w;
    float aspect = w/(float)h;
    bool underwater = player1->o.z<hf;
    double p[16];

    glFogi(GL_FOG_START, (fog+64)/8);
    glFogi(GL_FOG_END, fog);
    const float fogc[4] = {
      (fogcolour>>16)/256.0f,
      ((fogcolour>>8)&255)/256.0f,
      (fogcolour&255)/256.0f,
      1.0f
    };
    glFogfv(GL_FOG_COLOR, fogc);
    glClearColor(fogc[0], fogc[1], fogc[2], 1.0f);

    if (underwater) {
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
    skyoglid = lookuptex(DEFAULT_SKY, xs, ys);

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

    setupworld();
    transplayer();

    overbright(2);

    renderstrips();

    xtraverts = 0;

    game::renderclients();
    monster::monsterrender();

    entities::renderentities();

    renderspheres(curtime);
    rdr::renderents();

    glDisable(GL_CULL_FACE);

    drawhudgun(fovy, aspect, farplane);

    overbright(1);
    const int nquads = renderwater(hf);

    overbright(2);
    render_particles(curtime);
    overbright(1);

    glDisable(GL_FOG);

    glDisable(GL_TEXTURE_2D);

    drawhud(w, h, (int)curfps, nquads, curvert, underwater);

    glEnable(GL_CULL_FACE);
    glEnable(GL_FOG);
  }
} /* namespace ogl */
} /* namespace rdr */


