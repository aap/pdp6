#define PDP_REGS \
	X("DS", APR_DS)\
	X("MAS", APR_MAS)\
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
	X("CTL1", APR_CTL1_DN)\
	X("CTL1U", APR_CTL1_UP)\
	X("CTL2", APR_CTL2_DN)\
	X("CTL2U", APR_CTL2_UP)\
	X("MB", APR_MB)\
	X("AR", APR_AR)\
	X("MQ", APR_MQ)\
	X("TTY.TTI", TTY_TTI)\
	X("TTY.ST", TTY_ST)\
	X("PTR.PTR", PTR_PTR)\
	X("PTR.ST", PTR_ST)\
	X("PTR.FE", PTR_FE)\
	X("FE.REQ", FE_REQ)
#else
#define PDP_REGS_TEST
#endif
