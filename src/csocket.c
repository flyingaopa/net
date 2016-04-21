#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"


#define SOCKET_METATABLE "socket.metatable"
#define DPRINT(SD, FMT, ...) \
    if((SD)->dflag) fprintf(stderr, "%20s,%4d|%10s:%4d| " FMT "\n", __func__, __LINE__, "sock fd", (SD)->fd, __VA_ARGS__); 

typedef struct _sockdata {
    int fd;
    int domain;
    int type;
    int protocol;
    int dflag;
} sockdata;

static int _setdebug(lua_State *L)
{
    sockdata *sd = (sockdata *)luaL_checkudata(L, 1, SOCKET_METATABLE);
    int flag = lua_toboolean(L, 2);
    sd->dflag = flag;
    return 0;
}

static int _socket(lua_State *L)
{
    int domain = luaL_checkinteger(L, 1);
    int type = luaL_checkinteger(L, 2);
    int protocol = luaL_optinteger(L, 3, 0);

    int fd = socket(domain, type, protocol);
    if( fd < 0 )
    {
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }

    sockdata *sd = (sockdata *)lua_newuserdata(L, sizeof(sockdata));
    sd->fd = fd;
    sd->domain = domain;
    sd->type = type;
    sd->protocol = protocol;
    sd->dflag = 0;

    luaL_setmetatable(L, SOCKET_METATABLE);

    return 1;
}

static int _connect(lua_State *L)
{
    struct addrinfo hints;
    struct addrinfo *res;
    struct addrinfo *rp;
    int ret;

    sockdata *sd = (sockdata *)luaL_checkudata(L, 1, SOCKET_METATABLE);
    const char *host = luaL_checkstring(L, 2);
    const char *port = luaL_checkstring(L, 3);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = sd->domain;
    hints.ai_socktype = sd->type;
    hints.ai_protocol = sd->protocol;

    if( (ret = getaddrinfo(host, port, &hints, &res)) != 0 )
    {
        DPRINT(sd, "getaddrinfo: %s", gai_strerror(ret));
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }

    for( rp = res; rp != NULL; rp = rp->ai_next )
    {
        ret = connect(sd->fd, rp->ai_addr, rp->ai_addrlen);
        if( ret == 0 || ((ret == -1) && (errno == EINPROGRESS)) )
        {
            lua_pushboolean(L, 1);
            freeaddrinfo(res);
            return 1;
        }
        DPRINT(sd, "connect: %s", strerror(errno));
    }

    freeaddrinfo(res);
    lua_pushnil(L);
    lua_pushinteger(L, errno);
    return 2;
}

static int _bind(lua_State *L)
{
    struct addrinfo hints;
    struct addrinfo *res;
    struct addrinfo *rp;
    int ret;

    sockdata *sd = (sockdata *)luaL_checkudata(L, 1, SOCKET_METATABLE);
    const char *host = luaL_checkstring(L, 2);
    const char *port = luaL_checkstring(L, 3);

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = sd->domain;
    hints.ai_socktype = sd->type;
    hints.ai_protocol = sd->protocol;

    if( (ret = getaddrinfo(host, port, &hints, &res)) != 0 )
    {
        DPRINT(sd, "getaddrinfo: %s", gai_strerror(ret));
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }

    for( rp = res; rp != NULL; rp = rp->ai_next )
    {
        if( (ret = bind(sd->fd, rp->ai_addr, rp->ai_addrlen)) == 0 )
        {
            lua_pushboolean(L, 1);
            freeaddrinfo(res);
            return 1;
        }
        DPRINT(sd, "bind: %s", strerror(errno));
    }

    freeaddrinfo(res);
    lua_pushnil(L);
    lua_pushinteger(L, errno);
    return 2;
}

