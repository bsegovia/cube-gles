#include "cube.h"
#include "ogl.hpp"

namespace cube {
namespace rdr {

struct md2_header
{
  int magic;
  int version;
  int skinWidth, skinHeight;
  int frameSize;
  int numSkins, numVertices, numTexcoords;
  int numTriangles, numGlCommands, numFrames;
  int offsetSkins, offsetTexcoords, offsetTriangles;
  int offsetFrames, offsetGlCommands, offsetEnd;
};

struct md2_vertex { uchar vertex[3], lightNormalIndex; };

struct md2_frame
{
  float scale[3];
  float translate[3];
  char name[16];
  md2_vertex vertices[1];
};

struct md2
{
  enum {floatn = 5}; /* s,t,x,y,z */
  int numGlCommands;
  int* glcommands;
  GLuint vbo;
  int framesz;
  int numTriangles;
  int frameSize;
  int numFrames;
  int numVerts;
  char* frames;
  bool *builtframes;
  int displaylist;
  int displaylistverts;
  mapmodelinfo mmi;
  char *loadname;
  int mdlnum;
  bool loaded;
  bool load(char* filename);
  void render(vec &light, int numFrame, int range, float x, float y, float z,
              float yaw, float pitch, float scale, float speed, int snap, int basetime);
  void scale(int frame, float scale, int sn);

  md2(void) { memset(this,0,sizeof(md2)); }

