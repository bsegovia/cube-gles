#ifndef __CUBE_PHYSICS_HPP__
#define __CUBE_PHYSICS_HPP__

namespace cube {
namespace physics {

void moveplayer(dynent *pl, int moveres, bool local);
bool collide(dynent *d, bool spawn, float drop, float rise);
void setentphysics(int mml, int mmr);
void physicsframe(void);

} /* namespace physics */
} /* namespace cube */

#endif /* __CUBE_PHYSICS_HPP__ */