static int _listen(lua_State *L)
{
    sockdata *sd = (sockdata *)luaL_checkudata(L, 1, SOCKET_METATABLE);
    int bl = luaL_checkinteger(L, 2);
    if( listen(sd->fd, bl) == 0 )
    {
        lua_pushboolean(L, 1);
        return 1;
    }
    DPRINT(sd, "listen: %s", strerror(errno));
    lua_pushnil(L);
    lua_pushinteger(L, errno);
    return 2;
}

static int _accept(lua_State *L)
{
    int newfd;
    sockdata *sd = (sockdata *)luaL_checkudata(L, 1, SOCKET_METATABLE);
    if( (newfd = accept(sd->fd, NULL, NULL)) < 0 )
    {
        DPRINT(sd, "accept: %s", strerror(errno));
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }

    sockdata *newsd = (sockdata *)lua_newuserdata(L, sizeof(sockdata));
    newsd->fd = newfd;
    newsd->domain = sd->domain;
    newsd->type = sd->type;
    newsd->protocol = sd->protocol;

    DPRINT(sd, "new sock fd: %d", newsd->fd);
    luaL_getmetatable(L, SOCKET_METATABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int _setblocking(lua_State *L)
{
    sockdata *sd = (sockdata *)luaL_checkudata(L, 1, SOCKET_METATABLE);
    int bl = luaL_checkinteger(L, 2);

    int flag = fcntl(sd->fd, F_GETFL, 0);
    if( flag == -1 )
    {
        DPRINT(sd, "fcntl: %s", strerror(errno));
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }

    if( bl )
    {
        flag &= ~O_NONBLOCK;
    }
    else
    {
        flag |= O_NONBLOCK;
    }

    fcntl(sd->fd, F_SETFL, flag);
    lua_pushboolean(L, 1);
    return  1;
}

static int _send(lua_State *L)
{
    sockdata *sd = (sockdata *)luaL_checkudata(L, 1, SOCKET_METATABLE);
    size_t in_len;
    const char *buff = luaL_checklstring(L, 2, &in_len);
    int s_len = luaL_optinteger(L, 3, in_len);
    if( s_len > in_len )
    {
        s_len = in_len;
    }

    DPRINT(sd, "buffer: %s, len: %d", buff, s_len);
    int out_bytes = send(sd->fd, buff, s_len, 0);

    if( out_bytes == -1 )
    {
        DPRINT(sd, "send: %s", strerror(errno));
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }
    lua_pushinteger(L, out_bytes);
    return 1;
}

static int _sendto(lua_State *L)
{
    int ret;
    sockdata *sd = (sockdata *)luaL_checkudata(L, 1, SOCKET_METATABLE);
    const char *host = luaL_checkstring(L, 2);
    const char *port = luaL_checkstring(L, 3);
    size_t in_len;
    const char *buff = luaL_checklstring(L, 4, &in_len);
    int s_len = luaL_optinteger(L, 5, in_len);
    if( s_len > in_len )
    {
        s_len = in_len;
    }

    DPRINT(sd, "buffer: %s, len: %d", buff, s_len);

    struct addrinfo hints, *res, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = sd->domain;
    hints.ai_socktype = sd->type;
    hints.ai_protocol = sd->protocol;

    if( (ret = getaddrinfo(host, port, &hints, &res)) != 0 )
    {
        DPRINT(sd, "getaddrinfo: %s", gai_strerror(ret));
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2;
    }

    int out_bytes;
    for( rp = res; rp != NULL; rp = rp->ai_next )
    {
        out_bytes = sendto(sd->fd, buff, s_len, 0, rp->ai_addr, rp->ai_addrlen);
        if( out_bytes >= 0 )
        {
            lua_pushinteger(L, out_bytes);
            freeaddrinfo(res);
            return 1;
        }
        DPRINT(sd, "sendto: %s", strerror(errno));
    }

    freeaddrinfo(res);
    lua_pushnil(L);
    lua_pushinteger(L, errno);
    return 2;
}

static int _recv(lua_State *L)
{
    sockdata *sd = (sockdata *)luaL_checkudata(L, 1, SOCKET_METATABLE);
    size_t len = (lua_Unsigned)luaL_checkinteger(L, 2);
//#if defined ( __STDC_VERSION__ ) && __STDC_VERSION__ >= 199901L
    char buffer[len];
//#endif

    int rd = recv(sd->fd, buffer, len, 0);
    if( rd < 0 )
    {
        DPRINT(sd, "recv: %s", strerror(errno));
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }

    lua_pushlstring(L, buffer, rd);
    return 1;
}

static int _recvfrom(lua_State *L)
{
    struct sockaddr_storage addr;
    socklen_t addrlen;

    sockdata *sd = (sockdata *)luaL_checkudata(L, 1, SOCKET_METATABLE);
    size_t len = (lua_Unsigned)luaL_checkinteger(L, 2);
//#if defined ( __STDC_VERSION__ ) && __STDC_VERSION__ >= 199901L
    char buffer[len];
//#endif

    addrlen = sizeof(addr);
    int rd = recvfrom(sd->fd, buffer, len, 0, (struct sockaddr *)&addr, &addrlen);
    if( rd < 0 )
    {
        DPRINT(sd, "recvfrom: %s", strerror(errno));
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }

    lua_pushlstring(L, buffer, rd);

    char hostBfr[ NI_MAXHOST ];
    char servBfr[ NI_MAXSERV ];
    if( getnameinfo((struct sockaddr *)&addr, addrlen, hostBfr, sizeof(hostBfr),
            servBfr, sizeof(servBfr), NI_NUMERICHOST | NI_NUMERICSERV) == 0 )
    {
        lua_pushstring(L, hostBfr);
        lua_pushstring(L, servBfr);
        return 3;
    }

    return 1;
}

static int _close(lua_State *L)
{
    sockdata *sd = (sockdata *)luaL_checkudata(L, 1, SOCKET_METATABLE);
    if( sd->fd > 0 )
    {
        DPRINT(sd, "socket closed%s", "");
        close(sd->fd);
        sd->fd = -1;
    }
    return 0;
}

static int _fd(lua_State *L)
{
    sockdata *sd = (sockdata *)luaL_checkudata(L, 1, SOCKET_METATABLE);
    lua_pushinteger(L, sd->fd);
    return 1;
}

static int _tostring(lua_State *L)
{
    lua_pushfstring(L, "sock metadata: %p", (sockdata *)luaL_checkudata(L, 1, SOCKET_METATABLE));
    return 1;
}

static const luaL_Reg socketmethods[] =
{
    {"bind", _bind},
    {"fd", _fd},
    {"listen", _listen},
    {"accept", _accept},
    {"connect", _connect},
    {"recv", _recv},
    {"recvfrom", _recvfrom},
    {"send", _send},
    {"sendto", _sendto},
    {"close", _close},
    {"setblocking", _setblocking},
    {"setdebug", _setdebug},
    {NULL,NULL}
};

static const luaL_Reg interface[]=
{
    {"socket", _socket},
    {NULL,NULL}
};

static void _add_name(lua_State *L, const char *k, unsigned int v)
{
    lua_pushinteger(L, v);
    lua_setfield(L, -2, k);
}

#define ADD_NAME(L, name) _add_name(L, #name, name)
int luaopen_csocket(lua_State *L)
{
    luaL_checkversion(L);
    luaL_newmetatable(L, SOCKET_METATABLE);
    lua_pushcfunction(L, _close);
    lua_setfield(L, -2, "__gc");
    luaL_newlib(L, socketmethods);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, _tostring);
    lua_setfield(L, -2, "__tostring");

    luaL_newlib(L, interface);
    // address family
    ADD_NAME(L, AF_INET);
    ADD_NAME(L, AF_INET6);

    // socket type
    ADD_NAME(L, SOCK_STREAM);
    ADD_NAME(L, SOCK_DGRAM);

    // protocal type
    ADD_NAME(L, IPPROTO_TCP);
    ADD_NAME(L, IPPROTO_UDP); 
    return 1;
}

