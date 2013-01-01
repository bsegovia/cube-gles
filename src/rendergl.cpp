#include "cube.h"
#include <GL/gl.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

// XXX
int xtraverts;

namespace rdr { extern int curvert; }

namespace rdr {
namespace ogl {

  /* matrix handling. very inspired by opengl :-) */
  enum {MATRIX_STACK = 4};
  static mat4x4f vp[MATRIX_MODE] = {one, one};
  static mat4x4f vpstack[MATRIX_STACK][MATRIX_MODE];
  static int vpdepth = 0;
  static int vpmode = MODELVIEW;
  static bool vploaded[MATRIX_MODE] = {false,false}; /* XXX just to support OGL 1.x */

  void matrixmode(int mode) { vpmode = mode; }
  const mat4x4f &matrix(int mode) { return vp[mode]; }
  void loadmatrix(const mat4x4f &m) {
    vploaded[vpmode] = false;
    vp[vpmode] = m;
  }
  void identity(void) {
    vploaded[vpmode] = false;
    vp[vpmode] = mat4x4f(one);
  }
  void translate(const vec3f &v) {
    vploaded[vpmode] = false;
    vp[vpmode] = translate(vp[vpmode],v);
  }
  void mulmatrix(const mat4x4f &m) {
    vploaded[vpmode] = false;
    vp[vpmode] = m*vp[vpmode];
  }
  void rotate(float angle, const vec3f &axis) {
    vploaded[vpmode] = false;
    vp[vpmode] = rotate(vp[vpmode],angle,axis);
  }
  void perspective(float fovy, float aspect, float znear, float zfar) {
    vploaded[vpmode] = false;
    vp[vpmode] = vp[vpmode]*::perspective(fovy,aspect,znear,zfar);
  }
  void ortho(float left, float right, float bottom, float top, float znear, float zfar) {
    vploaded[vpmode] = false;
    vp[vpmode] = vp[vpmode]*::ortho(left,right,bottom,top,znear,zfar);
  }
  void scale(const vec3f &s) {
    vploaded[vpmode] = false;
    vp[vpmode] = scale(vp[vpmode],s);
  }
  void pushmatrix(void) {
    assert(vpdepth+1<MATRIX_STACK);
    vpstack[vpdepth++][vpmode] = vp[vpmode];
  }
  void popmatrix(void) {
    vploaded[vpmode] = false;
    assert(vpdepth>0);
    vp[vpmode] = vpstack[--vpdepth][vpmode];
  }
  static void loadmatrices(void) { /* XXX remove all that when OGL 1 is removed ? */
    if (!vploaded[MODELVIEW]) {
      OGL(MatrixMode, GL_MODELVIEW);
      OGL(LoadMatrixf, &vp[MODELVIEW][0][0]);
    }
    if (!vploaded[PROJECTION]) {
      OGL(MatrixMode, GL_PROJECTION);
      OGL(LoadMatrixf, &vp[PROJECTION][0][0]);
    }
    vploaded[MODELVIEW] = vploaded[PROJECTION] = true;
  }
  void drawarrays(int mode, int first, int count) {
    loadmatrices();
    OGL(DrawArrays, mode, first, count);
  }
  void drawelements(int mode, int count, int type, const void *indices) {
    loadmatrices();
    OGL(DrawElements, mode, count, type, indices);
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

  /* sphere management */
  static GLuint spherevbo = 0;
  static int spherevertn = 0;

  static float overbrightf = 1.f;

  static void buildsphere(float radius, int slices, int stacks)
  {
    vector<vvec<5>> v;
    loopj(stacks) {
      const float angle0 = M_PI * float(j) / float(stacks);
      const float angle1 = M_PI * float(j+1) / float(stacks);
      const float zLow = radius * cosf(angle0);
      const float zHigh = radius * cosf(angle1);
      const float sin1 = radius * sinf(angle0);
      const float sin2 = radius * sinf(angle1);

      loopi(slices+1) {
        const float angle = 2.f * M_PI * float(i) / float(slices);
        const float sin0 = sinf(angle);
        const float cos0 = cosf(angle);
        const int start = (i==0&&j!=0)?2:1;
        const int end = (i==slices&&j!=stacks-1)?2:1;
        loopk(start) { /* stick the strips together */
          const float s = 1.f-float(i)/slices, t = 1.f-float(j)/stacks;
          const float x = sin1*sin0, y = sin1*cos0, z = zLow;
          v.add(vvec<5>(s, t, x, y, z));
        }
        loopk(end) { /* idem */
          const float s = 1.f-float(i)/slices, t = 1.f-float(j+1)/stacks;
          const float x = sin2*sin0, y = sin2*cos0, z = zHigh;
          v.add(vvec<5>(s, t, x, y, z));
        }
        spherevertn += start+end;
      }
    }

    const size_t sz = sizeof(vvec<5>) * v.length();
    OGL(GenBuffers, 1, &spherevbo);
    OGL(BindBuffer, GL_ARRAY_BUFFER, spherevbo);
    OGL(BufferData, GL_ARRAY_BUFFER, sz, &v[0][0], GL_STATIC_DRAW);
    OGL(BindBuffer, GL_ARRAY_BUFFER, 0);
  }

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
    OGL(BindTexture, GL_TEXTURE_2D, tnum);
    OGL(PixelStorei, GL_UNPACK_ALIGNMENT, 1);
    OGL(TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    OGL(TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    OGL(TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    OGL(TexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    OGL(TexEnvi, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    xs = s->w;
    ys = s->h;
    if (xs>glmaxtexsize || ys>glmaxtexsize) fatal("texture dimensions are too large");
    OGL(TexImage2D, GL_TEXTURE_2D, 0, GL_RGB, xs, ys, 0, GL_RGB, GL_UNSIGNED_BYTE, s->pixels);
    OGL(GenerateMipmap, GL_TEXTURE_2D);
    SDL_FreeSurface(s);
    return true;
  }

  static bool checkshader(GLuint shadername)
  {
    GLint result = GL_FALSE;
    int infologlength;

    if (!shadername) return false;
    OGL(GetShaderiv, shadername, GL_COMPILE_STATUS, &result);
    OGL(GetShaderiv, shadername, GL_INFO_LOG_LENGTH, &infologlength);
    if (infologlength) {
      char *buffer = new char[infologlength + 1];
      buffer[infologlength] = 0;
      OGL(GetShaderInfoLog, shadername, infologlength, NULL, buffer);
      printf(buffer);
      delete [] buffer;
    }
    if (result == GL_FALSE) fatal("OGL: failed to compile shader");
    return result == GL_TRUE;
  }

  /* mostly to define an quick and dirty uber-shader */
  static const int subtypen = 3;
  static const int shadern = 1<<subtypen;
  static const int FOG = 1<<0;
  static const int KEYFRAME = 1<<1;
  static const int DIFFUSETEX = 1<<2;

  static GLuint loadshader(GLenum type, const char *source, const char *rulestr)
  {
    const GLuint name = glCreateShader(type);
    const char *sources[] = {rulestr, source};
    OGL(ShaderSource, name, 2, sources, NULL);
    OGL(CompileShader, name);
    if (!checkshader(name)) fatal("OGL: shader not valid");
    return name;
  }

  static GLuint loadprogram(const char *vertstr, const char *fragstr, uint rules)
  {
    GLuint program = 0;
    sprintf_sd(rulestr)("#version 130\n"
                        "#define USE_FOG %i\n"
                        "#define USE_KEYFRAME %i\n"
                        "#define USE_DIFFUSETEX %i\n",
                        rules&FOG,rules&KEYFRAME,rules&DIFFUSETEX);
    const GLuint vert = loadshader(GL_VERTEX_SHADER, vertstr, rulestr);
    const GLuint frag = loadshader(GL_FRAGMENT_SHADER, fragstr, rulestr);
    OGLR(program, CreateProgram);
    OGL(AttachShader, program, vert);
    OGL(AttachShader, program, frag);
    OGL(DeleteShader, vert);
    OGL(DeleteShader, frag);
    return program;
  }

  static const char ubervert[] = {
    "uniform mat4 MVP;\n"
    "#if USE_FOG\n"
    "  uniform vec4 zaxis;\n"
    "  out float fogz;\n"
    "#endif\n"
    "#if USE_KEYFRAME\n"
    "  uniform float delta;\n"
    "  in vec3 p0, p1;\n"
    "#else\n"
    "  in vec3 p;\n"
    "#endif\n"
    "in vec3 incol;\n"
    "#if USE_DIFFUSETEX\n"
    "  out vec2 texcoord;\n"
    "  in vec2 t;\n"
    "#endif\n"
    "out vec3 outcol;\n"
    "void main() {\n"
    "#if USE_DIFFUSETEX\n"
    "  texcoord = t;\n"
    "#endif\n"
    "  outcol = incol;\n"
    "#if USE_KEYFRAME\n"
    "  const vec3 p = mix(p0,p1,delta);\n"
    "#endif\n"
    "#if USE_FOG\n"
    "  fogz = dot(zaxis,vec4(p,1.0));\n"
    "#endif\n"
    "  gl_Position = MVP * vec4(p,1.0);\n"
    "}\n"
  };
  static const char uberfrag[] = {
    "#if USE_DIFFUSETEX\n"
    "  uniform sampler2D diffuse;\n"
    "  in vec2 texcoord;\n"
    "#endif\n"
    "#if USE_FOG\n"
    "  uniform vec4 fogcolor;\n"
    "  uniform vec2 fogstartend;\n"
    "  in float fogz;\n"
    "#endif\n"
    "uniform float overbright;\n"
    "in vec3 outcol;\n"
    "out vec4 c;\n"
    "void main() {\n"
    "  vec4 col;\n"
    "#if USE_DIFFUSETEX\n"
    "  col = texture(diffuse, texcoord);\n"
    "  col.xyz *= outcol.xyz;\n"
    "#else\n"
    "  col = vec4(outcol.xyz,1.0);\n"
    "#endif\n"
    "#if USE_FOG\n"
    "  const float factor = clamp((-fogz-fogstartend.x)*fogstartend.y,0.0,1.0)\n;"
    "  col.xyz = mix(col.xyz,fogcolor.xyz,factor);\n"
    "#endif\n"
    "  col.xyz *= overbright;\n"
    "  c = col;\n"
    "}\n"
  };

  static struct shader {
    uint rules; /* fog,keyframe...? */
    GLuint program; /* ogl program */
    GLuint udiffuse, udelta, umvp, uoverbright; /* uniforms */
    GLuint uzaxis, ufogstartend, ufogcolor; /* uniforms */
  } shaders[shadern];

  static float fogcolor[4];
  static float fogstartend[2];

  static void bindshader(shader shader, float lerp=0.f)
  {
    /* XXX DO IT ELSEWHERE ONLY ONCE! */
    const mat4x4f viewproj = vp[PROJECTION]*vp[MODELVIEW];
    OGL(UseProgram, shader.program);
    if (shader.rules & FOG) {
      const mat4x4f &mv = vp[MODELVIEW];
      const vec4f zaxis(mv.vx.z,mv.vy.z,mv.vz.z,mv.vw.z);
      OGL(Uniform2fv, shader.ufogstartend, 1, fogstartend);
      OGL(Uniform4fv, shader.ufogcolor, 1, fogcolor);
      OGL(Uniform4fv, shader.uzaxis, 1, &zaxis.x);
    }
    if (shader.rules & KEYFRAME) OGL(Uniform1f, shader.udelta, lerp);
    OGL(UniformMatrix4fv, shader.umvp, 1, GL_FALSE, &viewproj.vx.x);
    OGL(Uniform1f, shader.uoverbright, overbrightf);
  }

  static void unbindshader(void) { OGL(UseProgram, 0); }

  void rendermd2(const float *pos0, const float *pos1, float lerp, int n)
  {
    OGL(DisableClientState, GL_VERTEX_ARRAY);
    OGL(DisableClientState, GL_TEXTURE_COORD_ARRAY);
    OGL(VertexAttribPointer, TEX, 2, GL_FLOAT, 0, sizeof(float[5]), pos0);
    OGL(VertexAttribPointer, POS0, 3, GL_FLOAT, 0, sizeof(float[5]), pos0+2);
    OGL(VertexAttribPointer, POS1, 3, GL_FLOAT, 0, sizeof(float[5]), pos1+2);
    OGL(EnableVertexAttribArray, POS0);
    OGL(EnableVertexAttribArray, POS1);
    OGL(EnableVertexAttribArray, TEX);
    bindshader(shaders[FOG|KEYFRAME|DIFFUSETEX], lerp);
    OGL(DisableClientState, GL_COLOR_ARRAY);
    drawarrays(GL_TRIANGLES, 0, n);
    unbindshader();
    OGL(DisableVertexAttribArray, POS0);
    OGL(DisableVertexAttribArray, POS1);
    OGL(DisableVertexAttribArray, TEX);
    OGL(EnableClientState, GL_COLOR_ARRAY);
    OGL(EnableClientState, GL_VERTEX_ARRAY);
    OGL(EnableClientState, GL_TEXTURE_COORD_ARRAY);
  }

  static void buildshader(shader &shader, uint rules)
  {
    memset(&shader, 0, sizeof(struct shader));
    shader.program = loadprogram(ubervert, uberfrag, rules);
    shader.rules = rules;
    if (rules&KEYFRAME) {
      OGL(BindAttribLocation, shader.program, POS0, "p0");
      OGL(BindAttribLocation, shader.program, POS1, "p1");
    } else
      OGL(BindAttribLocation, shader.program, POS0, "p");
    if (rules&DIFFUSETEX)
      OGL(BindAttribLocation, shader.program, TEX, "t");
    OGL(BindAttribLocation, shader.program, COL, "incol");
    OGL(BindFragDataLocation, shader.program, 0, "c");
    OGL(LinkProgram, shader.program);
    OGL(ValidateProgram, shader.program);
    OGL(UseProgram, shader.program);
    OGLR(shader.uoverbright, GetUniformLocation, shader.program, "overbright");
    OGLR(shader.umvp, GetUniformLocation, shader.program, "MVP");
    if (rules&KEYFRAME)
      OGLR(shader.udelta, GetUniformLocation, shader.program, "delta");
    else
      shader.udelta = 0;
    if (rules&DIFFUSETEX) {
      OGLR(shader.udiffuse, GetUniformLocation, shader.program, "diffuse");
      OGL(Uniform1i, shader.udiffuse, 0);
    }
    if (rules&FOG) {
      OGLR(shader.uzaxis, GetUniformLocation, shader.program, "zaxis");
      OGLR(shader.ufogcolor, GetUniformLocation, shader.program, "fogcolor");
      OGLR(shader.ufogstartend, GetUniformLocation, shader.program, "fogstartend");
    }
    OGL(UseProgram, 0);
  }

  void init(int w, int h)
  {
    OGL(Viewport, 0, 0, w, h);
    glClearDepth(1.f);
    OGL(DepthFunc, GL_LESS);
    OGL(Enable, GL_DEPTH_TEST);
    OGL(CullFace, GL_FRONT);
    OGL(Enable, GL_CULL_FACE);
    OGL(GetIntegerv, GL_MAX_TEXTURE_SIZE, &glmaxtexsize);

    purgetextures();
    buildsphere(1, 12, 6);
    loopi(shadern) buildshader(shaders[i], i);
  }

  void clean(void) { OGL(DeleteBuffers, 1, &spherevbo); }

  void drawsphere(void)
  {
    OGL(BindBuffer, GL_ARRAY_BUFFER, spherevbo);
    drawarray(GL_TRIANGLE_STRIP, 3, 2, spherevertn, NULL);
    OGL(BindBuffer, GL_ARRAY_BUFFER, 0);
  }

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
    int frame = 0; /* other frames? */
    int tid = mapping[tex][frame];

    if (tid>=FIRSTTEX) {
      xs = texx[tid-FIRSTTEX];
      ys = texy[tid-FIRSTTEX];
      return tid;
    }

    xs = ys = 16;
    if (!tid) return 1; /* crosshair :) */

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
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    OGL(EnableVertexAttribArray, POS0);
    OGL(EnableVertexAttribArray, COL);
    OGL(EnableVertexAttribArray, TEX);
    setarraypointers2();
  }

  static int skyoglid;
  struct strip { int tex, start, num; };
  static vector<strip> strips;

  static void renderstripssky(void)
  {
    OGL(BindTexture, GL_TEXTURE_2D, skyoglid);
    loopv(strips)
      if (strips[i].tex==skyoglid)
        ogl::drawarrays(GL_TRIANGLE_STRIP, strips[i].start, strips[i].num);
  }

  static void renderstrips(void)
  {
    int lasttex = -1;
    loopv(strips) if (strips[i].tex!=skyoglid) {
      if (strips[i].tex!=lasttex) {
        OGL(BindTexture, GL_TEXTURE_2D, strips[i].tex);
        lasttex = strips[i].tex;
      }
      ogl::drawarrays(GL_TRIANGLE_STRIP, strips[i].start, strips[i].num);
    }
  }

  static void overbright(float amount) { overbrightf = amount; }

  void addstrip(int tex, int start, int n)
  {
    strip &s = strips.add();
    s.tex = tex;
    s.start = start;
    s.num = n;
  }

  static const vec3f roll(0.f,0.f,1.f);
  static const vec3f pitch(-1.f,0.f,0.f);
  static const vec3f yaw(0.f,1.f,0.f);
  static void transplayer(void)
  {
    identity();
    rotate(player1->roll,roll);
    rotate(player1->pitch,pitch);
    rotate(player1->yaw,yaw);
    translate(vec3f(-player1->o.x, (player1->state==CS_DEAD ? player1->eyeheight-0.2f : 0)-player1->o.z, -player1->o.y));
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
    if (!hudgun) return;

    OGL(Enable, GL_CULL_FACE);

    matrixmode(PROJECTION);
    identity();
    perspective(fovy, aspect, 0.3f, farplane);
    matrixmode(MODELVIEW);

    const int rtime = weapon::reloadtime(player1->gunselect);
    if (player1->lastaction &&
        player1->lastattackgun==player1->gunselect &&
        lastmillis-player1->lastaction<rtime)
      drawhudmodel(7, 18, rtime/18.0f, player1->lastaction);
    else
      drawhudmodel(6, 1, 100, 0);

    matrixmode(PROJECTION);
    identity();
    perspective(fovy, aspect, 0.15f, farplane);
    matrixmode(MODELVIEW);

    OGL(Disable, GL_CULL_FACE);
  }

  /* XXX remove that when use of shaders is done */
  void drawarray(int mode, size_t pos, size_t tex, size_t n, const float *data)
  {
    OGL(DisableClientState, GL_COLOR_ARRAY);
    if (!tex) OGL(DisableClientState, GL_TEXTURE_COORD_ARRAY);
    OGL(VertexPointer, pos, GL_FLOAT, (pos+tex)*sizeof(float), data+tex);
    if (tex) OGL(TexCoordPointer, tex, GL_FLOAT, (pos+tex)*sizeof(float), data);
    ogl::drawarrays(mode, 0, n);
    if (!tex) OGL(EnableClientState, GL_TEXTURE_COORD_ARRAY);
    OGL(EnableClientState, GL_COLOR_ARRAY);
  }

  void drawframe(int w, int h, float curfps)
  {
    const float hf = hdr.waterlevel-0.3f;
    const bool underwater = player1->o.z<hf;
    float fovy = (float)fov*h/w;
    float aspect = w/(float)h;

    fogstartend[0] = float((fog+64)/8);
    fogstartend[1] = 1.f/(float(fog)-fogstartend[0]);

    const float fogc[4] = {
      (fogcolour>>16)/256.0f,
      ((fogcolour>>8)&255)/256.0f,
      (fogcolour&255)/256.0f,
      1.0f
    };
    fogcolor[0] = float(fogcolour>>16)/256.0f;
    fogcolor[1] = float((fogcolour>>8)&255)/256.0f;
    fogcolor[2] = float(fogcolour&255)/256.0f;
    fogcolor[3] = 1.f;

    OGL(ClearColor, fogc[0], fogc[1], fogc[2], 1.0f);

    if (underwater) {
      fovy += (float)sin(lastmillis/1000.0)*2.0f;
      aspect += (float)sin(lastmillis/1000.0+PI)*0.1f;
      fogstartend[0] = 0.f;
      fogstartend[1] = 1.f/float((fog+96)/8);
    }

    OGL(Clear, (player1->outsidemap ? GL_COLOR_BUFFER_BIT : 0) | GL_DEPTH_BUFFER_BIT);

    const int farplane = fog*5/2;
    matrixmode(PROJECTION);
    identity();
    perspective(fovy, aspect, 0.15f, farplane);
    matrixmode(MODELVIEW);

    transplayer();

    OGL(Enable, GL_TEXTURE_2D);

    int xs, ys;
    skyoglid = lookuptex(DEFAULT_SKY, xs, ys);

    resetcubes();

    curvert = 0;
    strips.setsize(0);

    world::render(player1->o.x, player1->o.y, player1->o.z,
                  (int)player1->yaw, (int)player1->pitch, (float)fov, w, h);
    finishstrips();

    setupworld();

    bindshader(shaders[0]);
    renderstripssky();
    OGL(DisableVertexAttribArray, COL);

    identity();
    rotate(player1->pitch, pitch);
    rotate(player1->yaw, yaw);
    rotate(90.f, vec3f(1.f,0.f,0.f));
    OGL(VertexAttrib3f, COL, 1.0f, 1.0f, 1.0f);
    OGL(DepthFunc, GL_GREATER);
    bindshader(shaders[DIFFUSETEX]);
    draw_envbox(14, fog*4/3);
    OGL(DepthFunc, GL_LESS);
    OGL(DisableVertexAttribArray, POS0); /* XXX REMOVE! */
    OGL(DisableVertexAttribArray, TEX);
    unbindshader();

    transplayer();

    overbright(2);

    setupworld(); /* XXX REMOVE ! */
    bindshader(shaders[DIFFUSETEX|FOG]);
    renderstrips();
    unbindshader();
    OGL(DisableVertexAttribArray, POS0); /* XXX REMOVE! */
    OGL(DisableVertexAttribArray, COL);
    OGL(DisableVertexAttribArray, TEX);

    xtraverts = 0;

    game::renderclients();
    monster::monsterrender();

    entities::renderentities();

    renderspheres(curtime);
    rdr::renderents();

    OGL(Disable, GL_CULL_FACE);

    drawhudgun(fovy, aspect, farplane);

    overbright(1);
    setupworld(); /* XXX REMOVE ! */
    bindshader(shaders[DIFFUSETEX]);
    const int nquads = renderwater(hf);
    unbindshader();
    OGL(DisableVertexAttribArray, POS0); /* XXX REMOVE! */
    OGL(DisableVertexAttribArray, COL);
    OGL(DisableVertexAttribArray, TEX);

    overbright(2);
    bindshader(shaders[DIFFUSETEX]);
    render_particles(curtime);
    unbindshader();
    overbright(1);

    OGL(Disable, GL_TEXTURE_2D);

    drawhud(w, h, (int)curfps, nquads, curvert, underwater);

    OGL(Enable, GL_CULL_FACE);
  }
} /* namespace ogl */
} /* namespace rdr */

