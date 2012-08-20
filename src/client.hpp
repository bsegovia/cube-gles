#ifndef __QBE_CLIENT_HPP__
#define __QBE_CLIENT_HPP__

#include "tools.h"
#include <cstdio>

namespace client
{
  /*! Process any updates from the server */
  void localservertoclient(uchar *buf, int len);
  /*! Connect to the given server */
  void connect(const char *servername);
  /*! Disconned from the current server */
  void disconnect(int onlyclean = 0, int async = 0);
  /*! Send the given message to the server */
  void toserver(const char *text);
  /*! Add a new message to send to the server */
  void addmsg(int rel, int num, int type, ...);
  /*! Is it a multiplayer game? */
  bool multiplayer(void);
  /*! Toggle edit mode (restricted by client code) */
  bool allowedittoggle(void);
  /*! Set the ENetPacket to the server */
  void sendpackettoserv(void *packet);
  /*! Process (ENet) events from the server */
  void gets2c(void);
  /*! Send updates to the server */
  void c2sinfo(const dynent *d);
  /*! Triggers disconnection when invalid message is processed */
  void neterr(const char *s);
  /*! Create a default profile for the client */
  void initclientnet(void);
  /*! Indicate if the map can be started in MP */
  bool netmapstart(void);
  /*! Return our client ID in the game */
  int getclientnum(void);
  /*! Process forced map change from the server */
  void changemapserv(const char *name, int mode);
  /*! Outputs name and team in the given file */
  void writeclientinfo(FILE *f);
}

#endif /* __QBE_CLIENT_HPP__ */

