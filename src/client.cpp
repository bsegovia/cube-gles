// client.cpp, mostly network related client game code

#include "cube.h"

namespace client
{
  static ENetHost *clienthost = NULL;
  static int connecting = 0;
  static int connattempts = 0;
  static int disconnecting = 0;
  static int clientnum = -1; // our client id in the game
  static bool c2sinit = false; // whether we need to tell the other clients our stats
  static string ctext;
  static vector<ivector> messages; // collect c2s messages conveniently
  static int lastupdate = 0, lastping = 0;
  static string toservermap;
  // after a map change, since server doesn't have map data
  static bool senditemstoserver = false; 

  static string clientpassword;

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
  VARF(throttle_accel, 0, 2, 32, throttle());
  VARF(throttle_decel, 0, 2, 32, throttle());

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

    if (clienthost && !connecting)
      console::out("disconnected");
    clienthost = NULL;
    connecting = 0;
    connattempts = 0;
    disconnecting = 0;
    clientnum = -1;
    c2sinit = false;
    player1->lifesequence = 0;
    loopv(players)
      game::zapdynent(players[i]);

    localdisconnect();

    if (!onlyclean) {
      stop();
      localconnect();
    }
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

  void toserver(const char *text)
  {
    console::out("%s:\f %s", player1->name, text);
    strn0cpy(ctext, text, 80);
  }
  static void echo(char *text) { console::out("%s", text); }

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

  /*! Update the position of other clients in the game in our world don't care
   *  if he's in the scenery or other players, just don't overlap with our
   *  client
   */
  static void updatepos(dynent *d)
  {
    const float r = player1->radius+d->radius;
    const float dx = player1->o.x-d->o.x;
    const float dy = player1->o.y-d->o.y;
    const float dz = player1->o.z-d->o.z;
    const float rz = player1->aboveeye+d->eyeheight;
    const float fx = (float)fabs(dx);
    const float fy = (float)fabs(dy);
    const float fz = (float)fabs(dz);

    if (fx<r && fy<r && fz<rz && d->state!=CS_DEAD) {
      if (fx<fy)
        d->o.y += dy<0 ? r-fy : -(r-fy);  // push aside
      else 
        d->o.x += dx<0 ? r-fx : -(r-fx);
    }
    const int lagtime = lastmillis-d->lastupdate;
    if (lagtime) {
      d->plag = (d->plag*5+lagtime)/6;
      d->lastupdate = lastmillis;
    }
  }

  /*! Process forced map change from the server */
  static void changemapserv(const char *name, int mode)
  {
    gamemode = mode;
    load_world(name);
  }

