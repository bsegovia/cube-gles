// client.cpp, mostly network related client game code

#include "cube.h"

namespace client
{
  static ENetHost *clienthost = NULL;
  static int connecting = 0;
  static int connattempts = 0;
  static int disconnecting = 0;
  int clientnum = -1; // our client id in the game
  bool c2sinit = false; // whether we need to tell the other clients our stats

  int getclientnum(void) { return clientnum; }

  bool multiplayer(void)
  {
    // check not correct on listen server?
    if (clienthost)
      console::out("operation not available in multiplayer");
    return clienthost!=NULL;
  }

  void neterr(const char *s)
  {
    console::out("illegal network message (%s)", s);
    disconnect();
  }

  bool allowedittoggle(void)
  {
    const bool allow = !clienthost || gamemode==1;
    if (!allow)
      console::out("editing in multiplayer requires coopedit mode (1)");
    return allow; 
  }

  VARF(rate, 0, 0, 25000, 
    if (clienthost && (!rate || rate>1000))
      enet_host_bandwidth_limit (clienthost, rate, rate));

  static void throttle(void);
  VARF(throttle_interval, 0, 5, 30, throttle());
  VARF(throttle_accel,    0, 2, 32, throttle());
  VARF(throttle_decel,    0, 2, 32, throttle());

  static void throttle(void)
  {
    if (!clienthost || connecting) return;
    assert(ENET_PEER_PACKET_THROTTLE_SCALE==32);
    enet_peer_throttle_configure(clienthost->peers,
                                 throttle_interval*1000,
                                 throttle_accel, throttle_decel);
  }

  static void newname(const char *name) {
    c2sinit = false;
    strn0cpy(player1->name, name, 16);
  }

  static void newteam(const char *name) {
    c2sinit = false;
    strn0cpy(player1->team, name, 5);
  }

  void writeclientinfo(FILE *f) {
    fprintf(f, "name \"%s\"\nteam \"%s\"\n", player1->name, player1->team);
  }

  void connect(const char *servername)
  {
    disconnect(1);  // reset state
    addserver(servername);

    console::out("attempting to connect to %s", servername);
    ENetAddress address = {ENET_HOST_ANY, CUBE_SERVER_PORT};
    if (enet_address_set_host(&address, servername) < 0) {
      console::out("could not resolve server %s", servername);
      return;
    }

    clienthost = enet_host_create(NULL, 1, rate, rate);

    if (clienthost) {
      enet_host_connect(clienthost, &address, 1); 
      enet_host_flush(clienthost);
      connecting = lastmillis;
      connattempts = 0;
    } else {
      console::out("could not connect to server");
      disconnect();
    }
  }

  void disconnect(int onlyclean, int async)
  {
    if (clienthost) {
      if (!connecting && !disconnecting) {
        enet_peer_disconnect(clienthost->peers);
        enet_host_flush(clienthost);
        disconnecting = lastmillis;
      }
      if (clienthost->peers->state != ENET_PEER_STATE_DISCONNECTED) {
        if (async) return;
        enet_peer_reset(clienthost->peers);
      }
      enet_host_destroy(clienthost);
    }

    if (clienthost && !connecting) console::out("disconnected");
    clienthost = NULL;
    connecting = 0;
    connattempts = 0;
    disconnecting = 0;
    clientnum = -1;
    c2sinit = false;
    player1->lifesequence = 0;
    loopv(players) zapdynent(players[i]);

    localdisconnect();

    if (!onlyclean) { stop(); localconnect(); }
  }

  void trydisconnect()
  {
    if (!clienthost) {
      console::out("not connected");
      return;
    }
    if (connecting) {
      console::out("aborting connection attempt");
      disconnect();
      return;
    }
    console::out("attempting to disconnect...");
    disconnect(0, !disconnecting);
  }

  string ctext;
  void toserver(const char *text) {
    console::out("%s:\f %s", player1->name, text);
    strn0cpy(ctext, text, 80);
  }
  static void echo(char *text) { console::out("%s", text); }

  // collect c2s messages conveniently
  static vector<ivector> messages;

  void addmsg(int rel, int num, int type, ...)
  {
    if (demoplayback) return;
    if (num!=msgsizelookup(type)) {
      sprintf_sd(s)("inconsistant msg size for %d (%d != %d)",
        type, num, msgsizelookup(type));
      fatal(s);
    }
    if (messages.length()==100) {
      console::out("command flood protection (type %d)", type);
      return;
    }
    ivector &msg = messages.add();
    msg.add(num);
    msg.add(rel);
    msg.add(type);
    va_list marker;
    va_start(marker, type); 
    loopi(num-1) msg.add(va_arg(marker, int));
    va_end(marker);  
  }

  void server_err(void)
  {
    console::out("server network error, disconnecting...");
    disconnect();
  }

  static int lastupdate = 0, lastping = 0;
  string toservermap;
  // after a map change, since server doesn't have map data
  bool senditemstoserver = false; 

  string clientpassword;
  void password(char *p) { strcpy_s(clientpassword, p); }

  bool netmapstart(void)
  {
    senditemstoserver = true;
    return clienthost!=NULL;
  }

