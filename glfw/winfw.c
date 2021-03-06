#include "opengl.h"
#include "ejoy2dgame.h"
#include "fault.h"
#include "screen.h"
#include "winfw.h"

#include <lauxlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

struct WINDOWGAME {
	struct game *game;
	int intouch;
};

static const int BUFSIZE = 2048;

static struct WINDOWGAME *G = NULL;

static const char * startscript =
"local _,script = ...\n"
"require(\"ejoy2d.framework\").WorkDir = ''\n"
"assert(script, 'I need a script name')\n"
"path = string.match(path,[[(.*)/[^/]*$]])\n"
"package.path = path .. [[/?.lua;]] .. path .. [[/?/init.lua]]\n"
"local f = assert(loadfile(script))\n"
"f(script)\n"
;

static struct WINDOWGAME *
create_game() {
	struct WINDOWGAME * g = (struct WINDOWGAME *)malloc(sizeof(*g));
	g->game = ejoy2d_game();
	g->intouch = 0;
	return g;
}

static int
traceback(lua_State *L) {
	const char *msg = lua_tostring(L, 1);
	if (msg)
		luaL_traceback(L, L, msg, 1);
	else if (!lua_isnoneornil(L, 1)) {
	if (!luaL_callmeta(L, 1, "__tostring"))
		lua_pushliteral(L, "(no error message)");
	}
	return 1;
}

#ifdef __APPLE__
static int
read_exepath(char * buf, int bufsz) {
    const char *path = getenv("_");
    if (!path)
        return -1;
    return snprintf(buf, bufsz, "local path = '%s'\n", path);
}
#else
static int
read_exepath(char * buf, int bufsz) {
    int  count;
    char tmp[BUFSIZE];
    count = readlink("/proc/self/exe", tmp, bufsz);

    if (count < 0)
        return -1;
    tmp[count] = '\0';
    return snprintf(buf, bufsz, "local path = '%s'\n", tmp);
}
#endif


void
ejoy2d_win_init(int argc, char *argv[]) {
	G = create_game();
	lua_State *L = ejoy2d_game_lua(G->game);
	lua_pushcfunction(L, traceback);
	int tb = lua_gettop(L);

    char buf[BUFSIZE];
    int cnt = read_exepath(buf, BUFSIZE);
    if (cnt < 0)
        fault("can't read exepath");
    char * bufp = buf + cnt;
    snprintf(bufp, BUFSIZE - cnt, startscript);

	int err = luaL_loadstring(L, buf);
	if (err) {
		const char *msg = lua_tostring(L,-1);
		fault("%s", msg);
	}

	int i;
	for (i=0;i<argc;i++) {
		lua_pushstring(L, argv[i]);
	}

	err = lua_pcall(L, argc, 0, tb);
	if (err) {
		const char *msg = lua_tostring(L,-1);
		fault("%s", msg);
	}

	lua_pop(L,1);

	screen_init(WIDTH,HEIGHT,1.0f);
	ejoy2d_game_start(G->game);
}

void
ejoy2d_win_update() {
	ejoy2d_game_update(G->game, 0.01f);
}

void
ejoy2d_win_frame() {
    //glClearColor(1.0,0.0,1.0,0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	ejoy2d_game_drawframe(G->game);
}

void
ejoy2d_win_touch(int x, int y,int touch) {
	switch (touch) {
	case TOUCH_BEGIN:
		G->intouch = 1;
		break;
	case TOUCH_END:
		G->intouch = 0;
		break;
	case TOUCH_MOVE:
		if (!G->intouch) {
			return;
		}
		break;
	}
	// windows only support one touch id (0)
	int id = 0;
	ejoy2d_game_touch(G->game, id, x,y,touch);
}

