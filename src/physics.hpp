#ifndef __QBE_PHYSICS_HPP__
#define __QBE_PHYSICS_HPP__

namespace physics
{
  void moveplayer(dynent *pl, int moveres, bool local);
  bool collide(dynent *d, bool spawn, float drop, float rise);
  void setentphysics(int mml, int mmr);
  void physicsframe(void);
} /* namespace physics */

#endif /* __QBE_PHYSICS_HPP__ */


