#ifndef __CUBE_WEAPON_HPP__
#define __CUBE_WEAPON_HPP__

namespace cube {
namespace weapon {

void selectgun(int a = -1, int b = -1, int c =-1);
void shoot(dynent *d, vec &to);
void shootv(int gun, vec &from, vec &to, dynent *d = 0, bool local = false);
void createrays(vec &from, vec &to);
void moveprojectiles(float time);
void projreset(void);
char *playerincrosshair(void);
int reloadtime(int gun);

} /* namespace weapon */
} /* namespace cube */

#endif /* __CUBE_WEAPON_HPP__ */

