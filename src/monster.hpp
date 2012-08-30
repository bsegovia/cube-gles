#ifndef __QBE_MONSTER_HPP__
#define __QBE_MONSTER_HPP__

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

