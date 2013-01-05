#include "cube.h"
#include "ogl.hpp"
#include <GL/gl.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

// XXX
namespace rdr { extern int curvert; }

namespace ogl
{
  int xtraverts = 0;

  /*-------------------------------------------------------------------------
   - very simple state tracking
   -------------------------------------------------------------------------*/
  union {
    struct {
      uint shader:1; /* will force to reload everything */
      uint mvp:1;
      uint fog:1;
      uint overbright:1;
    } flags;
    uint any;
  } dirty;

  /*--------------------------------------------------------------------------
   - immediate mode and buffer support
   -------------------------------------------------------------------------*/
  static const uint glbufferbinding[BUFFER_NUM] = {
    GL_ARRAY_BUFFER,
    GL_ELEMENT_ARRAY_BUFFER
  };
  static GLuint bindedvbo[BUFFER_NUM] = {0,0};
  void bindbuffer(int target, uint buffer)
  {
    if (bindedvbo[target] != buffer) {
      OGL(BindBuffer, glbufferbinding[target], buffer);
      bindedvbo[target] = buffer;
    }
  }

  /* we use two big circular buffers to handle immediate mode */
  static int bigvbooffset=0, bigibooffset=0;
  static int drawibooffset=0, drawvbooffset=0;
  static GLuint bigvbo=0u, bigibo=0u;

  static void initbuffer(GLuint &bo, int &booffset, int target, int size)
  {
    if (bo == 0u) OGL(GenBuffers, 1, &bo);
    bindbuffer(target, bo);
    OGL(BufferData, glbufferbinding[target], size, NULL, GL_DYNAMIC_DRAW);
    bindbuffer(target, 0);
    booffset = 0u;
  }

  static void bigbufferinit(int size)
  {
    initbuffer(bigvbo, bigvbooffset, ARRAY_BUFFER, size);
    initbuffer(bigibo, bigibooffset, ELEMENT_ARRAY_BUFFER, size);
  }

  VARF(bigbuffersize, 4*MB, 4*MB, 16*MB, bigbufferinit(bigbuffersize));

  void immediate(bool useibo)
  {
    bindbuffer(ARRAY_BUFFER, bigvbo);
    if (useibo) bindbuffer(ELEMENT_ARRAY_BUFFER, bigibo);
  }

  void immediate_setattrib(int attrib, int n, int type, int sz, int offset)
  {
    const void *fake = (const void *) intptr_t(drawvbooffset+offset);
    OGL(VertexAttribPointer, attrib, n, type, 0, sz, fake);
  }

  static void immediate_setdata(int target, int sz, const void *data)
  {
    assert(sz < bigbuffersize);
    GLuint &bo = target==ARRAY_BUFFER ? bigvbo : bigibo;
    int &offset = target==ARRAY_BUFFER ? bigvbooffset : bigibooffset;
    int &drawoffset = target==ARRAY_BUFFER ? drawvbooffset : drawibooffset;
    if (offset+sz > uint(bigbuffersize)) {
      OGL(Flush);
      bindbuffer(target, 0);
      offset = 0u;
      bindbuffer(target, bo);
    }
    bindbuffer(target, bo);
    OGL(BufferSubData, glbufferbinding[target], offset, sz, data);
    drawoffset = offset;
    offset += sz;
  }

  void immediate_setvertices(int sz, const void *vertices)
  {
    immediate_setdata(ARRAY_BUFFER, sz, vertices);
  }

  void immediate_drawarrays(int mode, int count, int type)
  {
    drawarrays(mode,count,type);
  }

  void immediate_drawelements(int mode, int count, int type, const void *indices)
  {
    int sz = count;
    switch (type) {
      case GL_UNSIGNED_BYTE:  sz*=sizeof(uchar); break;
      case GL_UNSIGNED_SHORT: sz*=sizeof(ushort); break;
      case GL_UNSIGNED_INT: sz*=sizeof(uint); break;
    };
    immediate_setdata(ELEMENT_ARRAY_BUFFER, sz, indices);
    const void *fake = (const void *) intptr_t(drawibooffset);
    drawelements(mode, count, type, fake);
  }

