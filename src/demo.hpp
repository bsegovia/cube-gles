#ifndef __QBE_DEMO_HPP__
#define __QBE_DEMO_HPP__

struct vec;
#define SAVEGAMEVERSION 4               // bump if dynent/netprotocol changes or any other savegame/demo data

extern bool demoplayback;

namespace demo
{
  void loadgamerest(void);
  void incomingdata(uchar *buf, int len, bool extras = false);
  void playbackstep(void);
  void stop(void);
  void stopifrecording(void);
  void damage(int damage, vec &o);
  void blend(int damage);
} /* namespace demo */

#endif /* __QBE_DEMO_HPP__ */