  void localservertoclient(uchar *buf, int len)
  {
    if (ENET_NET_TO_HOST_16(*(ushort *)buf) != len)
      neterr("packet length");
    incomingdemodata(buf, len);

    uchar *end = buf+len;
    uchar *p = buf+2;
    char text[MAXTRANS];
    int cn = -1, type;
    dynent *d = NULL;
    bool mapchanged = false;

    while (p<end) switch (type = getint(p)) {
      // Welcome messsage from the server
      case SV_INITS2C: 
      {
        cn = getint(p);
        const int prot = getint(p);
        if (prot!=PROTOCOL_VERSION) {
          console::out("you are using a different game protocol (you: %d, server: %d)",
                       PROTOCOL_VERSION, prot);
          disconnect();
          return;
        }
        toservermap[0] = 0;
        clientnum = cn; // we are now fully connected
        // We are the first client on this server, set map
        if (!getint(p))
          strcpy_s(toservermap, game::getclientmap());
        sgetstr();
        if (text[0] && strcmp(text, clientpassword)) {
          console::out("you need to set the correct password to join this server!");
          disconnect();
          return;
        }
        if (getint(p)==1)
          console::out("server is FULL, disconnecting..");
        break;
      }

      // Position of another client
      case SV_POS:
      {
        cn = getint(p);
        d = game::getclient(cn);
        if (!d)
          return;
        d->o.x   = getint(p)/DMF;
        d->o.y   = getint(p)/DMF;
        d->o.z   = getint(p)/DMF;
        d->yaw   = getint(p)/DAF;
        d->pitch = getint(p)/DAF;
        d->roll  = getint(p)/DAF;
        d->vel.x = getint(p)/DVF;
        d->vel.y = getint(p)/DVF;
        d->vel.z = getint(p)/DVF;
        int f = getint(p);
        d->strafe = (f&3)==3 ? -1 : f&3;
        f >>= 2; 
        d->move = (f&3)==3 ? -1 : f&3;
        d->onfloor = (f>>2)&1;
        int state = f>>3;
        if (state==CS_DEAD && d->state!=CS_DEAD) d->lastaction = lastmillis;
        d->state = state;
        if (!demoplayback)
          updatepos(d);
        break;
      }

      // Sound to play
      case SV_SOUND:
        sound::play(getint(p), &d->o);
        break;

      // Text to print on screen
      case SV_TEXT:
        sgetstr();
        console::out("%s:\f %s", d->name, text); 
        break;

      // Map to load
      case SV_MAPCHANGE:
        sgetstr();
        changemapserv(text, getint(p));
        mapchanged = true;
        break;

      case SV_ITEMLIST:
      {
        int n;
        if (mapchanged) {
          senditemstoserver = false;
          resetspawns();
        }
        while ((n = getint(p))!=-1)
          if (mapchanged)
            setspawn(n, true);
        break;
      }

      // Server requests next map
      case SV_MAPRELOAD: {
        getint(p);
        sprintf_sd(nextmapalias)("nextmap_%s", game::getclientmap());
        char *map = cmd::getalias(nextmapalias); // look up map in the cycle
        changemap(map ? map : game::getclientmap());
        break;
      }

      // Another client either connected or changed name/team
      case SV_INITC2S:
      {
        sgetstr();
        if (d->name[0]) { // already connected
          if (strcmp(d->name, text))
            console::out("%s is now known as %s", d->name, text);
        } else { // new client
          c2sinit = false; // send new players my info again 
          console::out("connected: %s", text);
        }
        strcpy_s(d->name, text);
        sgetstr();
        strcpy_s(d->team, text);
        d->lifesequence = getint(p);
        break;
      }

      case SV_CDIS:
        cn = getint(p);
        if (!(d = game::getclient(cn)))
          break;
        console::out("player %s disconnected",
                     d->name[0] ? d->name : "[incompatible client]"); 
        game::zapdynent(players[cn]);
        break;

      case SV_SHOT:
      {
        const int gun = getint(p);
        vec s, e;
        s.x = getint(p)/DMF;
        s.y = getint(p)/DMF;
        s.z = getint(p)/DMF;
        e.x = getint(p)/DMF;
        e.y = getint(p)/DMF;
        e.z = getint(p)/DMF;
        if (gun==GUN_SG)
          createrays(s, e);
        shootv(gun, s, e, d);
        break;
      }

      // Damage done to a target
      case SV_DAMAGE:
      {
        const int target = getint(p);
        const int damage = getint(p);
        const int ls = getint(p);
        if (target==clientnum) {
          if (ls==player1->lifesequence)
            game::selfdamage(damage, cn, d);
        } else
          sound::play(S_PAIN1+rnd(5), &game::getclient(target)->o);
        break;
      }

      // Somebody died
      case SV_DIED:
      {
        const int actor = getint(p);
        if (actor==cn)
          console::out("%s suicided", d->name);
        else if (actor==clientnum) {
          int frags;
          if (isteam(player1->team, d->team)) {
            frags = -1;
            console::out("you fragged a teammate (%s)", d->name);
          } else {
            frags = 1;
            console::out("you fragged %s", d->name);
          }
          addmsg(1, 2, SV_FRAGS, player1->frags += frags);
        } else {
          const dynent * const a = game::getclient(actor);
          if (a) {
            if (isteam(a->team, d->name))
              console::out("%s fragged his teammate (%s)", a->name, d->name);
            else
              console::out("%s fragged %s", a->name, d->name);
          }
        }
        sound::play(S_DIE1+rnd(2), &d->o);
        d->lifesequence++;
        break;
      }

      // Update frag number for a player
      case SV_FRAGS:
        players[cn]->frags = getint(p);
        break;

      // Items has been picked up
      case SV_ITEMPICKUP:
        setspawn(getint(p), false);
        getint(p);
        break;

      // A new item spawned
      case SV_ITEMSPAWN:
      {
        uint i = getint(p);
        setspawn(i, true);
        if (i>=(uint)ents.length()) break;
        const vec v = {float(ents[i].x), float(ents[i].y), float(ents[i].z)};
        sound::play(S_ITEMSPAWN, &v); 
        break;
      }

      // Server acknowledges that I picked up this item
      case SV_ITEMACC:
        realpickup(getint(p), player1);
        break;

      // Coop editing messages, should be extended to include all possible
      // editing ops
      case SV_EDITH:
      case SV_EDITT:
      case SV_EDITS:
      case SV_EDITD:
      case SV_EDITE:
      {
        const int x  = getint(p);
        const int y  = getint(p);
        const int xs = getint(p);
        const int ys = getint(p);
        const int v  = getint(p);
        block b = { x, y, xs, ys };
        switch (type) {
          case SV_EDITH: editheightxy(v!=0, getint(p), b); break;
          case SV_EDITT: edittexxy(v, getint(p), b); break;
          case SV_EDITS: edittypexy(v, b); break;
          case SV_EDITD: setvdeltaxy(v, b); break;
          case SV_EDITE: editequalisexy(v!=0, b); break;
        }
        break;
      }

      // Coop edit of ent
      case SV_EDITENT:
      {
        uint i = getint(p);
        while ((uint)ents.length()<=i)
          ents.add().type = NOTUSED;
        const int to = ents[i].type;
        ents[i].type = getint(p);
        ents[i].x = getint(p);
        ents[i].y = getint(p);
        ents[i].z = getint(p);
        ents[i].attr1 = getint(p);
        ents[i].attr2 = getint(p);
        ents[i].attr3 = getint(p);
        ents[i].attr4 = getint(p);
        ents[i].spawned = false;
        if (ents[i].type==LIGHT || to==LIGHT)
          calclight();
        break;
      }

      // Ping message from the server
      case SV_PING:
        getint(p);
        break;

      // Acknowledges ping
      case SV_PONG: 
        addmsg(0, 2, SV_CLIENTPING, player1->ping = (player1->ping*5+lastmillis-getint(p))/6);
        break;

      // Get ping from a particular client
      case SV_CLIENTPING:
        players[cn]->ping = getint(p);
        break;

      // New game mode pushed by server
      case SV_GAMEMODE:
        nextmode = getint(p);
        break;

      // Time is up on the map
      case SV_TIMEUP:
        game::timeupdate(getint(p));
        break;

      // A new map is recieved
      case SV_RECVMAP:
      {
        sgetstr();
        console::out("received map \"%s\" from server, reloading..", text);
        const int mapsize = getint(p);
        writemap(text, mapsize, p);
        p += mapsize;
        changemapserv(text, gamemode);
        break;
      }

      // Output text from server
      case SV_SERVMSG:
        sgetstr();
        console::out("%s", text);
        break;

      // So we can messages without breaking previous clients/servers, if
      // necessary
      case SV_EXT:
        for (int n = getint(p); n; n--)
          getint(p);
        break;

      default:
        neterr("type");
        return;
    }
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

  void changemap(const char *name) { strcpy_s(toservermap, name); }

  COMMAND(echo, ARG_VARI);
  COMMANDN(say, toserver, ARG_VARI);
  COMMAND(connect, ARG_1STR);
  COMMANDN(disconnect, trydisconnect, ARG_NONE);
  COMMAND(password, ARG_1STR);
  COMMANDN(team, newteam, ARG_1STR);
  COMMANDN(name, newname, ARG_1STR);
  COMMANDN(map, changemap, ARG_1STR);

} /* namespace client */

