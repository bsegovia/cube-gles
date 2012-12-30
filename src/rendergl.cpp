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

  /* matrix handling. very inspired by opengl 1.0 :-) */
  enum {MATRIX_STACK = 16};
  static mat4x4f vp[MATRIX_MODE];
  static mat4x4f vpstack[MATRIX_STACK][MATRIX_MODE];
  static int vpdepth = 0;
  static int vpmode = MODELVIEW;

  static const int glmode[2]={GL_MODELVIEW,GL_PROJECTION}; /* XXX remove that */
  static const int glmatrix[2]={GL_MODELVIEW_MATRIX,GL_PROJECTION_MATRIX};

  void matrixmode(int mode)
  {
    glMatrixMode(glmode[mode]);
    vpmode = mode;
  }
  void loadmatrix(const mat4x4f &m)
  {
    glLoadMatrixf(&m[0][0]);
    vp[vpmode] = m;
  }
  const mat4x4f &matrix(int mode) { return vp[mode]; }
  void identity(void)
  {
    glLoadIdentity();
    vp[vpmode] = mat4x4f(one);
  }
  void translate(const vec3f &v)
  {
    glTranslatef(v.x,v.y,v.z);
    vp[vpmode] = translate(vp[vpmode],v);
  }
  void mulmatrix(const mat4x4f &m)
  {
    glMultMatrixf(&m[0][0]);
    vp[vpmode] = m*vp[vpmode];
  }
  void rotate(float angle, const vec3f &axis)
  {
    glRotatef(angle, axis.x, axis.y, axis.z);
    vp[vpmode] = rotate(vp[vpmode],angle,axis);
  }
  void perspective(float fovy, float aspect, float znear, float zfar)
  {
    double p[16];
    ::perspective(p, fovy, aspect, znear, zfar);
    glMultMatrixd(p);
    vp[vpmode] = ::perspective(fovy,aspect,znear,zfar)*vp[vpmode];
  }

  void ortho(float left, float right, float bottom, float top, float znear, float zfar) {
    vp[vpmode] = ::ortho(left,right,bottom,top,znear,zfar);
    glOrtho(left,right,bottom,top,znear,zfar);
  }
  void scale(const vec3f &s)
  {
    glScalef(s.x,s.y,s.z);
    scale(vp[vpmode],s);
  }

  void pushmatrix(void) {
    assert(vpdepth+1<MATRIX_STACK);
    glPushMatrix();
    vpstack[vpdepth++][vpmode] = vp[vpmode];
  }
  void popmatrix(void) {
    assert(vpdepth>0);
    glPopMatrix();
    vp[vpmode] = vpstack[--vpdepth][vpmode];
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
      /* const float sin3 = -sinf(angle0); */
      /* const float cos3 = -cosf(angle0); */
      /* const float sin4 = -sinf(angle1); */
      /* const float cos4 = -cosf(angle1); */

      loopi(slices+1) {
        const float angle = 2.f * M_PI * float(i) / float(slices);
        const float sin0 = sinf(angle);
        const float cos0 = cosf(angle);
        const int start = (i==0&&j!=0)?2:1;
        const int end = (i==slices&&j!=stacks-1)?2:1;
        loopk(start) { /* stick the strips together */
          const float s = 1.f-float(i)/slices, t = 1.f-float(j)/stacks;
          const float x = sin1*sin0, y = sin1*cos0, z = zLow;
          /* glNormal3f(sin0*sin3, cos0*sin3, cos3); */
          v.add(vvec<5>(s, t, x, y, z));
        }

        loopk(end) { /* idem */
          const float s = 1.f-float(i)/slices, t = 1.f-float(j+1)/stacks;
          const float x = sin2*sin0, y = sin2*cos0, z = zHigh;
          /* glNormal3f(sin0*sin4, cos0*sin4, cos4); */
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

  static bool checkshader(GLuint shaderName, const char *source)
  {
    GLint result = GL_FALSE;
    int infoLogLength;

    if (!shaderName) return false;
    OGL(GetShaderiv, shaderName, GL_COMPILE_STATUS, &result);
    OGL(GetShaderiv, shaderName, GL_INFO_LOG_LENGTH, &infoLogLength);
    if (infoLogLength) {
      char *buffer = new char[infoLogLength + 1];
      buffer[infoLogLength] = 0;
      OGL(GetShaderInfoLog, shaderName, infoLogLength, NULL, buffer);
      printf(buffer);
      delete [] buffer;
    }
    if (result == GL_FALSE) fatal("OGL: failed to compile shader");
    return result == GL_TRUE;
  }

  static GLuint loadshader(GLenum type, const char *source)
  {
    const GLuint name = glCreateShader(type);
    OGL(ShaderSource, name, 1, &source, NULL);
    OGL(CompileShader, name);
    if (!checkshader(name, source)) fatal("OGL: shader not valid");
    return name;
  }

  static GLuint loadprogram(const char *vertstr, const char *fragstr)
  {
    GLuint program = 0;
    const GLuint vert = loadshader(GL_VERTEX_SHADER, vertstr);
    const GLuint frag = loadshader(GL_FRAGMENT_SHADER, fragstr);
    OGLR(program, CreateProgram);
    OGL(AttachShader, program, vert);
    OGL(AttachShader, program, frag);
    OGL(DeleteShader, vert);
    OGL(DeleteShader, frag);
    return program;
  }

  /* for diffuse models */
  static const char diffusevert[] = {
    "#version 130\n"
    "uniform mat4 MVP;\n"
    "uniform vec3 zaxis;\n"
    "in vec3 p,incol;\n"
    "in vec2 t;\n"
    "out vec2 texcoord;\n"
    "out vec3 outcol;\n"
    "out float fogz;\n"
    "void main() {\n"
    "  texcoord = t;\n"
    "  fogz = dot(zaxis,p);\n"
    "  outcol = incol;\n"
    "  gl_Position = MVP * vec4(p,1.0);\n"
    "}\n"
  };
  /* mix(texture, fog) * color */
  static const char diffusefrag[] = {
    "#version 130\n"
    "uniform sampler2D diffuse;\n"
    "uniform vec4 fogcolor;\n"
    "uniform vec2 fogstartend;\n"
    "uniform float overbright;\n"
    "in vec2 texcoord;\n"
    "in vec3 outcol;\n"
    "in float fogz;\n"
    "out vec4 c;\n"
    "void main() {\n"
    "  const float factor = clamp((-fogz-fogstartend.x)*fogstartend.y.x,0.0,1.0)\n;"
    "  const vec4 tex = texture(diffuse, texcoord);\n"
    "  const vec3 col = mix(tex.xyz,fogcolor.xyz,factor);\n"
    "  c = vec4(col*overbright*outcol,tex.w);\n"
    "}\n"
  };

  /* for particles */
  static const char particlevert[] = {
    "#version 130\n"
    "uniform mat4 MVP;\n"
    "in vec3 p,incol;\n"
    "in vec2 t;\n"
    "out vec2 texcoord;\n"
    "out vec4 outcol;\n"
    "void main() {\n"
    "  texcoord = t;\n"
    "  outcol = vec4(incol,1.0);\n"
    "  gl_Position = MVP * vec4(p,1.0);\n"
    "}\n"
  };
  /* texture * color (for particles) */
  static const char particlefrag[] = {
    "#version 130\n"
    "uniform sampler2D diffuse;\n"
    "uniform float overbright;\n"
    "in vec2 texcoord;\n"
    "in vec4 outcol;\n"
    "out vec4 c;\n"
    "void main() {c = overbright*outcol*texture(diffuse, texcoord);}\n"
  };

  /* for md2 models with key frame interpolation */
  static const char md2vert[] = {
    "#version 130\n"
    "uniform mat4 MVP;\n"
    "uniform float delta;\n"
    "uniform vec3 zaxis;\n"
    "in vec3 p0,p1,incol;\n"
    "in vec2 t;\n"
    "out vec2 texcoord;\n"
    "out vec3 outcol;\n"
    "out float fogz;\n"
    "void main() {\n"
    "  texcoord = t;\n"
    "  const vec4 p = vec4(mix(p0,p1,delta),1.0);\n"
    "  fogz = dot(zaxis,p.xyz);\n"
    "  outcol = incol;\n"
    "  gl_Position = MVP * p;\n"
    "}\n"
  };
  static struct shader {
    GLuint program;
    GLuint udiffuse, udelta, umvp, uzaxis, ufogstartend, ufogcolor, uoverbright;
  } md2, diffuse, particle;

  static float fogcolor[4];
  static float fogstartend[2];

  static void bindshader(shader shader, bool keyframe, bool fog, float lerp)
  {
#if 0
    mat4x4f view, proj, viewproj; /* XXX DO IT ELSEWHERE ONLY ONCE! */
    OGL(GetFloatv, GL_PROJECTION_MATRIX, &proj.vx.x);
    OGL(GetFloatv, GL_MODELVIEW_MATRIX, &view.vx.x);
    viewproj = proj*view;
#else
    //const mat4x4f viewproj = vp[PROJECTION]*vp[MODELVIEW];
    const mat4x4f viewproj = vp[PROJECTION]*vp[MODELVIEW];
#endif
    OGL(UseProgram, shader.program);
    if (fog) {
      //const vec3f zaxis(view.vx.z,view.vy.z,view.vz.z);
      const vec3f zaxis(vp[MODELVIEW].vx.z,vp[MODELVIEW].vy.z,vp[MODELVIEW].vz.z);
      OGL(Uniform2fv, shader.ufogstartend, 1, fogstartend);
      OGL(Uniform4fv, shader.ufogcolor, 1, fogcolor);
      OGL(Uniform3fv, shader.uzaxis, 1, &zaxis.x);
    }
    if (keyframe) OGL(Uniform1f, shader.udelta, lerp);
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
    bindshader(md2, true, true, lerp);
    OGL(DisableClientState, GL_COLOR_ARRAY);
    OGL(DrawArrays, GL_TRIANGLES, 0, n);
    unbindshader();
    OGL(DisableVertexAttribArray, POS0);
    OGL(DisableVertexAttribArray, POS1);
    OGL(DisableVertexAttribArray, TEX);
    OGL(EnableClientState, GL_COLOR_ARRAY);
    OGL(EnableClientState, GL_VERTEX_ARRAY);
    OGL(EnableClientState, GL_TEXTURE_COORD_ARRAY);
  }

  static void buildshader(shader &shader, bool keyframe, bool fog)
  {
    if (keyframe) {
      OGL(BindAttribLocation, shader.program, POS0, "p0");
      OGL(BindAttribLocation, shader.program, POS1, "p1");
    } else
      OGL(BindAttribLocation, shader.program, POS0, "p");
    OGL(BindAttribLocation, shader.program, TEX, "t");
    OGL(BindAttribLocation, shader.program, COL, "incol");
    OGL(BindFragDataLocation, shader.program, 0, "c");
    OGL(LinkProgram, shader.program);
    OGL(ValidateProgram, shader.program);
    OGL(UseProgram, shader.program);
    OGLR(shader.uoverbright, GetUniformLocation, shader.program, "overbright");
    OGLR(shader.umvp, GetUniformLocation, shader.program, "MVP");
    if (keyframe)
      OGLR(shader.udelta, GetUniformLocation, shader.program, "delta");
    else
      shader.udelta = 0;
    OGLR(shader.udiffuse, GetUniformLocation, shader.program, "diffuse");
    if (fog) {
      OGLR(shader.uzaxis, GetUniformLocation, shader.program, "zaxis");
      OGLR(shader.ufogcolor, GetUniformLocation, shader.program, "fogcolor");
      OGLR(shader.ufogstartend, GetUniformLocation, shader.program, "fogstartend");
    }
    OGL(Uniform1i, shader.udiffuse, 0);
    OGL(UseProgram, 0);
  }

  void init(int w, int h)
  {
    OGL(Viewport, 0, 0, w, h);
    glClearDepth(1.f);
    OGL(DepthFunc, GL_LESS);
    OGL(Enable, GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);

    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_DENSITY, 0.25f);
    glHint(GL_FOG_HINT, GL_NICEST);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    OGL(CullFace, GL_FRONT);
    OGL(Enable, GL_CULL_FACE);

    const char *exts = (const char *)glGetString(GL_EXTENSIONS);

    if (strstr(exts, "GL_EXT_texture_env_combine"))
      hasoverbright = true;
    else
      console::out("WARNING: cannot use overbright lighting");

    OGL(GetIntegerv, GL_MAX_TEXTURE_SIZE, &glmaxtexsize);

    purgetextures();
    buildsphere(1, 12, 6);
    md2.program = loadprogram(md2vert, diffusefrag);
    buildshader(md2, true, true);

    diffuse.program = loadprogram(diffusevert, diffusefrag);
    buildshader(diffuse, false, true);

    particle.program = loadprogram(particlevert, particlefrag);
    buildshader(particle, false, false);
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
    OGL(BindTexture, GL_TEXTURE_2D, skyoglid);
    loopv(strips)
      if (strips[i].tex==skyoglid)
        glDrawArrays(GL_TRIANGLE_STRIP, strips[i].start, strips[i].num);
  }

  static void renderstrips(void)
  {
    int lasttex = -1;
    loopv(strips) if (strips[i].tex!=skyoglid) {
      if (strips[i].tex!=lasttex) {
        OGL(BindTexture, GL_TEXTURE_2D, strips[i].tex);
        lasttex = strips[i].tex;
      }
      glDrawArrays(GL_TRIANGLE_STRIP, strips[i].start, strips[i].num);
    }
  }

  static void overbright(float amount)
  {
    if (hasoverbright)
      glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, amount);
    overbrightf = amount;
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

    glFogi(GL_FOG_START, (fog+64)/8);
    glFogi(GL_FOG_END, fog);
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

    glFogfv(GL_FOG_COLOR, fogc);
    OGL(ClearColor, fogc[0], fogc[1], fogc[2], 1.0f);

    if (underwater) {
      fovy += (float)sin(lastmillis/1000.0)*2.0f;
      aspect += (float)sin(lastmillis/1000.0+PI)*0.1f;
      glFogi(GL_FOG_START, 0);
      glFogi(GL_FOG_END, (fog+96)/8);
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

    renderstripssky();

    identity();
    rotate(player1->pitch, pitch);
    rotate(player1->yaw, yaw);
    rotate(90.f, vec3f(1.f,0.f,0.f));
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

    OGL(Disable, GL_CULL_FACE);

    drawhudgun(fovy, aspect, farplane);

    overbright(1);
    const int nquads = renderwater(hf);

    overbright(2);
    bindshader(particle,false,false,0.f);
    render_particles(curtime);
    unbindshader();
    overbright(1);

    glDisable(GL_FOG);

    OGL(Disable, GL_TEXTURE_2D);

    drawhud(w, h, (int)curfps, nquads, curvert, underwater);

    OGL(Enable, GL_CULL_FACE);
    glEnable(GL_FOG);
  }
} /* namespace ogl */
} /* namespace rdr */

