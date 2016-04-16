#include "pdp6.h"

word iobus0, iobus1;
void (*iobusmap[128])(void);
u8 ioreq[128];
