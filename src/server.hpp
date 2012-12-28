#ifndef __QBE_SERVER_HPP__
#define __QBE_SERVER_HPP__

typedef struct _ENetPacket ENetPacket;

namespace server
{
  void init(bool dedicated, int uprate, const char *sdesc, const char *ip, const char *master, const char *passwd, int maxcl);
  void cleanup(void);
  void localconnect(void);
  void localdisconnect(void);
  void localclienttoserver(struct _ENetPacket *);
  void slice(int seconds, unsigned int timeout);
  void putint(uchar *&p, int n);
  int getint(uchar *&p);
  void sendstring(const char *t, uchar *&p);
  void startintermission(void);
  void restoreserverstate(vector<entity> &ents);
  uchar *retrieveservers(uchar *buf, int buflen);
  char msgsizelookup(int msg);
  void serverms(int mode, int numplayers, int minremain, char *smapname, int seconds, bool isfull);
  void servermsinit(const char *master, const char *sdesc, bool listen);
  void sendmaps(int n, string mapname, int mapsize, uchar *mapdata);
  ENetPacket *recvmap(int n);
} /* namespace server */

#define sgetstr() do { \
  char *t = text; \
  do { \
    *t = server::getint(p); \
  } while(*t++); \
} while (0)  // used by networking

#endif /* __QBE_SERVER_HPP__ */



