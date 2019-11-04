#define PDP_REGS \
	X("DS", APR_DS)\
	X("AS", APR_AS)\
	X("RPT", APR_RPT)\
	X("IR", APR_IR)\
	X("MI", APR_MI)\
	X("PC", APR_PC)\
	X("MA", APR_MA)\
	X("PIH", APR_PIH)\
	X("PIR", APR_PIR)\
	X("PIO", APR_PIO)\
	X("RUN", APR_RUN)\
	X("PION", APR_PION)\
	X("STOP", APR_STOP)
#ifdef TEST
#define PDP_REGS_TEST \
	X("SW", APR_SW_DN)\
	X("SWU", APR_SW_UP)\
	X("MAINT", APR_MAINT_DN)\
	X("MAINTU", APR_MAINT_UP)\
	X("AR", APR_AR)\
	X("BR", APR_BR)\
	X("MQ", APR_MQ)\
	X("AD", APR_AD)\
	X("TTY.TTI", TTY_TTI)\
	X("TTY.ST", TTY_ST)\
	X("PTR.PTR", PTR_PTR)\
	X("PTR.ST", PTR_ST)\
	X("PTR.FE", PTR_FE)\
	X("FE.REQ", FE_REQ)
#else
#define PDP_REGS_TEST
#endif
