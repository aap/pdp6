#include "pdp6.h"

char *fmem_ident = FMEM_IDENT;
char *cmem_ident = CMEM_IDENT;
char *mmem_ident = MMEM_IDENT;

Membus memterm;

void
readmem(const char *file, word *mem, word size)
{
	FILE *f;
	char buf[100], *s;
	hword a;
	word w;
	if(f = fopen(file, "r"), f == nil)
		return;
	a = 0;
	while(s = fgets(buf, 100, f)){
		while(*s){
			if(*s == ';')
				break;
			else if('0' <= *s && *s <= '7'){
				w = strtol(s, &s, 8);
				if(*s == ':'){
					a = w;
					s++;
				}else if(a < size)
					mem[a++] = w;
				else
					fprintf(stderr, "Address out of range: %o\n", a++);
			}else
				s++;
		}
	}
	fclose(f);
}

void
writemem(const char *file, word *mem, word size)
{
	FILE *f;
	hword i, a;

	if(f = fopen(file, "w"), f == nil)
		return;

	a = 0;
	for(i = 0; i < size; i++)
		if(mem[i] != 0){
			if(a != i){
				a = i;
				fprintf(f, "%06o:\n", a);
			}
			fprintf(f, "%012lo\n", mem[a++]);
		}

	fclose(f);
}

static void
synccore(Mem *mem)
{
	CMem *core;
	core = mem->module;
	writemem(core->filename, core->core, core->size);
}

/* Both functions below are very confusing. I'm sorry.
 * The schematics cannot be converted to C in a straightfoward way
 * but I tried my best. */

/* This is based on the 161C memory */
static int
wakecore(Mem *mem, int sel, Membus *bus)
{
	bool p2, p3;
	CMem *core;
	core = mem->module;

	/* If not connected to a bus, find out which to connect to.
	 * Lower numbers have higher priority but proc 2 and 3 have
	 * the same priority and requests are alternated between them.
	 * If no request is made, we should already be connected to a bus */
	if(core->cmc_p_act < 0){
		if(mem->bus[0]->c12 & MEMBUS_RQ_CYC)
			core->cmc_p_act = 0;
		else if(mem->bus[1]->c12 & MEMBUS_RQ_CYC)
			core->cmc_p_act = 1;
		else{
			p2 = !!(mem->bus[2]->c12 & MEMBUS_RQ_CYC);
			p3 = !!(mem->bus[3]->c12 & MEMBUS_RQ_CYC);
			if(p2 && p3){
				if(core->cmc_last_proc == 2)
					core->cmc_p_act = 3;
				else if(core->cmc_last_proc == 3)
					core->cmc_p_act = 2;
			}else{
				if(p2)
					core->cmc_p_act = 2;
				else if(p3)
					core->cmc_p_act = 3;
				else
					return 1;	/* no request at all? */
			}
			core->cmc_last_proc = core->cmc_p_act;
		}
	}

	/* The executing thread can only service requests from its own processor
	 * due to synchronization with the processor's pulse cycle. */
	/* TODO: is this still true after the removal of pthreads? */
	if(bus != mem->bus[core->cmc_p_act])
		return 1;

	if(core->cmc_aw_rq && bus->c12 & MEMBUS_RQ_CYC){
		/* accept cycle request */
		core->cmc_aw_rq = 0;
		//trace("	accepting memory cycle from proc %d\n", core->cmc_p_act);

		core->cmb = 0;
		core->cma = 0;
		core->cma_rd_rq = 0;
		core->cma_wr_rq = 0;
		core->cmc_pse_sync = 0;

		/* strobe address and send acknowledge */
		core->cma |= bus->c12>>4 & 037777;
core->cma |= sel<<14;
core->cma &= core->mask;
		core->cma_rd_rq |= !!(bus->c12 & MEMBUS_RD_RQ);
		core->cma_wr_rq |= !!(bus->c12 & MEMBUS_WR_RQ);
		//trace("	sending ADDR ACK\n");
		bus->c12 |= MEMBUS_MAI_ADDR_ACK;

		/* read and send read restart */
		if(core->cma_rd_rq){
			core->cmb |= core->core[core->cma];
			core->core[core->cma] = 0;
			bus->c34 |= core->cmb & FW;
			bus->c12 |= MEMBUS_MAI_RD_RS;
			//trace("	sending RD RS\n");
			if(core->cma_wr_rq)
				core->cmb = 0;
			else
				core->cmc_p_act = -1;
		}
		core->cmc_pse_sync = 1;
		if(!core->cma_wr_rq)
			goto end;
	}
	/* write restart */
	if(core->cmc_pse_sync && bus->c12_pulse & MEMBUS_WR_RS){
		//trace("	accepting WR RS\n");
		core->cmc_p_act = -1;
		core->cmb |= bus->c34 & FW;
		bus->c34 = 0;
end:
		//trace("	writing\n");
		core->core[core->cma] = core->cmb;
		core->cmc_p_act = -1;	/* this seems unnecessary */
		core->cmc_aw_rq = 1;
	}
	//if(core->cmc_p_act < 0)
	//	trace("	now disconnected from proc\n");
	return 0;
}