  /* XXX remove state crap */
  void immediate_draw(int mode, int pos, int tex, int col, size_t n, const float *data)
  {
    const int sz = (pos+tex+col)*sizeof(float);
    immediate();
    immediate_setvertices(n*sz, data);
    if (pos) {
      immediate_setattrib(ogl::POS0, pos, GL_FLOAT, sz, (tex+col)*sizeof(float));
      OGL(EnableVertexAttribArray, POS0);
    }
    if (tex) {
      immediate_setattrib(ogl::TEX, tex, GL_FLOAT, sz, col*sizeof(float));
      OGL(EnableVertexAttribArray, TEX);
    }
    if (col) {
      immediate_setattrib(ogl::COL, col, GL_FLOAT, sz, 0);
      OGL(EnableVertexAttribArray, COL);
    }
    immediate_drawarrays(mode, 0, n);
    bindbuffer(ARRAY_BUFFER,0); /* XXX BUFFER remove it */
    OGL(DisableVertexAttribArray, TEX);
    OGL(DisableVertexAttribArray, COL);
    OGL(DisableVertexAttribArray, POS0);
  }

  /*-------------------------------------------------------------------------
   - matrix handling. very inspired by opengl :-)
   -------------------------------------------------------------------------*/
  enum {MATRIX_STACK = 4};
  static mat4x4f vp[MATRIX_MODE] = {one, one};
  static mat4x4f vpstack[MATRIX_STACK][MATRIX_MODE];
  static int vpdepth = 0;
  static int vpmode = MODELVIEW;
  static mat4x4f viewproj(one);

  void matrixmode(int mode) { vpmode = mode; }
  const mat4x4f &matrix(int mode) { return vp[mode]; }
  void loadmatrix(const mat4x4f &m) {
    dirty.flags.mvp=1;
    vp[vpmode] = m;
  }
  void identity(void) {
    dirty.flags.mvp=1;
    vp[vpmode] = mat4x4f(one);
  }
  void translate(const vec3f &v) {
    dirty.flags.mvp=1;
    vp[vpmode] = translate(vp[vpmode],v);
  }
  void mulmatrix(const mat4x4f &m) {
    dirty.flags.mvp=1;
    vp[vpmode] = m*vp[vpmode];
  }
  void rotate(float angle, const vec3f &axis) {
    dirty.flags.mvp=1;
    vp[vpmode] = rotate(vp[vpmode],angle,axis);
  }
  void perspective(float fovy, float aspect, float znear, float zfar) {
    dirty.flags.mvp=1;
    vp[vpmode] = vp[vpmode]*::perspective(fovy,aspect,znear,zfar);
  }
  void ortho(float left, float right, float bottom, float top, float znear, float zfar) {
    dirty.flags.mvp=1;
    vp[vpmode] = vp[vpmode]*::ortho(left,right,bottom,top,znear,zfar);
  }
  void scale(const vec3f &s) {
    dirty.flags.mvp=1;
    vp[vpmode] = scale(vp[vpmode],s);
  }
  void pushmatrix(void) {
    assert(vpdepth+1<MATRIX_STACK);
    vpstack[vpdepth++][vpmode] = vp[vpmode];
  }
  void popmatrix(void) {
    assert(vpdepth>0);
    dirty.flags.mvp=1;
    vp[vpmode] = vpstack[--vpdepth][vpmode];
  }

  /*--------------------------------------------------------------------------
   - management of texture slots each texture slot can have multiple texture
   - frames, of which currently only the first is used additional frames can be
   - used for various shaders
   -------------------------------------------------------------------------*/
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

  static void purgetextures(void) {loopi(256)loop(j,MAXFRAMES)mapping[i][j]=0;}

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

  static void texturereset(void) { curtexnum = 0; }
  COMMAND(texturereset, ARG_NONE);

  static void texture(char *aframe, char *name)
  {
    int num = curtexnum++, frame = atoi(aframe);
    if (num<0 || num>=256 || frame<0 || frame>=MAXFRAMES) return;
    mapping[num][frame] = 1;
    char *n = mapname[num][frame];
    strcpy_s(n, name);
    path(n);
  }
  COMMAND(texture, ARG_2STR);

