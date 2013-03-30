#ifndef __CUBE_CLIENT_HPP__
#define __CUBE_CLIENT_HPP__

#include "tools.h"
#include <cstdio>

namespace cube {

// XXX move that into namespace?
// network messages codes, c2s, c2c, s2c
enum {
  SV_INITS2C, SV_INITC2S, SV_POS, SV_TEXT, SV_SOUND, SV_CDIS,
  SV_DIED, SV_DAMAGE, SV_SHOT, SV_FRAGS,
  SV_TIMEUP, SV_EDITENT, SV_MAPRELOAD, SV_ITEMACC,
  SV_MAPCHANGE, SV_ITEMSPAWN, SV_ITEMPICKUP, SV_DENIED,
  SV_PING, SV_PONG, SV_CLIENTPING, SV_GAMEMODE,
  SV_EDITH, SV_EDITT, SV_EDITS, SV_EDITD, SV_EDITE,
  SV_SENDMAP, SV_RECVMAP, SV_SERVMSG, SV_ITEMLIST,
  SV_EXT,
};

enum { CS_ALIVE = 0, CS_DEAD, CS_LAGGED, CS_EDITING };

enum {
  MAXCLIENTS = 256,         // in a multiplayer game, can be arbitrarily changed
  MAXTRANS = 5000,          // max amount of data to swallow in 1 go
  CUBE_SERVER_PORT = 28765,
  CUBE_SERVINFO_PORT = 28766,
  PROTOCOL_VERSION = 122    // bump when protocol changes
};

static const float DMF = 16.0f;  //<! Quantizes positions 
static const float DAF = 1.0f;   //<! Quantizes angles
static const float DVF = 100.0f; //<! Quantizes velocities

struct dynent;

namespace client {

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
/*! Outputs name and team in the given file */
void writeclientinfo(FILE *f);
/*! Request map change, server may ignore */
void changemap(const char *name);

} /* namespace client */
} /* namespace cube */

#endif /* __CUBE_CLIENT_HPP__ */

