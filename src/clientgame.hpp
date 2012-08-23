#ifndef __QBE_GAME_HPP__
#define __QBE_GAME_HPP__

namespace game
{
  /*! Apply mouse movement to player1 */
  void mousemove(int dx, int dy); 
  /*! Main game update loop */
  void updateworld(int millis);
  /*! Create a new client */
  void initclient(void);
  /*! place at random spawn. also used by monsters! */
  void spawnplayer(dynent *d);
  /*! Damage arriving from the network, monsters, yourself, ends up here */
  void selfdamage(int damage, int actor, dynent *act);
  /*! Create a new blank player or monster */
  dynent *newdynent(void);
  /*! Get the map currently used by the client */
  const char *getclientmap(void);
  /*! Return the name of the mode currently played */
  const char *modestr(int n);
  /*! Free the entity */
  void zapdynent(dynent *&d);
  /*! Get entity for Client cn */
  dynent *getclient(int cn);
  /*! Useful for timi limited modes */
  void timeupdate(int timeremain);
  /*! Set the entity as not moving anymore */
  void resetmovement(dynent *d);
  /*! Properly clamp angle values (raw,pitch,yaw) */
  void fixplayer1range(void);
  /* Brute force but effective way to find a free spawn spot in the map */
  void entinmap(dynent *d);
  /* Called just after a map load */
  void startmap(const char *name);
  /*! Render all the clients */
  void renderclients(void);
  /*! Render the client */
  void renderclient(dynent *d, bool team, const char *mdlname, bool hellpig, float scale);
  /*! Render the score on screen */
  void renderscores(void);
} /* namespace game */

#endif /* __QBE_GAME_HPP__ */

