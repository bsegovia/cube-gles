#ifndef __QBE_GAME_HPP__
#define __QBE_GAME_HPP__

namespace game
{
  void mousemove(int dx, int dy); 
  void updateworld(int millis);
  void initclient(void);
  void spawnplayer(dynent *d);
  void selfdamage(int damage, int actor, dynent *act);
  dynent *newdynent(void);
  char *getclientmap(void);
  const char *modestr(int n);
  void zapdynent(dynent *&d);
  dynent *getclient(int cn);
  void timeupdate(int timeremain);
  void resetmovement(dynent *d);
  void fixplayer1range(void);
  /* Brute force but effective way to find a free spawn spot in the map */
  void entinmap(dynent *d);
  /* Called just after a map load */
  void startmap(const char *name);
} /* namespace game */

#endif /* __QBE_GAME_HPP__ */

