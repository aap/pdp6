/* Memory map */
enum
{
	/* Core and Fast memory */
	MM_MEMIF_BASE	= 0,
	MM_MEMIF_ADDR	= MM_MEMIF_BASE+0,
	MM_MEMIF_LOW	= MM_MEMIF_BASE+1,
	MM_MEMIF_HIGH	= MM_MEMIF_BASE+2,

	/* APR */
	MM_APR_BASE	= 010,
	MM_APR_CTL1_DN	= MM_APR_BASE+0,
	MM_APR_CTL1_UP	= MM_APR_BASE+1,
	MM_APR_CTL2_DN	= MM_APR_BASE+2,
	MM_APR_CTL2_UP	= MM_APR_BASE+3,
	MM_APR_MAINT_UP	= MM_APR_BASE+4,
	MM_APR_MAINT_DN	= MM_APR_BASE+5,
	MM_APR_DSLT	= MM_APR_BASE+6,
	MM_APR_DSRT	= MM_APR_BASE+7,
	MM_APR_MAS	= MM_APR_BASE+010,
	MM_APR_REPEAT	= MM_APR_BASE+011,
	MM_APR_IR	= MM_APR_BASE+012,
	MM_APR_MILT	= MM_APR_BASE+013,
	MM_APR_MIRT	= MM_APR_BASE+014,
	MM_APR_PC	= MM_APR_BASE+015,
	MM_APR_MA	= MM_APR_BASE+016,
	MM_APR_PI	= MM_APR_BASE+017,
	MM_APR_MBLT	= MM_APR_BASE+020,
	MM_APR_MBRT	= MM_APR_BASE+021,
	MM_APR_ARLT	= MM_APR_BASE+022,
	MM_APR_ARRT	= MM_APR_BASE+023,
	MM_APR_MQLT	= MM_APR_BASE+024,
	MM_APR_MQRT	= MM_APR_BASE+025,
	MM_APR_FF1	= MM_APR_BASE+026,
	MM_APR_FF2	= MM_APR_BASE+027,
	MM_APR_FF3	= MM_APR_BASE+030,
	MM_APR_FF4	= MM_APR_BASE+031,
	MM_APR_MMU	= MM_APR_BASE+032,
	// TODO: more
};