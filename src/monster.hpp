#ifndef __QBE_MONSTER_HPP__
#define __QBE_MONSTER_HPP__

// XXX move that into namespace?
enum { M_NONE = 0, M_SEARCH, M_HOME, M_ATTACKING, M_PAIN, M_SLEEP, M_AIMING };  // monster states

namespace monster
{
  void monsterclear(void);
  void restoremonsterstate(void);
  void monsterthink(void);
  void monsterrender(void);
  dvector &getmonsters(void);
  void monsterpain(dynent *m, int damage, dynent *d);
  void endsp(bool allkilled);
} /* namespace monster */

#endif /* __QBE_MONSTER_HPP__ */

