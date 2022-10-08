/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// net_wins.c

#include "../qcommon/qcommon.h"

#define MAX_LOOPBACK 4

typedef struct {
  byte data[MAX_MSGLEN];
  int datalen;
} loopmsg_t;

typedef struct {
  loopmsg_t msgs[MAX_LOOPBACK];
  int get, send;
} loopback_t;

cvar_t *net_shownet;

loopback_t loopbacks[2];

bool NET_CompareAdr(netadr_t a, netadr_t b) {
  if(a.type != b.type)
    return false;

  if(a.type == NA_LOOPBACK)
    return TRUE;

  if(a.type == NA_IP) {
    if(a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] && a.port == b.port)
      return true;
    return false;
  }

  return false;
}

bool NET_CompareBaseAdr(netadr_t a, netadr_t b) {
  if(a.type != b.type)
    return false;

  if(a.type == NA_LOOPBACK)
    return TRUE;

  if(a.type == NA_IP) {
    if(a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3])
      return true;
    return false;
  }

  return false;
}

char *NET_AdrToString(netadr_t a) {
  static char s[64];

  if(a.type == NA_LOOPBACK)
    Com_sprintf(s, sizeof(s), "loopback");
  else if(a.type == NA_IP)
    Com_sprintf(s, sizeof(s), "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], ntohs(a.port));

  return s;
}

static inline bool StringToSockaddr(char *s, struct sockaddr *sadr) {
  struct hostent *h;
  char *colon;
  int val;
  char copy[128];

  memset(sadr, 0, sizeof(*sadr));

  ((struct sockaddr_in *)sadr)->sin_family = AF_INET;

  ((struct sockaddr_in *)sadr)->sin_port = 0;

  strcpy(copy, s);
  // strip off a trailing :port if present
  for(colon = copy; *colon; colon++)
    if(*colon == ':') {
      *colon = 0;
      ((struct sockaddr_in *)sadr)->sin_port = htons((short)atoi(colon + 1));
    }

  if(copy[0] >= '0' && copy[0] <= '9') {
    *(int *)&((struct sockaddr_in *)sadr)->sin_addr = inet_addr(copy);
  } else {
    if(!(h = gethostbyname(copy)))
      return 0;
    *(int *)&((struct sockaddr_in *)sadr)->sin_addr = *(int *)h->h_addr_list[0];
  }

  return true;
}

static inline void SockadrToNetadr(struct sockaddr *s, netadr_t *a) {
  if(s->sa_family == AF_INET) {
    a->type = NA_IP;
    *(int *)&a->ip = ((struct sockaddr_in *)s)->sin_addr.s_addr;
    a->port = ((struct sockaddr_in *)s)->sin_port;
  }
}

bool NET_StringToAdr(char *s, netadr_t *a) {
  struct sockaddr sadr;

  if(!strcmp(s, "localhost")) {
    memset(a, 0, sizeof(*a));
    a->type = NA_LOOPBACK;
    return true;
  }

  if(!StringToSockaddr(s, &sadr))
    return false;

  SockadrToNetadr(&sadr, a);

  return true;
}

bool NET_IsLocalAddress(netadr_t adr) { return adr.type == NA_LOOPBACK; }

bool NET_GetLoopPacket(netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message) {
  int i;
  loopback_t *loop;

  loop = &loopbacks[sock];

  if(loop->send - loop->get > MAX_LOOPBACK)
    loop->get = loop->send - MAX_LOOPBACK;

  if(loop->get >= loop->send)
    return false;

  i = loop->get & (MAX_LOOPBACK - 1);
  loop->get++;

  memcpy(net_message->data, loop->msgs[i].data, loop->msgs[i].datalen);
  net_message->cursize = loop->msgs[i].datalen;
  memset(net_from, 0, sizeof(*net_from));
  net_from->type = NA_LOOPBACK;
  return true;
}

void NET_SendLoopPacket(netsrc_t sock, int length, void *data, netadr_t to) {
  int i;
  loopback_t *loop;

  loop = &loopbacks[sock ^ 1];

  i = loop->send & (MAX_LOOPBACK - 1);
  loop->send++;

  memcpy(loop->msgs[i].data, data, length);
  loop->msgs[i].datalen = length;
}

bool NET_GetPacket(netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message) {
  if(NET_GetLoopPacket(sock, net_from, net_message))
    return true;
}

void NET_SendPacket(netsrc_t sock, int length, void *data, netadr_t to) {
  if(to.type == NA_LOOPBACK) {
    NET_SendLoopPacket(sock, length, data, to);
    return;
  }
}

static uv_udp_t ip_sockets[2];

static void RecvAllocate(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
  buf->base = malloc(suggested_size);
  buf->len = suggested_size;
}



void NET_Config(bool enable) {
  static bool server_on;
  static bool client_on;

  const char *ip = Cvar_Get("ip", "localhost", CVAR_NOSET)->value;
  int dedicated = Cvar_VariableValue("dedicated");

  if(server_on != enable) {
    server_on = enable;

    if(server_on) {
      struct sockaddr_in addr;
      addr.sin_port = Cvar_Get("ip_hostport", "0", CVAR_NOSET)->value;
      if(!addr.sin_port) {
        addr.sin_port = Cvar_Get("hostport", "0", CVAR_NOSET)->value;
        if(!addr.sin_port) {
          addr.sin_port = Cvar_Get("port", va("%i", PORT_SERVER), CVAR_NOSET)->value;
        }
      }

      if(!ip || !ip[0] || !stricmp(ip, "localhost"))
        addr.sin_addr.s_addr = INADDR_ANY;
      else
        NET_StringToSockaddr(ip, (struct sockaddr *)&addr);

      uv_udp_init(&global_uv_loop, &ip_sockets[NS_SERVER]);
      uv_udp_connect(&ip_sockets[NS_SERVER], (const struct sockaddr *)&addr);
      uv_udp_recv_start(&ip_sockets[NS_SERVER], RecvAllocate, SV_Recv);
    } else {
      uv_udp_recv_stop(&ip_sockets[NS_SERVER]);
    }

    if(multiplayer) {
      int port;

      cvar_t *ip = Cvar_Get("ip", "localhost", CVAR_NOSET);

      int dedicated = Cvar_VariableValue("dedicated");

      uv_udp_init(&global_uv_loop, &ip_sockets[NS_SERVER]);

    } else {
    }
  }

  // sleeps msec or until net socket is ready
  void NET_Sleep(int msec) {}

  void NET_Init(void) { net_shownet = Cvar_Get("net_shownet", "0", 0); }

  void NET_Shutdown(void) {}
