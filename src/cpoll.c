#include <sys/epoll.h>
#include <errno.h>
#include <string.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define EPOLL_METATABLE "epoll.metatable"

#define DPRINT(SD, FMT, ...) \
    if((SD)->dflag) fprintf(stderr, "%20s,%4d|%10s:%4d| " FMT "\n", __func__, __LINE__, "epoll id", (SD)->instance, __VA_ARGS__);


typedef struct _epdata {
    int instance;
    int dflag;
} epdata;


static int _setdebug(lua_State *L)
{
    epdata *ed = (epdata *)luaL_checkudata(L, 1, EPOLL_METATABLE);
    int flag = lua_toboolean(L, 2);
    ed->dflag = flag;
    return 0;
}


static int _epoll_create(lua_State *L)
{
    int size = luaL_optinteger(L, 1, 1024);
    int instance = epoll_create(size);

    if( instance < 0 )
    {
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }

    epdata *ed = (epdata *)lua_newuserdata(L, sizeof(epdata));
    ed->instance = instance;

    luaL_setmetatable(L, EPOLL_METATABLE);
    lua_newtable(L);
    lua_setuservalue(L, -2);
    return 1;
}


static int _epoll_ctladd(lua_State *L)
{
    struct epoll_event ee;
    lua_settop(L, 4);
    memset(&ee, 0, sizeof(ee));
    epdata *ed = (epdata *)luaL_checkudata(L, 1, EPOLL_METATABLE);
    int fd = luaL_checkinteger(L, 2);
    ee.events = luaL_checkinteger(L, 3);
    ee.data.fd = fd;

    if( epoll_ctl(ed->instance, EPOLL_CTL_ADD, fd, &ee) < 0 )
    {
        DPRINT(ed, "epoll_ctl add: ", strerror(errno));
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }
    
    if( lua_type(L, 4) != LUA_TNIL )
    {
        DPRINT(ed, "epoll_ctl add: param 4 %d", lua_type(L, 4));
        lua_getuservalue(L, 1);
        lua_pushvalue(L, 4);
        lua_rawseti(L, -2, fd);
        lua_pop(L, 1);
    }

    DPRINT(ed, "epoll_ctl add: sock fd = %d, events = %d, data = %s", fd, ee.events, luaL_tolstring(L, 4, NULL));
    lua_pushboolean(L, 1);
    return 1;
}

static int _epoll_ctlmod(lua_State *L)
{
    struct epoll_event ee;
    lua_settop(L, 4);
    memset(&ee, 0, sizeof(ee));
    epdata *ed = (epdata *)luaL_checkudata(L, 1, EPOLL_METATABLE);
    int fd = luaL_checkinteger(L, 2);
    ee.events = luaL_checkinteger(L, 3);
    ee.data.fd = fd;

    if( epoll_ctl(ed->instance, EPOLL_CTL_MOD, fd, &ee) < 0 )
    {
        DPRINT(ed, "epoll_ctl mod: ", strerror(errno));
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }
    
    if( lua_type(L, 4) != LUA_TNIL )
    {
        lua_getuservalue(L, 1);
        lua_pushvalue(L, 4);
        lua_rawseti(L, -2, fd);
        lua_pop(L, 1);
    }

    DPRINT(ed, "epoll_ctl mod: sock fd = %d, events = %d, data = %s", fd, ee.events, luaL_tolstring(L, 4, NULL));
    lua_pushboolean(L, 1);
    return 1;
}

static int _epoll_ctldel(lua_State *L)
{
    struct epoll_event ee;
    lua_settop(L, 2);
    epdata *ed = (epdata *)luaL_checkudata(L, 1, EPOLL_METATABLE);
    int fd = luaL_checkinteger(L, 2);
    if( epoll_ctl(ed->instance, EPOLL_CTL_DEL, fd, NULL ) < 0 )
    {
        DPRINT(ed, "epoll_ctl del: ", strerror(errno));
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }

    lua_getuservalue(L, 1);
    lua_pushnil(L);
    lua_rawseti(L, -2, fd);
    lua_pop(L, 1);
    
    DPRINT(ed, "epoll_ctl del: sock fd = %d", fd);
    lua_pushboolean(L, 1);
    return 1;
}

static int _epoll_wait(lua_State *L)
{
    int idx;
    lua_settop(L, 3);
    epdata *ed = (epdata *)luaL_checkudata(L, 1, EPOLL_METATABLE);
    int num = luaL_checkinteger(L, 2);
    int to = luaL_checkinteger(L, 3);
    struct epoll_event events[num];
    int nfds = epoll_wait(ed->instance, events, num, to);
    if( nfds < 0 )
    {
        DPRINT(ed, "epoll_wait: ", strerror(errno));
        lua_pushnil(L);
        lua_pushinteger(L, errno);
        return 2;
    }
    else if( nfds == 0 )
    {
        lua_pushinteger(L, 0);
        return 1;
    }

    lua_pushinteger(L, nfds);
    lua_newtable(L);
    lua_getuservalue(L, 1);
    for( idx = 0; idx < nfds; idx++ )
    {
        lua_newtable(L);
        lua_pushstring(L, "fd");
        lua_pushinteger(L, events[idx].data.fd);
        lua_rawset(L, -3);
        lua_pushstring(L, "events");
        lua_pushinteger(L, events[idx].events);
        lua_rawset(L, -3);
        lua_pushstring(L, "data");
        lua_rawgeti(L, 6, events[idx].data.fd);
        lua_rawset(L, -3);
        lua_rawseti(L, 5, idx + 1);
    }
    lua_pop(L, 1);
    return 2;
}

static int _epoll_close(lua_State *L)
{
    epdata *ed = (epdata *)luaL_checkudata(L, 1, EPOLL_METATABLE);
    if( ed->instance >= 0 )
    {
        DPRINT(ed, "epoll closed%s", "");
        lua_pushnil(L);
        lua_setuservalue(L, -2);
        close(ed->instance);
        ed->instance = -1;
    }
    return 0;
}

const struct luaL_Reg interface[] = 
{
    {"epoll_create", _epoll_create},
    {NULL,NULL}
};

const struct luaL_Reg epollmethods[] =
{
    {"epoll_ctladd", _epoll_ctladd},
    {"epoll_ctldel", _epoll_ctldel},
    {"epoll_ctlmod", _epoll_ctlmod},
    {"epoll_wait", _epoll_wait},
    {"epoll_close", _epoll_close},
    {"setdebug", _setdebug},
    {NULL,NULL}
};

int luaopen_cepoll(lua_State *L)
{
    luaL_checkversion(L);

    luaL_newmetatable(L, EPOLL_METATABLE);
    lua_pushcfunction(L, _epoll_close);
    lua_setfield(L, -2, "__gc");
    luaL_newlib(L, epollmethods);
    lua_setfield(L, -2, "__index");

    luaL_newlib(L, interface);
    lua_pushinteger(L, EPOLLIN);
    lua_setfield(L, -2, "EPOLLIN");
    lua_pushinteger(L, EPOLLOUT);
    lua_setfield(L, -2, "EPOLLOUT");
    return 1;
}
