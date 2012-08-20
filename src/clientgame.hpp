#ifndef __QBE_GAME_HPP__
#define __QBE_GAME_HPP__

namespace game
{
  void mousemove(int dx, int dy); 
  void updateworld(int millis);
  void startmap(const char *name);
  void changemap(const char *name);
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
}

#endif /* __QBE_GAME_HPP__ */