  ~md2(void) {
    if (vbo) OGL(DeleteBuffers, 1, &vbo);
    if (glcommands) delete [] glcommands;
    if (frames) delete [] frames;
    if (builtframes) delete [] builtframes;
  }
};


bool md2::load(char* filename)
{
  FILE* file;
  md2_header header;

  if ((file= fopen(filename, "rb"))==NULL) return false;

  size_t sz = fread(&header, sizeof(md2_header), 1, file); (void) sz;
  endianswap(&header, sizeof(int), sizeof(md2_header)/sizeof(int));

  if (header.magic!= 844121161 || header.version!=8) return false;

  frames = new char[header.frameSize*header.numFrames];
  if (frames==NULL) return false;

  fseek(file, header.offsetFrames, SEEK_SET);
  sz = fread(frames, header.frameSize*header.numFrames, 1, file); (void) sz;

  for (int i = 0; i < header.numFrames; ++i)
    endianswap(frames + i * header.frameSize, sizeof(float), 6);

  glcommands = new int[header.numGlCommands];
  if (glcommands==NULL) return false;

  fseek(file, header.offsetGlCommands, SEEK_SET);
  sz = fread(glcommands, header.numGlCommands*sizeof(int), 1, file); (void) sz;

  endianswap(glcommands, sizeof(int), header.numGlCommands);

  numFrames    = header.numFrames;
  numGlCommands= header.numGlCommands;
  frameSize    = header.frameSize;
  numTriangles = header.numTriangles;
  numVerts     = header.numVertices;

  fclose(file);

  /* XXX remove that as soon as we use vbo and shaders for animation */
  builtframes = new bool[numFrames];
  loopj(numFrames) builtframes[j] = false;

  /* allocate the complete vbo for all key frames */
  for (int *command=glcommands, i=0; (*command)!=0; ++i) {
    const int n = abs(*command++);
    framesz += 3*(n-2)*sizeof(float)*floatn;
    command += 3*n; /* +1 for index, +2 for u,v */
  }
  OGL(GenBuffers, 1, &vbo);
  ogl::bindbuffer(ogl::ARRAY_BUFFER, vbo);
  OGL(BufferData, GL_ARRAY_BUFFER, numFrames*framesz, NULL, GL_STATIC_DRAW);
  ogl::bindbuffer(ogl::ARRAY_BUFFER, 0);
  return true;
}

static float snap(int sn, float f)
{
  return sn ? (float)(((int)(f+sn*0.5f))&(~(sn-1))) : f;
}

void md2::scale(int frame, float scale, int sn)
{
  builtframes[frame] = true;
  const md2_frame *cf = (md2_frame *) ((char*)frames+frameSize*frame);
  const float sc = 16.0f/scale;

  vector<vvec<floatn>> tris;
  for (int *command = glcommands, i=0; (*command)!=0; ++i) {
    const int moden = *command++;
    const int n = abs(moden);
    vector<vvec<floatn>> trisv;
    loopi(n) {
      const float s = *((const float*)command++);
      const float t = *((const float*)command++);
      const int vn = *command++;
      const uchar *cv = (uchar *)&cf->vertices[vn].vertex;
      const vec v(+(snap(sn, cv[0]*cf->scale[0])+cf->translate[0])/sc,
                  -(snap(sn, cv[1]*cf->scale[1])+cf->translate[1])/sc,
                  +(snap(sn, cv[2]*cf->scale[2])+cf->translate[2])/sc);
      trisv.add(vvec<floatn>(s,t,v.x,v.z,v.y));
    }
    loopi(n-2) { /* just stolen from sauer. XXX use an index buffer */
      if (moden <= 0) { /* fan */
        tris.add(trisv[0]);
        tris.add(trisv[i+1]);
        tris.add(trisv[i+2]);
      } else /* strip */
        loopk(3) tris.add(trisv[i&1 && k ? i+(1-(k-1))+1 : i+k]);
    }
  }
  OGL(BufferSubData, GL_ARRAY_BUFFER, framesz*frame, framesz, &tris[0][0]);
}

void md2::render(vec &light, int frame, int range, float x, float y, float z,
                 float yaw, float pitch, float sc, float speed, int snap, int basetime)
{
  ogl::bindbuffer(ogl::ARRAY_BUFFER, vbo);
  loopi(range) if (!builtframes[frame+i]) scale(frame+i, sc, snap);

  OGL(VertexAttrib3fv, ogl::COL, &light.x);
  ogl::pushmatrix();
  ogl::translate(vec3f(x, y, z));
  ogl::rotate(yaw+180.f, vec3f(0.f, -1.f, 0.f));
  ogl::rotate(pitch, vec3f(0.f, 0.f, 1.f));

  const int n = framesz / sizeof(float[floatn]);
  const int time = lastmillis-basetime;
  intptr_t fr1 = (intptr_t)(time/speed);
  const float frac = (time-fr1*speed)/speed;
  fr1 = fr1%range+frame;
  intptr_t fr2 = fr1+1u;
  if (fr2>=frame+range) fr2 = frame;
  const float *pos0 = (const float*)(fr1*framesz);
  const float *pos1 = (const float*)(fr2*framesz);
  ogl::rendermd2(pos0, pos1, frac, n);
  ogl::bindbuffer(ogl::ARRAY_BUFFER, 0);
  ogl::xtraverts += n;

  ogl::popmatrix();
}

static hashtable<md2*> *mdllookup = NULL;
static vector<md2*> mapmodels;
static const int FIRSTMDL = 20;

void delayedload(md2 *m)
{
  if (!m->loaded) {
    sprintf_sd(name1)("packages/models/%s/tris.md2", m->loadname);
    if (!m->load(path(name1))) fatal("loadmodel: ", name1);
    sprintf_sd(name2)("packages/models/%s/skin.jpg", m->loadname);
    int xs, ys;
    ogl::installtex(FIRSTMDL+m->mdlnum, path(name2), xs, ys);
    m->loaded = true;
  }
}

static int modelnum = 0;

md2 *loadmodel(const char *name)
{
  if (!mdllookup) mdllookup = new hashtable<md2 *>;
  md2 **mm = mdllookup->access(name);
  if (mm) return *mm;
  md2 *m = new md2();
  m->mdlnum = modelnum++;
  mapmodelinfo mmi = { 2, 2, 0, 0, "" };
  m->mmi = mmi;
  m->loadname = newstring(name);
  mdllookup->access(m->loadname, &m);
  return m;
}

void mapmodel(char *rad, char *h, char *zoff, char *snap, char *name)
{
  md2 *m = loadmodel(name);
  mapmodelinfo mmi = { atoi(rad), atoi(h), atoi(zoff), atoi(snap), m->loadname };
  m->mmi = mmi;
  mapmodels.add(m);
}

void mapmodelreset(void) { mapmodels.setsize(0); }

mapmodelinfo &getmminfo(int i) {
  return i<mapmodels.length() ? mapmodels[i]->mmi : *(mapmodelinfo *)0;
}

COMMAND(mapmodel, ARG_5STR);
COMMAND(mapmodelreset, ARG_NONE);

void rendermodel(const char *mdl, int frame, int range, int tex,
                 float rad, float x, float y, float z,
                 float yaw, float pitch, bool teammate,
                 float scale, float speed, int snap, int basetime)
{
  md2 *m = loadmodel(mdl);

  if (world::isoccluded(player1->o.x, player1->o.y, x-rad, z-rad, rad*2))
    return;

  delayedload(m);

  int xs, ys;
  ogl::bindtexture(GL_TEXTURE_2D, tex ? ogl::lookuptex(tex, xs, ys) : FIRSTMDL+m->mdlnum);

  const int ix = (int)x;
  const int iy = (int)z;
  vec light(1.0f, 1.0f, 1.0f);

  if (!OUTBORD(ix, iy)) {
    sqr *s = S(ix,iy);
    const float ll = 256.0f; // 0.96f;
    const float of = 0.0f; // 0.1f;
    light.x = s->r/ll+of;
    light.y = s->g/ll+of;
    light.z = s->b/ll+of;
  }

  if (teammate) {
    light.x *= 0.6f;
    light.y *= 0.7f;
    light.z *= 1.2f;
  }

  m->render(light, frame, range, x, y, z, yaw, pitch, scale, speed, snap, basetime);
}

} /* namespace rdr */
} /* namespace cube */