static void
powercore(Mem *mem)
{
	CMem *core;

//	printf("[powercore]\n");
	core = mem->module;
	readmem(core->filename, core->core, core->size);
	core->cmc_aw_rq = 1;
	core->cmc_p_act = -1;
	core->cmc_last_proc = 2;	/* not reset by the hardware :/ */
}

/* This is based on the 162 memory */
static int
wakeff(Mem *mem, int sel, Membus *bus)
{
	FMem *ff;
	hword fma;
	bool fma_rd_rq, fma_wr_rq;
	bool fmc_wr_sel;
	ff = mem->module;

	/* Only respond to one processor */
	if(bus != mem->bus[ff->fmc_p_sel])
		return 1;

	fma = bus->c12>>4 & 017;
	fma_rd_rq = !!(bus->c12 & MEMBUS_RD_RQ);
	fma_wr_rq = !!(bus->c12 & MEMBUS_WR_RQ);
	if(!ff->fmc_act && bus->c12 & MEMBUS_RQ_CYC){
		//trace("	accepting memory cycle from proc %d\n", ff->fmc_p_sel);
		ff->fmc_act = 1;

		//trace("	sending ADDR ACK %o\n", fma);
		bus->c12 |= MEMBUS_MAI_ADDR_ACK;

		if(fma_rd_rq){
			bus->c34 |= ff->ff[fma] & FW;
			bus->c12 |= MEMBUS_MAI_RD_RS;
			//trace("	sending RD RS\n");
			if(!fma_wr_rq)
				goto end;
		}
		if(fma_wr_rq){
			ff->ff[fma] = 0;
			ff->fmc_wr = 1;
		}
	}
	fmc_wr_sel = ff->fmc_act && !fma_rd_rq;
	if(fmc_wr_sel && ff->fmc_wr && bus->c34){
		//trace("	writing fmem %o %012lo\n", fma, bus->c34);
		ff->ff[fma] |= bus->c34 & FW;
		bus->c34 = 0;
	}
	if(bus->c12_pulse & MEMBUS_WR_RS){
		//trace("	accepting WR RS\n");
end:
		ff->fmc_act = 0;
		ff->fmc_wr = 0;
	}
	//if(!ff->fmc_act)
	//	trace("	now available again\n");
	return 0;
}

static void
powerff(Mem *mem)
{
	FMem *ff;

//	printf("[powerff]\n");
	ff = mem->module;
	ff->fmc_act = 0;
	ff->fmc_wr = 0;
}

void
wakemem(Membus *bus)
{
	int sel;
	int nxm;

	if(bus->c12 & MEMBUS_MA_FMC_SEL1){
		nxm = bus->fmem->wake(bus->fmem, 0, bus);
		if(nxm)
			goto core;
	}else{
	core:
		sel = 0;
		if(bus->c12 & MEMBUS_MA21_1) sel |= 001;
		if(bus->c12 & MEMBUS_MA20_1) sel |= 002;
		if(bus->c12 & MEMBUS_MA19_1) sel |= 004;
		if(bus->c12 & MEMBUS_MA18_1) sel |= 010;
		if(bus->cmem[sel])
			nxm = bus->cmem[sel]->wake(bus->cmem[sel], sel, bus);
		else
			nxm = 1;
	}
	/* TODO: do something when memory didn't respond */
}

