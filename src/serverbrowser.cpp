#include "cube.h"
#include "SDL/SDL_thread.h"
#include <enet/enet.h>

namespace cube {
namespace browser {

struct ResolverThread
{
  SDL_Thread *thread;
  char *query;
  int starttime;
};

struct ResolverResult
{
  char *query;
  ENetAddress address;
};

struct ServerInfo
{
  string name;
  string full;
  string map;
  string sdesc;
  int mode, numplayers, ping, protocol, minremain;
  ENetAddress address;
};

static vector<ResolverThread> ResolverThreads;
static vector<char *> resolverqueries;
static vector<ResolverResult> ResolverResults;
static SDL_mutex *resolvermutex = NULL;
static SDL_sem *resolversem = NULL;
static int resolverlimit = 1000;

static int resolverloop(void * data)
{
  ResolverThread *rt = (ResolverThread *) data;
  for (;;) {
    SDL_SemWait(resolversem);
    SDL_LockMutex(resolvermutex);
    if (resolverqueries.empty()) {
      SDL_UnlockMutex(resolvermutex);
      continue;
    }
    rt->query = resolverqueries.pop();
    rt->starttime = lastmillis;
    SDL_UnlockMutex(resolvermutex);
    ENetAddress address = { ENET_HOST_ANY, CUBE_SERVINFO_PORT };
    enet_address_set_host(&address, rt->query);
    SDL_LockMutex(resolvermutex);
    ResolverResult &rr = ResolverResults.add();
    rr.query = rt->query;
    rr.address = address;
    rt->query = NULL;
    rt->starttime = 0;
    SDL_UnlockMutex(resolvermutex);
  }
  return 0;
}

static void resolverinit(int threads, int limit)
{
  resolverlimit = limit;
  resolversem = SDL_CreateSemaphore(0);
  resolvermutex = SDL_CreateMutex();

  while (threads > 0) {
    ResolverThread &rt = ResolverThreads.add();
    rt.query = NULL;
    rt.starttime = 0;
    rt.thread = SDL_CreateThread(resolverloop, &rt);
    --threads;
  }
}

static void resolverstop(ResolverThread &rt, bool restart)
{
  SDL_LockMutex(resolvermutex);
  SDL_KillThread(rt.thread);
  rt.query = NULL;
  rt.starttime = 0;
  rt.thread = NULL;
  if (restart) rt.thread = SDL_CreateThread(resolverloop, &rt);
    SDL_UnlockMutex(resolvermutex);
}

static void resolverclear(void)
{
  SDL_LockMutex(resolvermutex);
  resolverqueries.setsize(0);
  ResolverResults.setsize(0);
  while (SDL_SemTryWait(resolversem) == 0);
  loopv(ResolverThreads) {
    ResolverThread &rt = ResolverThreads[i];
    resolverstop(rt, true);
  }
  SDL_UnlockMutex(resolvermutex);
}

static void resolverquery(char *name)
{
  SDL_LockMutex(resolvermutex);
  resolverqueries.add(name);
  SDL_SemPost(resolversem);
  SDL_UnlockMutex(resolvermutex);
}

static bool resolvercheck(char **name, ENetAddress *address)
{
  SDL_LockMutex(resolvermutex);
  if (!ResolverResults.empty()) {
    ResolverResult &rr = ResolverResults.pop();
    *name = rr.query;
    *address = rr.address;
    SDL_UnlockMutex(resolvermutex);
    return true;
  }
  loopv(ResolverThreads) {
    ResolverThread &rt = ResolverThreads[i];
      if (rt.query) {
        if (lastmillis - rt.starttime > resolverlimit) {
          resolverstop(rt, true);
          *name = rt.query;
          SDL_UnlockMutex(resolvermutex);
          return true;
        }
      }
  }
  SDL_UnlockMutex(resolvermutex);
    return false;
}

static vector<ServerInfo> servers;
static ENetSocket pingsock = ENET_SOCKET_NULL;
static int lastinfo = 0;

const char *getservername(int n) { return servers[n].name; }

void addserver(const char *servername)
{
  loopv(servers) if (strcmp(servers[i].name, servername)==0) return;
  ServerInfo &si = servers.insert(0, ServerInfo());
  strcpy_s(si.name, servername);
  si.full[0] = 0;
  si.mode = 0;
  si.numplayers = 0;
  si.ping = 9999;
  si.protocol = 0;
  si.minremain = 0;
  si.map[0] = 0;
  si.sdesc[0] = 0;
  si.address.host = ENET_HOST_ANY;
  si.address.port = CUBE_SERVINFO_PORT;
}

static void pingservers(void)
{
  ENetBuffer buf;
  uchar ping[MAXTRANS];
  uchar *p;
  loopv(servers) {
    ServerInfo &si = servers[i];
    if (si.address.host == ENET_HOST_ANY) continue;
    p = ping;
    server::putint(p, lastmillis);
    buf.data = ping;
    buf.dataLength = p - ping;
    enet_socket_send(pingsock, &si.address, &buf, 1);
  }
  lastinfo = lastmillis;
}

static void checkresolver(void)
{
  char *name = NULL;
  ENetAddress addr = {ENET_HOST_ANY, CUBE_SERVINFO_PORT};
  while (resolvercheck(&name, &addr)) {
    if (addr.host == ENET_HOST_ANY) continue;
    loopv(servers) {
      ServerInfo &si = servers[i];
      if (name == si.name) {
        si.address = addr;
        addr.host = ENET_HOST_ANY;
        break;
      }
    }
  }
}

static void checkpings(void)
{
  enet_uint32 events = ENET_SOCKET_WAIT_RECEIVE;
  ENetBuffer buf;
  ENetAddress addr;
  uchar ping[MAXTRANS], *p;
  char text[MAXTRANS];
  buf.data = ping;
  buf.dataLength = sizeof(ping);
  while (enet_socket_wait(pingsock, &events, 0) >= 0 && events) {
    if (enet_socket_receive(pingsock, &addr, &buf, 1) <= 0) return;
    loopv(servers) {
      ServerInfo &si = servers[i];
      if (addr.host == si.address.host) {
        p = ping;
        si.ping = lastmillis - server::getint(p);
        si.protocol = server::getint(p);
        if (si.protocol!=PROTOCOL_VERSION) si.ping = 9998;
        si.mode = server::getint(p);
        si.numplayers = server::getint(p);
        si.minremain = server::getint(p);
        sgetstr();
        strcpy_s(si.map, text);
        sgetstr();
        strcpy_s(si.sdesc, text);
        break;
      }
    }
  }
}

static int sicompare(const ServerInfo *a, const ServerInfo *b)
{
  return a->ping>b->ping ? 1 :
    (a->ping<b->ping ? -1 : strcmp(a->name, b->name));
}

void refreshservers(void)
{
  checkresolver();
  checkpings();
  if (lastmillis - lastinfo >= 5000) pingservers();
  servers.sort((void *)sicompare);
  int maxmenu = 16;
  loopv(servers) {
    ServerInfo &si = servers[i];
    if (si.address.host != ENET_HOST_ANY && si.ping != 9999) {
      if (si.protocol!=PROTOCOL_VERSION)
        sprintf_s(si.full)("%s [different cube protocol]", si.name);
      else
        sprintf_s(si.full)("%d\t%d\t%s, %s: %s %s",
          si.ping, si.numplayers, si.map[0] ? si.map : "[unknown]",
          game::modestr(si.mode), si.name, si.sdesc);
    } else
      sprintf_s(si.full)(si.address.host != ENET_HOST_ANY ?
        "%s [waiting for server response]" :
        "%s [unknown host]\t", si.name);
    si.full[50] = 0; // cut off too long server descriptions
    menu::manual(1, i, si.full);
    if (!--maxmenu) return;
  }
}

static void servermenu(void)
{
  if (pingsock == ENET_SOCKET_NULL) {
    pingsock = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM, NULL);
    resolverinit(1, 1000);
  }
  resolverclear();
  loopv(servers) resolverquery(servers[i].name);
  refreshservers();
  menu::set(1);
}

static void updatefrommaster(void)
{
  const int MAXUPD = 32000;
  uchar buf[MAXUPD];
  uchar *reply = server::retrieveservers(buf, MAXUPD);
  if (!*reply ||
      strstr((char *)reply, "<html>") ||
      strstr((char *)reply, "<HTML>"))
    console::out("master server not replying");
  else {
    servers.setsize(0);
    cmd::execute((char *)reply);
  }
  servermenu();
}

void writeservercfg(void)
{
  FILE *f = fopen("servers.cfg", "w");
  if (!f) return;
  fprintf(f, "// servers connected to are added here automatically\n\n");
  loopvrev(servers) fprintf(f, "addserver %s\n", servers[i].name);
  fclose(f);
}

COMMAND(addserver, ARG_1STR);
COMMAND(servermenu, ARG_NONE);
COMMAND(updatefrommaster, ARG_NONE);

} /* namespace browser */
} /* namespace cube */

