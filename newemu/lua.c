#include "common.h"
#include "pdp6.h"

#include <unistd.h>
#include <fcntl.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

lua_State *L;

//	initdevs(pdp);
//	attach_ptp(pdp);
//	attach_ptr(pdp);
//	attach_tty(pdp);
//	dis = attach_dis(pdp);
//	Dc136 *dc = attach_dc(pdp);
//	Ut551 *ut = attach_ut(pdp, dc);
//	ux1 = attach_ux(ut, 1);
//	ux2 = attach_ux(ut, 2);
//	ux3 = attach_ux(ut, 3);
//	ux4 = attach_ux(ut, 4);
//	// stub for ITS
//	attach_ge(pdp);


// this is not ideal...
extern Word memory[01000000];
extern Word fmem[020];
extern Dis340 *dis;
extern Ux555 *ux1;
extern Ux555 *ux2;
extern Ux555 *ux3;
extern Ux555 *ux4;

static PDP6 *pdp;

// things we would like to be able to do:

//	pdp->netmem_fd.fd = dial("maya", 20006);
//	if(pdp->netmem_fd.fd >= 0)
//		printf("netmem connected\n");
//	waitfd(&pdp->netmem_fd);


// fix the language a bit
static int
L_oct(lua_State *L)
{
	lua_Integer n = luaL_checkinteger(L, 1);
	int pad = luaL_optinteger(L, 2, -1);
	char buf[100];
	if(pad < 0)
		snprintf(buf, sizeof(buf), "%0llo", (unsigned long long)n);
	else
		snprintf(buf, sizeof(buf), "%0*llo", pad, (unsigned long long)n);
	lua_pushstring(L, buf);
	return 1;
}

static int
L_examine(lua_State *L)
{
	lua_Integer addr = luaL_checkinteger(L, 1);
	int fast = luaL_optinteger(L, 2, 1);
	if(addr >= 0) {
		if(addr < 020 && fast)
			lua_pushinteger(L, fmem[addr]);
		else if(addr < 01000000)
			lua_pushinteger(L, memory[addr]);
		else
			lua_pushinteger(L, 0);
	} else
		lua_pushinteger(L, 0);
	return 1;
}

static int
L_deposit(lua_State *L)
{
	lua_Integer addr = luaL_checkinteger(L, 1);
	lua_Integer word = luaL_checkinteger(L, 2);
	int fast = luaL_optinteger(L, 3, 1);
	if(addr >= 0) {
		if(addr < 020 && fast)
			fmem[addr] = word;
		else if(addr < 01000000)
			memory[addr] = word;
	}
	return 0;
}

Hword readmemory(const char *path);

static int
L_readmemory(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);
	int start = readmemory(path);
	lua_pushinteger(L, start);
	return 1;
}

static int
L_setpc(lua_State *L)
{
	int pc = luaL_checkinteger(L, 1);
	pdp->pc = pc & 0777777;
	return 0;
}

static int
L_ttycon(lua_State *L)
{
	const char *path = luaL_optstring(L, 1, "/tmp/tty");
	if(pdp->tty_fd.fd >= 0)
		closefd(&pdp->tty_fd);
	if((pdp->tty_fd.fd = open(path, O_RDWR)) < 0)
		printf("couldn't open %s\n", path);
	waitfd(&pdp->tty_fd);
	return 0;
}

static int
L_ptrmount(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);
	int fd = open(path, O_RDONLY);
	if(fd < 0)
		printf("couldn't open %s\n", path);
	else
		ptrmount(pdp, fd);
	return 0;
}

static int
L_ptrunmount(lua_State *L)
{
	ptrunmount(pdp);
	return 0;
}

static int
L_ptpmount(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);
	int fd = open(path, O_CREAT|O_WRONLY|O_TRUNC, 0644);
	if(fd < 0)
		printf("couldn't open %s\n", path);
	else
		ptpmount(pdp, fd);
	return 0;
}

static int
L_ptpunmount(lua_State *L)
{
	ptpunmount(pdp);
	return 0;
}

static int
L_uxmount(lua_State *L)
{
	int unit = luaL_checkinteger(L, 1);
	const char *path = luaL_checkstring(L, 2);

	Ux555 *ux[] = { ux1, ux2, ux3, ux4 };
	if(0 < unit && unit <= 4)
		uxmount(ux[unit-1], path);
	return 0;
}

static int
L_uxunmount(lua_State *L)
{
	int unit = luaL_checkinteger(L, 1);

	Ux555 *ux[] = { ux1, ux2, ux3, ux4 };
	if(0 < unit && unit <= 4)
		uxunmount(ux[unit-1]);
	return 0;
}

static int
L_discon(lua_State *L)
{
	const char *host = luaL_optstring(L, 1, "localhost");
	int port = luaL_optinteger(L, 2, 3400);
	int dis_fd = dial(host, port);
	if(dis_fd < 0)
		printf("can't open display\n");
	dis_connect(dis, dis_fd);
	return 0;
}

void
doline(char *line)
{
	char *retline = nil;
	asprintf(&retline, "return %s", line);

	int status = luaL_loadstring(L, retline);
	free(retline);

	if(status != LUA_OK) {
		lua_pop(L, 1);
		status = luaL_loadstring(L, line);
	}
	if(status != LUA_OK) {
		fprintf(stderr, "error: %s\n", lua_tostring(L, -1));
		return;
	}
	status = lua_pcall(L, 0, LUA_MULTRET, 0);

	const char *s;
	int n = lua_gettop(L);
	for(int i = 1; i <= n; i++) {
		if(lua_type(L, i) == LUA_TNUMBER) {
			lua_getglobal(L, "pnum");
			lua_pushvalue(L, i);
			lua_pcall(L, 1, 1, 0);
			s = lua_tostring(L, -1);
			printf("%s%s", i > 1 ? "\t" : "", s);
			lua_pop(L, 1);
		} else {
			s = luaL_tolstring(L, i, nil);
			printf("%s%s", i > 1 ? "\t" : "", s);
			lua_pop(L, 1);
		}
	}
	if (n > 0) {
		lua_pushvalue(L, n);
		lua_setglobal(L, "_");
		printf("\n");
	}
	lua_settop(L, 0);
}

void
initlua(PDP6 *apdp)
{
	L = luaL_newstate();
	luaL_openlibs(L);

	pdp = apdp;
	lua_register(L, "oct", L_oct);
	lua_register(L, "pnum", L_oct);
	lua_register(L, "e", L_examine);
	lua_register(L, "d", L_deposit);
	lua_register(L, "readmemory", L_readmemory);
	lua_register(L, "setpc", L_setpc);
	lua_register(L, "ttycon", L_ttycon);
	lua_register(L, "ptrmount", L_ptrmount);
	lua_register(L, "ptrunmount", L_ptrunmount);
	lua_register(L, "ptpmount", L_ptpmount);
	lua_register(L, "ptpunmount", L_ptpunmount);
	lua_register(L, "uxmount", L_uxmount);
	lua_register(L, "uxunmount", L_uxunmount);
	lua_register(L, "discon", L_discon);

	if(luaL_dofile(L, "init.lua") != LUA_OK) {
		fprintf(stderr, "error: %s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
		return;
	}
}
