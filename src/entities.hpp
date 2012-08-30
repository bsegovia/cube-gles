#ifndef __QBE_ENTITIES_HPP__
#define __QBE_ENTITIES_HPP__
// XXX
extern void renderents(void);

namespace entities
{
  extern void putitems(uchar *&p);
  extern void checkquad(int time);
  extern void checkitems(void);
  extern void realpickup(int n, dynent *d);
  extern void renderentities(void);
  extern void resetspawns(void);
  extern void setspawn(uint i, bool on);
  extern void teleport(int n, dynent *d);
  extern void baseammo(int gun);
} /* namespace entities */

#endif /* __QBE_ENTITIES_HPP__ */