  /*--------------------------------------------------------------------------
   - sphere management
   -------------------------------------------------------------------------*/
  static GLuint spherevbo = 0;
  static int spherevertn = 0;

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
    ogl::bindbuffer(ARRAY_BUFFER, spherevbo);
    OGL(BufferData, GL_ARRAY_BUFFER, sz, &v[0][0], GL_STATIC_DRAW);
  }

  /*--------------------------------------------------------------------------
   - overbright -> just multiply final color
   -------------------------------------------------------------------------*/
  static float overbrightf = 1.f;
  static void overbright(float amount)
  {
    dirty.flags.overbright=1;
    overbrightf = amount;
  }

  /*--------------------------------------------------------------------------
   - quick and dirty shader management
   -------------------------------------------------------------------------*/
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
    "in vec4 incol;\n"
    "#if USE_DIFFUSETEX\n"
    "  out vec2 texcoord;\n"
    "  in vec2 t;\n"
    "#endif\n"
    "out vec4 outcol;\n"
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
    "in vec4 outcol;\n"
    "out vec4 c;\n"
    "void main() {\n"
    "  vec4 col;\n"
    "#if USE_DIFFUSETEX\n"
    "  col = texture(diffuse, texcoord);\n"
    "  col *= outcol;\n"
    "#else\n"
    "  col = outcol;\n"
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

  static const char watervert[] = { /* use DIFFUSETEX */
    "#define PI 3.14159265\n"
    "uniform mat4 MVP;\n"
    "uniform vec2 duv, dxy;\n"
    "uniform float hf, delta;\n"
    "in vec2 p, t;\n"
    "in vec4 incol;\n"
    "out vec2 texcoord;\n"
    "out vec4 outcol;\n"
    "float dx(float x) { return x+sin(x*2.0+delta/1000.0)*0.04; }\n"
    "float dy(float x) { return x+sin(x*2.0+delta/900.0+PI/5.0)*0.05; }\n"
    "void main() {\n"
    "  texcoord = vec2(dx(t.x+duv.x),dy(t.y+duv.y));\n"
    "  vec2 absp = dxy+p;\n"
    "  vec3 pos = vec3(absp.x,hf-sin(absp.x*absp.y*0.1+delta/300.0)*0.2,absp.y);\n"
    "  outcol = incol;\n"
    "  gl_Position = MVP * vec4(pos,1.0);\n"
    "}\n"
  };
  static struct watershader : shader {
    GLuint uduv, udxy, uhf;
  } watershader;

  static shader *bindedshader = NULL;
  static vec4f fogcolor;
  static vec2f fogstartend;

  static void bindshader(shader &shader)
  {
    if (bindedshader != &shader) {
      bindedshader = &shader;
      dirty.any = ~0x0;
      glUseProgram(bindedshader->program);
    }
  }

  void bindshader(uint flags) { bindshader(shaders[flags]); }

  static void buildshaderattrib(shader &shader)
  {
    if (shader.rules&KEYFRAME) {
      OGL(BindAttribLocation, shader.program, POS0, "p0");
      OGL(BindAttribLocation, shader.program, POS1, "p1");
    } else
      OGL(BindAttribLocation, shader.program, POS0, "p");
    if (shader.rules&DIFFUSETEX)
      OGL(BindAttribLocation, shader.program, TEX, "t");
    OGL(BindAttribLocation, shader.program, COL, "incol");
    OGL(BindFragDataLocation, shader.program, 0, "c");
    OGL(LinkProgram, shader.program);
    OGL(ValidateProgram, shader.program);
    OGL(UseProgram, shader.program);
    OGLR(shader.uoverbright, GetUniformLocation, shader.program, "overbright");
    OGLR(shader.umvp, GetUniformLocation, shader.program, "MVP");
    if (shader.rules&KEYFRAME)
      OGLR(shader.udelta, GetUniformLocation, shader.program, "delta");
    else
      shader.udelta = 0;
    if (shader.rules&DIFFUSETEX) {
      OGLR(shader.udiffuse, GetUniformLocation, shader.program, "diffuse");
      OGL(Uniform1i, shader.udiffuse, 0);
    }
    if (shader.rules&FOG) {
      OGLR(shader.uzaxis, GetUniformLocation, shader.program, "zaxis");
      OGLR(shader.ufogcolor, GetUniformLocation, shader.program, "fogcolor");
      OGLR(shader.ufogstartend, GetUniformLocation, shader.program, "fogstartend");
    }
    OGL(UseProgram, 0);
  }

  static void buildshader(shader &shader, const char *vertsrc, const char *fragsrc, uint rules)
  {
    memset(&shader, 0, sizeof(struct shader));
    shader.program = loadprogram(vertsrc, fragsrc, rules);
    shader.rules = rules;
    buildshaderattrib(shader);
  }

  static void buildubershader(shader &shader, uint rules)
  {
    buildshader(shader, ubervert, uberfrag, rules);
  }

  /*--------------------------------------------------------------------------
   - world vbo is just a big vbo that contains all the triangles to render
   -------------------------------------------------------------------------*/
  static GLuint worldvbo = 0u;
  static void bindworldvbo(void)
  {
    if (worldvbo == 0u) OGL(GenBuffers, 1, &worldvbo);
    OGL(BindBuffer, GL_ARRAY_BUFFER, worldvbo);
  }

  /* display the binded md2 model */
  void rendermd2(const float *pos0, const float *pos1, float lerp, int n)
  {
    OGL(VertexAttribPointer, TEX, 2, GL_FLOAT, 0, sizeof(float[5]), pos0);
    OGL(VertexAttribPointer, POS0, 3, GL_FLOAT, 0, sizeof(float[5]), pos0+2);
    OGL(VertexAttribPointer, POS1, 3, GL_FLOAT, 0, sizeof(float[5]), pos1+2);
    OGL(EnableVertexAttribArray, POS0);
    OGL(EnableVertexAttribArray, POS1);
    OGL(EnableVertexAttribArray, TEX);
    bindshader(FOG|KEYFRAME|DIFFUSETEX);
    OGL(Uniform1f, bindedshader->udelta, lerp);
    drawarrays(GL_TRIANGLES, 0, n);
    OGL(DisableVertexAttribArray, POS0);
    OGL(DisableVertexAttribArray, POS1);
    OGL(DisableVertexAttribArray, TEX);
  }

  /* flush all the states required for the draw call */
  static void flush(void)
  {
    if (dirty.any == 0) return; /* fast path */
    if (dirty.flags.shader) {
      OGL(UseProgram, bindedshader->program);
      dirty.flags.shader = 0;
    }
    if ((bindedshader->rules & FOG) && (dirty.flags.fog || dirty.flags.mvp)) {
      const mat4x4f &mv = vp[MODELVIEW];
      const vec4f zaxis(mv.vx.z,mv.vy.z,mv.vz.z,mv.vw.z);
      OGL(Uniform2fv, bindedshader->ufogstartend, 1, &fogstartend.x);
      OGL(Uniform4fv, bindedshader->ufogcolor, 1, &fogcolor.x);
      OGL(Uniform4fv, bindedshader->uzaxis, 1, &zaxis.x);
      dirty.flags.fog = 0;
    }
    if (dirty.flags.mvp) {
      viewproj = vp[PROJECTION]*vp[MODELVIEW];
      OGL(UniformMatrix4fv, bindedshader->umvp, 1, GL_FALSE, &viewproj.vx.x);
      dirty.flags.mvp = 0;
    }
    if (dirty.flags.overbright) {
      OGL(Uniform1f, bindedshader->uoverbright, overbrightf);
      dirty.flags.overbright = 0;
    }
  }

  void drawarrays(int mode, int first, int count) {
    flush();
    OGL(DrawArrays, mode, first, count);
  }
  void drawelements(int mode, int count, int type, const void *indices) {
    flush();
    OGL(DrawElements, mode, count, type, indices);
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
    dirty.any = ~0x0;
    purgetextures();
    buildsphere(1, 12, 6);
    loopi(shadern) buildubershader(shaders[i], i);
    buildshader(watershader, watervert, uberfrag, DIFFUSETEX);
    OGLR(watershader.udelta, GetUniformLocation, watershader.program, "delta");
    OGLR(watershader.uduv, GetUniformLocation, watershader.program, "duv");
    OGLR(watershader.udxy, GetUniformLocation, watershader.program, "dxy");
    OGLR(watershader.uhf, GetUniformLocation, watershader.program, "hf");
    bigbufferinit(bigbuffersize);
  }

  void clean(void) { OGL(DeleteBuffers, 1, &spherevbo); }

  void drawsphere(void)
  {
    ogl::bindbuffer(ARRAY_BUFFER, spherevbo);
    ogl::bindshader(DIFFUSETEX);
    draw(GL_TRIANGLE_STRIP, 3, 2, spherevertn, NULL);
  }

  static void setupworld(void)
  {
    OGL(EnableVertexAttribArray, POS0);
    OGL(EnableVertexAttribArray, COL);
    OGL(EnableVertexAttribArray, TEX);
    rdr::setarraypointers();
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
    rdr::rendermodel(hudgunnames[player1->gunselect], start, end, 0, 1.0f, player1->o.x, player1->o.z, player1->o.y, player1->yaw+90, player1->pitch, false, 1.0f, speed, 0, base);
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

  /* XXX remove state crap */
  void draw(int mode, int pos, int tex, size_t n, const float *data)
  {
    if (tex) {
      OGL(EnableVertexAttribArray, TEX);
      OGL(VertexAttribPointer, TEX, tex, GL_FLOAT, 0, (pos+tex)*sizeof(float), data);
    }
    OGL(EnableVertexAttribArray, POS0);
    OGL(VertexAttribPointer, POS0, pos, GL_FLOAT, 0, (pos+tex)*sizeof(float), data+tex);
    ogl::drawarrays(mode, 0, n);

    OGL(DisableVertexAttribArray, TEX);
    OGL(DisableVertexAttribArray, COL);
    OGL(DisableVertexAttribArray, POS0);
  }

  VAR(renderparticles,0,1,1);

  void drawframe(int w, int h, float curfps)
  {
    const float hf = hdr.waterlevel-0.3f;
    const bool underwater = player1->o.z<hf;
    float fovy = (float)fov*h/w;
    float aspect = w/(float)h;

    fogstartend.x = float((fog+64)/8);
    fogstartend.y = 1.f/(float(fog)-fogstartend[0]);
    fogcolor.x = float(fogcolour>>16)/256.0f;
    fogcolor.y = float((fogcolour>>8)&255)/256.0f;
    fogcolor.z = float(fogcolour&255)/256.0f;
    fogcolor.w = 1.f;
    OGL(ClearColor, fogcolor.x, fogcolor.y, fogcolor.z, fogcolor.w);
    if (underwater) {
      fovy += (float)sin(lastmillis/1000.0)*2.0f;
      aspect += (float)sin(lastmillis/1000.0+PI)*0.1f;
      fogstartend.x = 0.f;
      fogstartend.y = 1.f/float((fog+96)/8);
    }
    dirty.flags.fog = 1;

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

    rdr::resetcubes();

    rdr::curvert = 0;
    strips.setsize(0);

    world::render(player1->o.x, player1->o.y, player1->o.z,
                  (int)player1->yaw, (int)player1->pitch, (float)fov, w, h);
    rdr::finishstrips();
    bindworldvbo();
    rdr::uploadworld();

    /* render sky */
    setupworld();
    bindshader(COLOR_ONLY);
    renderstripssky();
    OGL(DisableVertexAttribArray, COL);
    identity();
    rotate(player1->pitch, pitch);
    rotate(player1->yaw, yaw);
    rotate(90.f, vec3f(1.f,0.f,0.f));
    OGL(VertexAttrib3f,COL,1.0f,1.0f,1.0f);
    OGL(DepthFunc, GL_GREATER);
    bindshader(DIFFUSETEX);
    rdr::draw_envbox(14, fog*4/3);
    OGL(DepthFunc, GL_LESS);

    /* render diffuse objects */
    transplayer();
    overbright(2.f);
    bindworldvbo();
    setupworld(); /* XXX REMOVE ! */
    bindshader(DIFFUSETEX|FOG);
    renderstrips();
    OGL(DisableVertexAttribArray, COL); /* XXX put it elsewhere */

    ogl::xtraverts = 0;

    game::renderclients();
    monster::monsterrender();
    entities::renderentities();
    rdr::renderspheres(curtime);
    rdr::renderents();

    OGL(Disable, GL_CULL_FACE);

    drawhudgun(fovy, aspect, farplane);

    overbright(1.f);
    bindshader(watershader);
    OGL(Uniform1f, watershader.udelta, float(lastmillis));
    OGL(Uniform1f, watershader.uhf, hf);
    OGL(EnableVertexAttribArray, POS0); /* XXX REMOVE! */
    OGL(EnableVertexAttribArray, COL);
    OGL(EnableVertexAttribArray, TEX);
    const int nquads = rdr::renderwater(hf, watershader.udxy, watershader.uduv);
    OGL(DisableVertexAttribArray, POS0); /* XXX REMOVE! */
    OGL(DisableVertexAttribArray, COL);
    OGL(DisableVertexAttribArray, TEX);

    if (renderparticles) {
      overbright(2.f);
      bindshader(DIFFUSETEX);
      rdr::render_particles(curtime);
    }

    overbright(1.f);
    OGL(Disable, GL_TEXTURE_2D);
    rdr::drawhud(w, h, int(curfps), nquads, rdr::curvert, underwater);
    OGL(Enable, GL_CULL_FACE);
  }
} /* namespace ogl */

