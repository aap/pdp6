
enum {
	// sw0 data right
	// sw1 data left
	// sw2 addr
	// sw3
	KEY_START	= 01,
	KEY_READIN	= 02,
	KEY_INST_CONT	= 04,
	KEY_MEM_CONT	= 010,
	KEY_INST_STOP	= 020,
	KEY_MEM_STOP	= 040,
	KEY_IO_RESET	= 0100,
	KEY_EXEC	= 0200,
	KEY_DEP		= 0400,
	KEY_DEP_NXT	= 01000,
	KEY_EX		= 02000,
	KEY_EX_NXT	= 04000,

	SW_POWER	= 0400000,
	SW_REPEAT	= 0200000,
	SW_MEM_DISABLE	= 0100000,
	SW_ADDR_STOP	= 040000,

	// sw4
	KEY_MOTOR_ON	= 01,
	KEY_MOTOR_OFF	= 02,
	KEY_PTR_FEED	= 04,
	KEY_PTP_FEED	= 010,

	// l0	MI right
	// l1	MI left
	// l2	MA
	// l3	IR
	// l4	PC
	// l5
	L5_POWER	= 0400000,
	L5_REPEAT	= 0200000,
	L5_MEM_DISABLE	= 0100000,
	L5_ADDR_STOP	= 0040000,
	L5_RUN		= 0010000,
	L5_MC_STOP	= 0004000,
	L5_PI_ON	= 0002000,
	// pio		000177
	// l6
	// pih		037600
	// pir		000177
};

typedef struct Panel6 Panel6;
struct Panel6
{
	int sw0;
	int sw1;
	int sw2;
	int sw3;
	int sw4;
	int lights0;
	int lights1;
	int lights2;
	int lights3;
	int lights4;
	int lights5;
	int lights6;
};