/* Allocate a core memory module and
 * a memory bus attachment. */
Mem*
makecoremem(const char *file, int size)
{
	CMem *core;
	Mem *mem;

	core = malloc(sizeof(CMem));
	memset(core, 0, sizeof(CMem));
	core->filename = strdup(file);
	core->size = size;
	core->mask = size-1;
	if((core->size & core->mask) != 0){
		fprintf(stderr, "%d not a power of 2\n", core->size);
		free(core->filename);
		free(core);
		return nil;
	}
	core->core = malloc(core->size*sizeof(word));
	memset(core->core, 0, core->size*sizeof(word));
	mem = malloc(sizeof(Mem));
	memset(mem, 0, sizeof(Mem));
	mem->dev.type = cmem_ident;
	mem->dev.name = "";

	mem->module = core;
	mem->bus[0] = &memterm;
	mem->bus[1] = &memterm;
	mem->bus[2] = &memterm;
	mem->bus[3] = &memterm;
	mem->wake = wakecore;
	mem->poweron = powercore;
	mem->sync = synccore;

	return mem;
}

Mem*
makefastmem(int p)
{
	FMem *ff;
	Mem *mem;

	ff = malloc(sizeof(FMem));
	memset(ff, 0, sizeof(FMem));

	ff->fmc_p_sel = p;

	mem = malloc(sizeof(Mem));
	memset(mem, 0, sizeof(Mem));
	mem->dev.type = fmem_ident;
	mem->dev.name = "";

	mem->module = ff;
	mem->bus[0] = &memterm;
	mem->bus[1] = &memterm;
	mem->bus[2] = &memterm;
	mem->bus[3] = &memterm;
	mem->wake = wakeff;
	mem->poweron = powerff;

	return mem;
}

Device*
makefmem(int argc, char *argv[])
{
	Mem *m;
	int p;

	if(argc > 0)
		p = atoi(argv[0]);
	else
		p = 0;
	m = makefastmem(p);
	m->poweron(m);
	return &m->dev;
}

Device*
make16kmem(int argc, char *argv[])
{
	Mem *m;
	char *path;
	if(argc > 0)
		path = argv[0];
	else
		path = "/dev/null";
	m = makecoremem(path, 16*1024);
	m->poweron(m);
	return &m->dev;
}
Device*
make256kmem(int argc, char *argv[])
{
	Mem *m;
	char *path;
	if(argc > 0)
		path = argv[0];
	else
		path = "/dev/null";
	m = makecoremem(path, 256*1024);
	m->dev.type = mmem_ident;
	m->poweron(m);
	return &m->dev;
}

/* Attach mem to bus for processor p.
 * If n < 0, it is attached as fast memory,
 * else at addresses starting at n<<14. */
void
attachmem(Mem *mem, int p, Membus *bus, int n)
{
	CMem *core;
	int i;

	if(n < 0)
		bus->fmem = mem;
	else if(mem->dev.type == cmem_ident || mem->dev.type == mmem_ident){
		core = mem->module;
		for(i = 0; i < core->size/040000; i++)
			bus->cmem[i+n] = mem;
	}
	mem->bus[p] = bus;
}

void
showmem(Membus *bus)
{
	int i;
	CMem *core;
	FMem *ff;

	if(bus->fmem){
		ff = bus->fmem->module;
		printf("fastmem: proc %d\n", ff->fmc_p_sel);
	}
	for(i = 0; i < 16; i++)
		if(bus->cmem[i]){
			core = bus->cmem[i]->module;
			printf("%s: %06o %06o\n", core->filename, i<<14, (i+1 << 14)-1);
		}
}

word*
getmemref(Membus *bus, hword addr, int fastmem)
{
	CMem *core;
	FMem *ff;

	if(fastmem && addr < 020 && bus->fmem){
		ff = bus->fmem->module;
		return &ff->ff[addr];
	}
	if(bus->cmem[addr>>14]){
		core = bus->cmem[addr>>14]->module;
		return &core->core[addr & core->mask];
	}
	return nil;
}
