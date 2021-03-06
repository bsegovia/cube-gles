#ifndef __CUBE_SOUND_HPP__
#define __CUBE_SOUND_HPP__

#include <cstdlib>


namespace cube {

struct vec; // 3D vector

namespace sound {

/*! Init the sound module */
void init(void);
/*! Stop the sound module */
void clean(void);
/*! Play sound n at given location */
void play(int n, const vec *loc = NULL);
/*! Play sound n and send message to the server */
void playc(int n);
/*! Update the overall volume */
void updatevol(void);

} /* namespace sound */
} /* namespace cube */

/*! Hardcoded sounds, defined in sounds.cfg */
enum
{
  S_JUMP = 0, S_LAND, S_RIFLE, S_PUNCH1, S_SG, S_CG,
  S_RLFIRE, S_RLHIT, S_WEAPLOAD, S_ITEMAMMO, S_ITEMHEALTH,
  S_ITEMARMOUR, S_ITEMPUP, S_ITEMSPAWN, S_TELEPORT, S_NOAMMO, S_PUPOUT,
  S_PAIN1, S_PAIN2, S_PAIN3, S_PAIN4, S_PAIN5, S_PAIN6,
  S_DIE1, S_DIE2,
  S_FLAUNCH, S_FEXPLODE,
  S_SPLASH1, S_SPLASH2,
  S_GRUNT1, S_GRUNT2, S_RUMBLE,
  S_PAINO,
  S_PAINR, S_DEATHR,
  S_PAINE, S_DEATHE,
  S_PAINS, S_DEATHS,
  S_PAINB, S_DEATHB,
  S_PAINP, S_PIGGR2,
  S_PAINH, S_DEATHH,
  S_PAIND, S_DEATHD,
  S_PIGR1, S_ICEBALL, S_SLIMEBALL,
  S_JUMPPAD,
};

#endif /* __CUBE_SOUND_HPP__ */

