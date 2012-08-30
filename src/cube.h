// one big bad include file for the whole engine... nasty!

#include "tools.h"

enum                            // static entity types
{
    NOTUSED = 0,                // entity slot not in use in map
    LIGHT,                      // lightsource, attr1 = radius, attr2 = intensity
    PLAYERSTART,                // attr1 = angle
    I_SHELLS, I_BULLETS, I_ROCKETS, I_ROUNDS,
    I_HEALTH, I_BOOST,
    I_GREENARMOUR, I_YELLOWARMOUR,
    I_QUAD,
    TELEPORT,                   // attr1 = idx
    TELEDEST,                   // attr1 = angle, attr2 = idx
    MAPMODEL,                   // attr1 = angle, attr2 = idx
    MONSTER,                    // attr1 = angle, attr2 = monstertype
    CARROT,                     // attr1 = tag, attr2 = type
    JUMPPAD,                    // attr1 = zpush, attr2 = ypush, attr3 = xpush
    MAXENTTYPES
};

struct persistent_entity        // map entity
{
  short x, y, z;              // cube aligned position
  short attr1;
  uchar type;                 // type is one of the above
  uchar attr2, attr3, attr4;
};

struct entity : public persistent_entity
{
    bool spawned;               // the only dynamic state of a map entity
};

#define MAPVERSION 5            // bump if map format changes, see worldio.cpp

struct header                   // map file format header
{
    char head[4];               // "CUBE"
    int version;                // any >8bit quantity is a little indian
    int headersize;             // sizeof(header)
    int sfactor;                // in bits
    int numents;
    char maptitle[128];
    uchar texlists[3][256];
    int waterlevel;
    int reserved[15];
};

#define SWS(w,x,y,s) (&(w)[(y)*(s)+(x)])
#define SW(w,x,y) SWS(w,x,y,ssize)
#define S(x,y) SW(map,x,y)            // convenient lookup of a lowest mip cube
#define SMALLEST_FACTOR 6               // determines number of mips there can be
#define DEFAULT_FACTOR 8
#define LARGEST_FACTOR 11               // 10 is already insane
#define SOLID(x) ((x)->type==SOLID)
#define MINBORD 2                       // 2 cubes from the edge of the world are always solid
#define OUTBORD(x,y) ((x)<MINBORD || (y)<MINBORD || (x)>=ssize-MINBORD || (y)>=ssize-MINBORD)

struct vec { float x, y, z; };
struct mapmodelinfo { int rad, h, zoff, snap; const char *name; };

enum { GUN_FIST = 0, GUN_SG, GUN_CG, GUN_RL, GUN_RIFLE, GUN_FIREBALL, GUN_ICEBALL, GUN_SLIMEBALL, GUN_BITE, NUMGUNS };

struct dynent                           // players & monsters
{
    vec o, vel;                         // origin, velocity
    float yaw, pitch, roll;             // used as vec in one place
    float maxspeed;                     // cubes per second, 24 for player
    bool outsidemap;                    // from his eyes
    bool inwater;
    bool onfloor, jumpnext;
    int move, strafe;
    bool k_left, k_right, k_up, k_down; // see input code
    int timeinair;                      // used for fake gravity
    float radius, eyeheight, aboveeye;  // bounding box size
    int lastupdate, plag, ping;
    int lifesequence;                   // sequence id for each respawn, used in damage test
    int state;                          // one of CS_* below
    int frags;
    int health, armour, armourtype, quadmillis;
    int gunselect, gunwait;
    int lastaction, lastattackgun, lastmove;
    bool attacking;
    int ammo[NUMGUNS];
    int monsterstate;                   // one of M_* below, M_NONE means human
    int mtype;                          // see monster.cpp
    dynent *enemy;                      // monster wants to kill this entity
    float targetyaw;                    // monster wants to look in this direction
    bool blocked, moving;               // used by physics to signal ai
    int trigger;                        // millis at which transition to another monsterstate takes place
    vec attacktarget;                   // delayed attacks
    int anger;                          // how many times already hit by fellow monster
    string name, team;
};

#define SAVEGAMEVERSION 4               // bump if dynent/netprotocol changes or any other savegame/demo data

enum { A_BLUE, A_GREEN, A_YELLOW };     // armour types... take 20/40/60 % off
enum { M_NONE = 0, M_SEARCH, M_HOME, M_ATTACKING, M_PAIN, M_SLEEP, M_AIMING };  // monster states

