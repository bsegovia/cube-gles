#ifndef __QBE_PROTOS_HPP__
#define __QBE_PROTOS_HPP__

#include "command.hpp"
#include "console.hpp"
#include "menu.hpp"
#include "sound.hpp"
#include "client.hpp"
#include "clientgame.hpp"
#include "world.hpp"
#include "editing.hpp"
#include "entities.hpp"
#include "monster.hpp"
#include "physics.hpp"
#include "renderer.hpp"
#include "demo.hpp"

// serverbrowser
extern void addserver(const char *servername);
extern const char *getservername(int n);
extern void refreshservers(void);
extern void writeservercfg(void);

// main
extern void fatal(const char *s, const char *o = "");
extern void *alloc(int s);
extern void keyrepeat(bool on);

// server
extern void initserver(bool dedicated, int uprate, const char *sdesc, const char *ip, const char *master, const char *passwd, int maxcl);
extern void cleanupserver();
extern void localconnect();
extern void localdisconnect();
extern void localclienttoserver(struct _ENetPacket *);
extern void serverslice(int seconds, unsigned int timeout);
extern void putint(uchar *&p, int n);
extern int getint(uchar *&p);
extern void sendstring(const char *t, uchar *&p);
extern void startintermission();
extern void restoreserverstate(vector<entity> &ents);
extern uchar *retrieveservers(uchar *buf, int buflen);
extern char msgsizelookup(int msg);
extern void serverms(int mode, int numplayers, int minremain, char *smapname, int seconds, bool isfull);
extern void servermsinit(const char *master, const char *sdesc, bool listen);
extern void sendmaps(int n, string mapname, int mapsize, uchar *mapdata);
extern ENetPacket *recvmap(int n);

// weapon
extern void selectgun(int a = -1, int b = -1, int c =-1);
extern void shoot(dynent *d, vec &to);
extern void shootv(int gun, vec &from, vec &to, dynent *d = 0, bool local = false);
extern void createrays(vec &from, vec &to);
extern void moveprojectiles(float time);
extern void projreset();
extern char *playerincrosshair();
extern int reloadtime(int gun);

// rndmap
extern void perlinarea(block &b, int scale, int seed, int psize);

#endif /* __QBE_PROTOS_HPP__ */

