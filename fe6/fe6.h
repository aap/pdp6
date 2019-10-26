#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "util.h"
#include "pdp6common.h"

#define nil NULL

typedef uint64_t u64, word;
typedef uint32_t u32, hword;
typedef uint16_t u16;
typedef uint8_t u8;

#define LT 0777777000000
#define RT 0777777
#define FW (LT|RT)
#define F0 0400000000000

#define MAXMEM (16*1024)

enum {
	APR_DS = 01000020,
	APR_MAS,
	APR_RPT,
	APR_IR,
	APR_MI,
	APR_PC,
	APR_MA,
	APR_PIH,
	APR_PIR,
	APR_PIO,
	APR_RUN,
	APR_PION,
	APR_STOP,

#ifdef TEST
	APR_CTL1_DN,
	APR_CTL1_UP,
	APR_CTL2_DN,
	APR_CTL2_UP,
	APR_MB,
	APR_AR,
	APR_MQ,

	TTY_TTI,
	TTY_ST,
	PTR_PTR,
	PTR_ST,
	PTR_FE,

	FE_REQ,
#endif

	APR_END,
};

/* FE devices */
enum
{
	DEV_PTR = 0,
	DEV_PTP,
};

struct dev
{
	char *devname;
	int mode;
	int fd;
	char *path;
};
extern struct dev devtab[];


extern hword starta;
extern int started;
extern char *helpstr, *colhelpstr;

char *findacsym(word v);
char *findsymval(word v);

void quit(void);
void err(char *str);
void typestr(char *str);
void typestrnl(char *str);

void coloncmd(char *line);
void docmd(char *cmd, char *line);

int isrunning(void);
int isstopped(void);

void deposit(hword a, word w);
word examine(hword a);
void cpu_start(hword a);
void cpu_readin(hword a);
void cpu_setpc(hword pc);
void cpu_stopinst(void);
void cpu_stopmem(void);
void cpu_cont(void);
void cpu_nextinst(void);
void cpu_nextmem(void);
void cpu_exec(word inst);
void cpu_ioreset(void);
void cpu_printflags(void);

void fe_svc(void);

void init6(void);
void deinit6(void);
