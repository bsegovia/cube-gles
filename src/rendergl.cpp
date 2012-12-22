#include "cube.h"
#include <GL/gl.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

// XXX
int xtraverts;
bool hasoverbright = false;

namespace renderer
{
#ifdef DARWIN
#define GL_COMBINE_EXT GL_COMBINE_ARB
#define GL_COMBINE_RGB_EXT GL_COMBINE_RGB_ARB
#define GL_SOURCE0_RBG_EXT GL_SOURCE0_RGB_ARB
#define GL_SOURCE1_RBG_EXT GL_SOURCE1_RGB_ARB
#define GL_RGB_SCALE_EXT GL_RGB_SCALE_ARB
#endif

  extern int curvert;

  void purgetextures();

  int glmaxtexsize = 256;

  struct quadric {
    int	normals;
    int	textureCoords;
    int	orientation;
    int	drawStyle;
  };
#define CACHE_SIZE	240
#define GLU_POINT                          100010
#define GLU_LINE                           100011
#define GLU_FILL                           100012
#define GLU_SILHOUETTE                     100013
#define GLU_SMOOTH                         100000
#define GLU_FLAT                           100001
#define GLU_NONE                           100002
#define GLU_OUTSIDE                        100020
#define GLU_INSIDE                         100021

  void sphere(quadric *qobj, GLdouble radius, GLint slices, GLint stacks)
  {
    GLint i,j;
    GLfloat sinCache1a[CACHE_SIZE];
    GLfloat cosCache1a[CACHE_SIZE];
    GLfloat sinCache2a[CACHE_SIZE];
    GLfloat cosCache2a[CACHE_SIZE];
    GLfloat sinCache3a[CACHE_SIZE];
    GLfloat cosCache3a[CACHE_SIZE];
    GLfloat sinCache1b[CACHE_SIZE];
    GLfloat cosCache1b[CACHE_SIZE];
    GLfloat sinCache2b[CACHE_SIZE];
    GLfloat cosCache2b[CACHE_SIZE];
    GLfloat sinCache3b[CACHE_SIZE];
    GLfloat cosCache3b[CACHE_SIZE];
    GLfloat angle;
    GLfloat zLow, zHigh;
    GLfloat sintemp1 = 0.0, sintemp2 = 0.0, sintemp3 = 0.0, sintemp4 = 0.0;
    GLfloat costemp1 = 0.0, costemp2 = 0.0, costemp3 = 0.0, costemp4 = 0.0;
    GLboolean needCache2, needCache3;
    GLint start, finish;

    if (slices >= CACHE_SIZE) slices = CACHE_SIZE-1;
    if (stacks >= CACHE_SIZE) stacks = CACHE_SIZE-1;
    if (slices < 2 || stacks < 1 || radius < 0.0) return;

    /* Cache is the vertex locations cache */
    /* Cache2 is the various normals at the vertices themselves */
    /* Cache3 is the various normals for the faces */
    needCache2 = needCache3 = GL_FALSE;

    if (qobj->normals == GLU_SMOOTH) {
      needCache2 = GL_TRUE;
    }

    if (qobj->normals == GLU_FLAT) {
      if (qobj->drawStyle != GLU_POINT) {
        needCache3 = GL_TRUE;
      }
      if (qobj->drawStyle == GLU_LINE) {
        needCache2 = GL_TRUE;
      }
    }

    for (i = 0; i < slices; i++) {
      angle = 2 * PI * i / slices;
      sinCache1a[i] = sinf(angle);
      cosCache1a[i] = cosf(angle);
      if (needCache2) {
        sinCache2a[i] = sinCache1a[i];
        cosCache2a[i] = cosCache1a[i];
      }
    }

    for (j = 0; j <= stacks; j++) {
      angle = PI * j / stacks;
      if (needCache2) {
        if (qobj->orientation == GLU_OUTSIDE) {
          sinCache2b[j] = sinf(angle);
          cosCache2b[j] = cosf(angle);
        } else {
          sinCache2b[j] = -sinf(angle);
          cosCache2b[j] = -cosf(angle);
        }
      }
      sinCache1b[j] = radius * sinf(angle);
      cosCache1b[j] = radius * cosf(angle);
    }
    /* Make sure it comes to a point */
    sinCache1b[0] = 0;
    sinCache1b[stacks] = 0;

    if (needCache3) {
      for (i = 0; i < slices; i++) {
        angle = 2 * PI * (i-0.5) / slices;
        sinCache3a[i] = sinf(angle);
        cosCache3a[i] = cosf(angle);
      }
      for (j = 0; j <= stacks; j++) {
        angle = PI * (j - 0.5) / stacks;
        if (qobj->orientation == GLU_OUTSIDE) {
          sinCache3b[j] = sinf(angle);
          cosCache3b[j] = cosf(angle);
        } else {
          sinCache3b[j] = -sinf(angle);
          cosCache3b[j] = -cosf(angle);
        }
      }
    }

    sinCache1a[slices] = sinCache1a[0];
    cosCache1a[slices] = cosCache1a[0];
    if (needCache2) {
      sinCache2a[slices] = sinCache2a[0];
      cosCache2a[slices] = cosCache2a[0];
    }
    if (needCache3) {
      sinCache3a[slices] = sinCache3a[0];
      cosCache3a[slices] = cosCache3a[0];
    }

    switch (qobj->drawStyle) {
      case GLU_FILL:
        /* Do ends of sphere as TRIANGLE_FAN's (if not texturing)
         ** We don't do it when texturing because we need to respecify the
         ** texture coordinates of the apex for every adjacent vertex (because
         ** it isn't a constant for that point)
         */
        if (!(qobj->textureCoords)) {
          start = 1;
          finish = stacks - 1;

          /* Low end first (j == 0 iteration) */
          sintemp2 = sinCache1b[1];
          zHigh = cosCache1b[1];
          switch(qobj->normals) {
            case GLU_FLAT:
              sintemp3 = sinCache3b[1];
              costemp3 = cosCache3b[1];
              break;
            case GLU_SMOOTH:
              sintemp3 = sinCache2b[1];
              costemp3 = cosCache2b[1];
              glNormal3f(sinCache2a[0] * sinCache2b[0],
                  cosCache2a[0] * sinCache2b[0],
                  cosCache2b[0]);
              break;
            default:
              break;
          }
          glBegin(GL_TRIANGLE_FAN);
          glVertex3f(0.0, 0.0, radius);
          if (qobj->orientation == GLU_OUTSIDE) {
            for (i = slices; i >= 0; i--) {
              switch(qobj->normals) {
                case GLU_SMOOTH:
                  glNormal3f(sinCache2a[i] * sintemp3,
                      cosCache2a[i] * sintemp3,
                      costemp3);
                  break;
                case GLU_FLAT:
                  if (i != slices) {
                    glNormal3f(sinCache3a[i+1] * sintemp3,
                        cosCache3a[i+1] * sintemp3,
                        costemp3);
                  }
                  break;
                case GLU_NONE:
                default:
                  break;
              }
              glVertex3f(sintemp2 * sinCache1a[i],
                  sintemp2 * cosCache1a[i], zHigh);
            }
          } else {
            for (i = 0; i <= slices; i++) {
              switch(qobj->normals) {
                case GLU_SMOOTH:
                  glNormal3f(sinCache2a[i] * sintemp3,
                      cosCache2a[i] * sintemp3,
                      costemp3);
                  break;
                case GLU_FLAT:
                  glNormal3f(sinCache3a[i] * sintemp3,
                      cosCache3a[i] * sintemp3,
                      costemp3);
                  break;
                case GLU_NONE:
                default:
                  break;
              }
              glVertex3f(sintemp2 * sinCache1a[i],
                  sintemp2 * cosCache1a[i], zHigh);
            }
          }
          glEnd();

          /* High end next (j == stacks-1 iteration) */
          sintemp2 = sinCache1b[stacks-1];
          zHigh = cosCache1b[stacks-1];
          switch(qobj->normals) {
            case GLU_FLAT:
              sintemp3 = sinCache3b[stacks];
              costemp3 = cosCache3b[stacks];
              break;
            case GLU_SMOOTH:
              sintemp3 = sinCache2b[stacks-1];
              costemp3 = cosCache2b[stacks-1];
              glNormal3f(sinCache2a[stacks] * sinCache2b[stacks],
                  cosCache2a[stacks] * sinCache2b[stacks],
                  cosCache2b[stacks]);
              break;
            default:
              break;
          }
          glBegin(GL_TRIANGLE_FAN);
          glVertex3f(0.0, 0.0, -radius);
          if (qobj->orientation == GLU_OUTSIDE) {
            for (i = 0; i <= slices; i++) {
              switch(qobj->normals) {
                case GLU_SMOOTH:
                  glNormal3f(sinCache2a[i] * sintemp3,
                      cosCache2a[i] * sintemp3,
                      costemp3);
                  break;
                case GLU_FLAT:
                  glNormal3f(sinCache3a[i] * sintemp3,
                      cosCache3a[i] * sintemp3,
                      costemp3);
                  break;
                case GLU_NONE:
                default:
                  break;
              }
              glVertex3f(sintemp2 * sinCache1a[i],
                  sintemp2 * cosCache1a[i], zHigh);
            }
          } else {
            for (i = slices; i >= 0; i--) {
              switch(qobj->normals) {
                case GLU_SMOOTH:
                  glNormal3f(sinCache2a[i] * sintemp3,
                      cosCache2a[i] * sintemp3,
                      costemp3);
                  break;
                case GLU_FLAT:
                  if (i != slices) {
                    glNormal3f(sinCache3a[i+1] * sintemp3,
                        cosCache3a[i+1] * sintemp3,
                        costemp3);
                  }
                  break;
                case GLU_NONE:
                default:
                  break;
              }
              glVertex3f(sintemp2 * sinCache1a[i],
                  sintemp2 * cosCache1a[i], zHigh);
            }
          }
          glEnd();
        } else {
          start = 0;
          finish = stacks;
        }
        for (j = start; j < finish; j++) {
          zLow = cosCache1b[j];
          zHigh = cosCache1b[j+1];
          sintemp1 = sinCache1b[j];
          sintemp2 = sinCache1b[j+1];
          switch(qobj->normals) {
            case GLU_FLAT:
              sintemp4 = sinCache3b[j+1];
              costemp4 = cosCache3b[j+1];
              break;
            case GLU_SMOOTH:
              if (qobj->orientation == GLU_OUTSIDE) {
                sintemp3 = sinCache2b[j+1];
                costemp3 = cosCache2b[j+1];
                sintemp4 = sinCache2b[j];
                costemp4 = cosCache2b[j];
              } else {
                sintemp3 = sinCache2b[j];
                costemp3 = cosCache2b[j];
                sintemp4 = sinCache2b[j+1];
                costemp4 = cosCache2b[j+1];
              }
              break;
            default:
              break;
          }

          glBegin(GL_QUAD_STRIP);
          for (i = 0; i <= slices; i++) {
            switch(qobj->normals) {
              case GLU_SMOOTH:
                glNormal3f(sinCache2a[i] * sintemp3,
                    cosCache2a[i] * sintemp3,
                    costemp3);
                break;
              case GLU_FLAT:
              case GLU_NONE:
              default:
                break;
            }
            if (qobj->orientation == GLU_OUTSIDE) {
              if (qobj->textureCoords) {
                glTexCoord2f(1 - (float) i / slices,
                    1 - (float) (j+1) / stacks);
              }
              glVertex3f(sintemp2 * sinCache1a[i],
                  sintemp2 * cosCache1a[i], zHigh);
            } else {
              if (qobj->textureCoords) {
                glTexCoord2f(1 - (float) i / slices,
                    1 - (float) j / stacks);
              }
              glVertex3f(sintemp1 * sinCache1a[i],
                  sintemp1 * cosCache1a[i], zLow);
            }
            switch(qobj->normals) {
              case GLU_SMOOTH:
                glNormal3f(sinCache2a[i] * sintemp4,
                    cosCache2a[i] * sintemp4,
                    costemp4);
                break;
              case GLU_FLAT:
                glNormal3f(sinCache3a[i] * sintemp4,
                    cosCache3a[i] * sintemp4,
                    costemp4);
                break;
              case GLU_NONE:
              default:
                break;
            }
            if (qobj->orientation == GLU_OUTSIDE) {
              if (qobj->textureCoords) {
                glTexCoord2f(1 - (float) i / slices,
                    1 - (float) j / stacks);
              }
              glVertex3f(sintemp1 * sinCache1a[i],
                  sintemp1 * cosCache1a[i], zLow);
            } else {
              if (qobj->textureCoords) {
                glTexCoord2f(1 - (float) i / slices,
                    1 - (float) (j+1) / stacks);
              }
              glVertex3f(sintemp2 * sinCache1a[i],
                  sintemp2 * cosCache1a[i], zHigh);
            }
          }
          glEnd();
        }
        break;
      case GLU_POINT:
        glBegin(GL_POINTS);
        for (j = 0; j <= stacks; j++) {
          sintemp1 = sinCache1b[j];
          costemp1 = cosCache1b[j];
          switch(qobj->normals) {
            case GLU_FLAT:
            case GLU_SMOOTH:
              sintemp2 = sinCache2b[j];
              costemp2 = cosCache2b[j];
              break;
            default:
              break;
          }
          for (i = 0; i < slices; i++) {
            switch(qobj->normals) {
              case GLU_FLAT:
              case GLU_SMOOTH:
                glNormal3f(sinCache2a[i] * sintemp2,
                    cosCache2a[i] * sintemp2,
                    costemp2);
                break;
              case GLU_NONE:
              default:
                break;
            }

            zLow = j * radius / stacks;

            if (qobj->textureCoords) {
              glTexCoord2f(1 - (float) i / slices,
                  1 - (float) j / stacks);
            }
            glVertex3f(sintemp1 * sinCache1a[i],
                sintemp1 * cosCache1a[i], costemp1);
          }
        }
        glEnd();
        break;
      case GLU_LINE:
      case GLU_SILHOUETTE:
        for (j = 1; j < stacks; j++) {
          sintemp1 = sinCache1b[j];
          costemp1 = cosCache1b[j];
          switch(qobj->normals) {
            case GLU_FLAT:
            case GLU_SMOOTH:
              sintemp2 = sinCache2b[j];
              costemp2 = cosCache2b[j];
              break;
            default:
              break;
          }

          glBegin(GL_LINE_STRIP);
          for (i = 0; i <= slices; i++) {
            switch(qobj->normals) {
              case GLU_FLAT:
                glNormal3f(sinCache3a[i] * sintemp2,
                    cosCache3a[i] * sintemp2,
                    costemp2);
                break;
              case GLU_SMOOTH:
                glNormal3f(sinCache2a[i] * sintemp2,
                    cosCache2a[i] * sintemp2,
                    costemp2);
                break;
              case GLU_NONE:
              default:
                break;
            }
            if (qobj->textureCoords) {
              glTexCoord2f(1 - (float) i / slices,
                  1 - (float) j / stacks);
            }
            glVertex3f(sintemp1 * sinCache1a[i],
                sintemp1 * cosCache1a[i], costemp1);
          }
          glEnd();
        }
        for (i = 0; i < slices; i++) {
          sintemp1 = sinCache1a[i];
          costemp1 = cosCache1a[i];
          switch(qobj->normals) {
            case GLU_FLAT:
            case GLU_SMOOTH:
              sintemp2 = sinCache2a[i];
              costemp2 = cosCache2a[i];
              break;
            default:
              break;
          }

          glBegin(GL_LINE_STRIP);
          for (j = 0; j <= stacks; j++) {
            switch(qobj->normals) {
              case GLU_FLAT:
                glNormal3f(sintemp2 * sinCache3b[j],
                    costemp2 * sinCache3b[j],
                    cosCache3b[j]);
                break;
              case GLU_SMOOTH:
                glNormal3f(sintemp2 * sinCache2b[j],
                    costemp2 * sinCache2b[j],
                    cosCache2b[j]);
                break;
              case GLU_NONE:
              default:
                break;
            }

            if (qobj->textureCoords) {
              glTexCoord2f(1 - (float) i / slices,
                  1 - (float) j / stacks);
            }
            glVertex3f(sintemp1 * sinCache1b[j],
                costemp1 * sinCache1b[j], cosCache1b[j]);
          }
          glEnd();
        }
        break;
      default:
        break;
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

    char *exts = (char *)glGetString(GL_EXTENSIONS);

    if (strstr(exts, "GL_EXT_texture_env_combine")) hasoverbright = true;
    else console::out("WARNING: cannot use overbright lighting, using old lighting model!");

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &glmaxtexsize);

    purgetextures();

    quadric qsphere;
    qsphere.normals = GLU_SMOOTH;
    qsphere.textureCoords = GL_TRUE;
    qsphere.orientation = GLU_INSIDE;
    qsphere.drawStyle = GLU_FILL;
    glNewList(1, GL_COMPILE);
    sphere(&qsphere, 1, 12, 6);
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

    if (hasoverbright)
    {
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

  void renderstrips()
  {
    int lasttex = -1;
    loopv(strips) if (strips[i].tex!=skyoglid)
    {
      if (strips[i].tex!=lasttex)
      {
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

  VARFP(gamma, 30, 100, 300,
      {
      float f = gamma/100.0f;
      if (SDL_SetGamma(f,f,f)==-1)
      {
      console::out("Could not set gamma (card/driver doesn't support it?)");
      console::out("sdl: %s", SDL_GetError());
      }
      });

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

  const char *hudgunnames[] = { "hudguns/fist", "hudguns/shotg", "hudguns/chaing", "hudguns/rocket", "hudguns/rifle" };

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
    {
      drawhudmodel(7, 18, rtime/18.0f, player1->lastaction);
    }
    else
    {
      drawhudmodel(6, 1, 100, 0);
    }

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