#define MAXCLIENTS 256                  // in a multiplayer game, can be arbitrarily changed
#define MAXTRANS 5000                   // max amount of data to swallow in 1 go
#define CUBE_SERVER_PORT 28765
#define CUBE_SERVINFO_PORT 28766
#define PROTOCOL_VERSION 122            // bump when protocol changes

// network messages codes, c2s, c2c, s2c
enum
{
    SV_INITS2C, SV_INITC2S, SV_POS, SV_TEXT, SV_SOUND, SV_CDIS,
    SV_DIED, SV_DAMAGE, SV_SHOT, SV_FRAGS,
    SV_TIMEUP, SV_EDITENT, SV_MAPRELOAD, SV_ITEMACC,
    SV_MAPCHANGE, SV_ITEMSPAWN, SV_ITEMPICKUP, SV_DENIED,
    SV_PING, SV_PONG, SV_CLIENTPING, SV_GAMEMODE,
    SV_EDITH, SV_EDITT, SV_EDITS, SV_EDITD, SV_EDITE,
    SV_SENDMAP, SV_RECVMAP, SV_SERVMSG, SV_ITEMLIST,
    SV_EXT,
};

enum { CS_ALIVE = 0, CS_DEAD, CS_LAGGED, CS_EDITING };


// vertex array format

struct vertex { float u, v, x, y, z; uchar r, g, b, a; };

typedef vector<dynent *> dvector;
typedef vector<char *> cvector;
typedef vector<int> ivector;

// globals ooh naughty

extern header hdr;                 // current map header
extern int sfactor, ssize;         // ssize = 2^sfactor
extern int cubicsize, mipsize;     // cubicsize = ssize^2
extern dynent *player1;            // special client ent that receives input and acts as camera
extern dvector players;            // all the other clients (in multiplayer)
extern bool editmode;
extern vector<entity> ents;        // map entities
extern vec worldpos;               // current target of the crosshair in the world
extern int lastmillis;             // last time
extern int curtime;                // current frame time
extern int gamemode, nextmode;
extern int xtraverts;
extern bool demoplayback;

#define DMF 16.0f
#define DAF 1.0f
#define DVF 100.0f

#define VIRTW 2400                      // virtual screen size for text & HUD
#define VIRTH 1800
#define FONTH 64
#define PIXELTAB (VIRTW/12)

#define PI  (3.1415927f)
#define PI2 (2*PI)

// simplistic vector ops
#define dotprod(u,v) ((u).x * (v).x + (u).y * (v).y + (u).z * (v).z)
#define vmul(u,f)    { (u).x *= (f); (u).y *= (f); (u).z *= (f); }
#define vdiv(u,f)    { (u).x /= (f); (u).y /= (f); (u).z /= (f); }
#define vadd(u,v)    { (u).x += (v).x; (u).y += (v).y; (u).z += (v).z; };
#define vsub(u,v)    { (u).x -= (v).x; (u).y -= (v).y; (u).z -= (v).z; };
#define vdist(d,v,e,s) vec v = s; vsub(v,e); float d = (float)sqrt(dotprod(v,v));
#define vreject(v,u,max) ((v).x>(u).x+(max) || (v).x<(u).x-(max) || (v).y>(u).y+(max) || (v).y<(u).y-(max))
#define vlinterp(v,f,u,g) { (v).x = (v).x*f+(u).x*g; (v).y = (v).y*f+(u).y*g; (v).z = (v).z*f+(u).z*g; }

#define sgetstr() { char *t = text; do { *t = getint(p); } while(*t++); }   // used by networking

#define m_noitems     (gamemode>=4)
#define m_noitemsrail (gamemode<=5)
#define m_arena       (gamemode>=8)
#define m_tarena      (gamemode>=10)
#define m_teammode    (gamemode&1 && gamemode>2)
#define m_sp          (gamemode<0)
#define m_dmsp        (gamemode==-1)
#define m_classicsp   (gamemode==-2)
#define isteam(a,b)   (m_teammode && strcmp(a, b)==0)

#define ATOI(s) strtol(s, NULL, 0) // supports hexadecimal numbers

#ifdef WIN32
    #define WIN32_LEAN_AND_MEAN
    #include "windows.h"
    #define _WINDOWS
    #define ZLIB_DLL
#else
    #include <dlfcn.h>
#endif

#include <time.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include <enet/enet.h>

#include <zlib.h>

#include "protos.h" // external function decls

