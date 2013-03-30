#ifndef __CUBE_ENTITIES_HPP__
#define __CUBE_ENTITIES_HPP__

namespace cube {

// XXX move that into the namespace
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

// XXX move that to namespace?
enum { GUN_FIST = 0, GUN_SG, GUN_CG, GUN_RL, GUN_RIFLE, GUN_FIREBALL, GUN_ICEBALL, GUN_SLIMEBALL, GUN_BITE, NUMGUNS };

enum { A_BLUE, A_GREEN, A_YELLOW };     // armour types... take 20/40/60 % off

struct persistent_entity      // map entity
{
  short x, y, z;              // cube aligned position
  short attr1;
  uchar type;                 // type is one of the above
  uchar attr2, attr3, attr4;
};

struct entity : persistent_entity
{
  bool spawned;               // the only dynamic state of a map entity
};

struct mapmodelinfo { int rad, h, zoff, snap; const char *name; };

/*! Players & Monsters */
struct dynent
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
  int monsterstate;                   // one of M_*, M_NONE means human
  int mtype;                          // see monster.cpp
  dynent *enemy;                      // monster wants to kill this entity
  float targetyaw;                    // monster wants to look in this direction
  bool blocked, moving;               // used by physics to signal ai
  int trigger;                        // millis at which transition to another monsterstate takes place
  vec attacktarget;                   // delayed attacks
  int anger;                          // how many times already hit by fellow monster
  string name, team;
};

typedef vector<dynent*> dvector;
extern dynent *player1;            // special client ent that receives input and acts as camera
extern dvector players;            // all the other clients (in multiplayer)
extern vector<entity> ents;        // map entities

namespace entities {

  void putitems(uchar *&p);
  void checkquad(int time);
  void checkitems(void);
  void realpickup(int n, dynent *d);
  void renderentities(void);
  void resetspawns(void);
  void setspawn(uint i, bool on);
  void teleport(int n, dynent *d);
  void baseammo(int gun);

} /* namespace entities */
} /* namespace cube */

#endif /* __CUBE_ENTITIES_HPP__ */