  void initclientnet(void)
  {
    ctext[0] = 0;
    toservermap[0] = 0;
    clientpassword[0] = 0;
    newname("unnamed");
    newteam("red");
  }

  void sendpackettoserv(void *packet)
  {
    if (clienthost) {
      enet_host_broadcast(clienthost, 0, (ENetPacket *)packet);
      enet_host_flush(clienthost);
    }
    else
      localclienttoserver((ENetPacket *)packet);
  }

  void c2sinfo(const dynent *d)
  {
    // We haven't had a welcome message from the server yet
    if (clientnum<0)
      return;
    // Don't update faster than 25fps
    if (lastmillis-lastupdate<40)
      return;
    ENetPacket *packet = enet_packet_create (NULL, MAXTRANS, 0);
    uchar *start = packet->data;
    uchar *p = start+2;
    bool serveriteminitdone = false;
    if (toservermap[0]) { // suggest server to change map
      // do this exclusively as map change may invalidate rest of update
      packet->flags = ENET_PACKET_FLAG_RELIABLE;
      putint(p, SV_MAPCHANGE);
      sendstring(toservermap, p);
      toservermap[0] = 0;
      putint(p, nextmode);
    } else {
      putint(p, SV_POS);
      putint(p, clientnum);
      // quantize coordinates to 1/16th of a cube, between 1 and 3 bytes
      putint(p, (int)(d->o.x*DMF)); 
      putint(p, (int)(d->o.y*DMF));
      putint(p, (int)(d->o.z*DMF));
      putint(p, (int)(d->yaw*DAF));
      putint(p, (int)(d->pitch*DAF));
      putint(p, (int)(d->roll*DAF));
      // quantize to 1/100, almost always 1 byte
      putint(p, (int)(d->vel.x*DVF));
      putint(p, (int)(d->vel.y*DVF));
      putint(p, (int)(d->vel.z*DVF));
      // pack rest in 1 byte: strafe:2, move:2, onfloor:1, state:3
      putint(p, (d->strafe&3) |
                ((d->move&3)<<2) |
                (((int)d->onfloor)<<4) |
                ((editmode ? CS_EDITING : d->state)<<5));

      if (senditemstoserver) {
        packet->flags = ENET_PACKET_FLAG_RELIABLE;
        putint(p, SV_ITEMLIST);
        if (!m_noitems) putitems(p);
        putint(p, -1);
        senditemstoserver = false;
        serveriteminitdone = true;
      }
      // player chat, not flood protected for now
      if (ctext[0]) {
        packet->flags = ENET_PACKET_FLAG_RELIABLE;
        putint(p, SV_TEXT);
        sendstring(ctext, p);
        ctext[0] = 0;
      }
      // tell other clients who I am
      if (!c2sinit) {
        packet->flags = ENET_PACKET_FLAG_RELIABLE;
        c2sinit = true;
        putint(p, SV_INITC2S);
        sendstring(player1->name, p);
        sendstring(player1->team, p);
        putint(p, player1->lifesequence);
      }
      // send messages collected during the previous frames
      loopv(messages) {
        ivector &msg = messages[i];
        if (msg[1]) packet->flags = ENET_PACKET_FLAG_RELIABLE;
        loopi(msg[0]) putint(p, msg[i+2]);
      }
      messages.setsize(0);
      if (lastmillis-lastping>250) {
        putint(p, SV_PING);
        putint(p, lastmillis);
        lastping = lastmillis;
      }
    }
    *(ushort *)start = ENET_HOST_TO_NET_16(p-start);
    enet_packet_resize(packet, p-start);
    incomingdemodata(start, p-start, true);
    if (clienthost) {
      enet_host_broadcast(clienthost, 0, packet);
      enet_host_flush(clienthost);
    } else
      localclienttoserver(packet);
    lastupdate = lastmillis;
    if (serveriteminitdone)
      loadgamerest();  // hack
  }

  void gets2c(void)
  {
    ENetEvent event;
    if (!clienthost) return;
    if (connecting && lastmillis/3000 > connecting/3000) {
      console::out("attempting to connect...");
      connecting = lastmillis;
      ++connattempts; 
      if (connattempts > 3) {
        console::out("could not connect to server");
        disconnect();
        return;
      }
    }
    while (clienthost!=NULL && enet_host_service(clienthost, &event, 0)>0)
      switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
          console::out("connected to server");
          connecting = 0;
          throttle();
          break;

        case ENET_EVENT_TYPE_RECEIVE:
          if (disconnecting)
            console::out("attempting to disconnect...");
          else
            localservertoclient(event.packet->data, event.packet->dataLength);
          enet_packet_destroy(event.packet);
          break;

        case ENET_EVENT_TYPE_DISCONNECT:
          if (disconnecting)
            disconnect();
          else
            server_err();
          return;
        default:
          break;
      }
  }

  COMMAND(echo, ARG_VARI);
  COMMANDN(say, toserver, ARG_VARI);
  COMMAND(connect, ARG_1STR);
  COMMANDN(disconnect, trydisconnect, ARG_NONE);
  COMMAND(password, ARG_1STR);
  COMMANDN(team, newteam, ARG_1STR);
  COMMANDN(name, newname, ARG_1STR);
}

