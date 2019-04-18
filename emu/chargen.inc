enum {
	I = 020,
	R = 010,
	L = 014,
	U = 02,
	D = 03
};

static int char_na[] = {
	I, U|I, U|I, U|I, U|I, U|I, U|I,
	R|I, D|I, D|I, D|I, D|I, D|I, D|I,
	R|I, U|I, U|I, U|I, U|I, U|I, U|I,
	R|I, D|I, D|I, D|I, D|I, D|I, D|I,
	R|I, U|I, U|I, U|I, U|I, U|I, U|I,
	D|R, D|R, D, D, D, D,
	0
};

static int char_A[] = {
	I, U|I, U|I, U|I, U|I, U|I, U|R|I,
	R|I, R|I, R|D|I, D|I, L, L, L|D|I,
	R|I, R|I, R|I, D|I, D|I, D|I, R, R,
	0
};

static int char_B[] = {
	I, U|I, U|I, U|I, U|I, U|I, U|I, R|I,
	R|I, R|I, R|D|I, D|I, L|D|I, L|I, L|I,
	D, D, D|I, R|I, R|I, U|R|I, U|I,
	D|R, D|R,
	0
};

static int char_C[] = {
	U|I, U|I, U|I, U|I, U|I, U|R|I, R|I, R|I, D|R|I,
	D|L, D|L, D|L, D, D|I,
	R|I, R|I, U|R|I, D|R, R,
	0
};

static int char_D[] = {
	I, U|I, U|I, U|I, U|I, U|I, U|I,
	R|I, R|I, R|I, R|D|I, D|I, D|I, D|I, D|I,
	L, L, L|D|I, R|I, R|I, R, R, R,
	0
};

static int char_E[] = {
	I, U|I, U|I, U|I, U|I, U|I, U|I,
	R|I, R|I, R|I, R|I,
	D|L, D|L, D|L|I,
	R|I, R|I, R|I,
	D|L, D|L, D|L|I,
	R|I, R|I, R|I, R, R,
	0
};

static int char_F[] = {
	I, U|I, U|I, U|I, U|I, U|I, U|I,
	R|I, R|I, R|I, R|I,
	D|L, D|L, D|L|I,
	R|I, R|I, R|I,
	D|L, D|L, D|L,
	R, R, R, R, R,
	0
};

static int char_G[] = {
	U|I, U|I, U|I, U|I, U|I,
	U|R|I, R|I, R|I, D|R|I,
	D, D|I, L|I, L|I, D|L, D,
	D|I, R|I, R|I, U|R|I, U|I, D|R, D|R,
	0
};

static int char_H[] = {
	I, U|I, U|I, U|I, U|I, U|I, U|I,
	D|R, D, D|I, R|I, R|I,
	U, U, U|R|I,
	D|I, D|I, D|I, D|I, D|I, D|I, R, R,
	0
};

static int char_I[] = {
	U, U, U, U, U, U|R|I,
	R|I, R|I, D|L|I,
	D|I, D|I, D|I, D|I, D|L|I,
	R|I, R|I, R, R, R,
	0
};

static int char_J[] = {
	U|I, D|R|I, R|I, R|I,
	U|R|I, U|I, U|I, U|I, U|I, U|I,
	D|R, D|R, D, D, D, D,
	0
};

static int char_K[] = {
	I, U|I, U|I, U|I, U|I, U|I, U|I,
	R, R, R, R|I,
	D|L|I, D|L|I, D|L|I,
	D|R|I, D|R|I, D|R|I, R, R,
	0
};

static int char_L[] = {
	I, U|I, U|I, U|I, U|I, U|I, U|I,
	D|R, D, D, D, D, D|I,
	R|I, R|I, R|I, R, R,
	0
};

static int char_M[] = {
	I, U|I, U|I, U|I, U|I, U|I, U|I,
	D|R|I, D|R|I, U|R|I, U|R|I,
	D|I, D|I, D|I, D|I, D|I, D|I, R, R,
	0
};

static int char_N[] = {
	I, U|I, U|I, U|I, U|I, U|I, U|I,
	D|R|I, D|R|I, D|R|I,
	U, U, U|R|I,
	D|I, D|I, D|I, D|I, D|I, D|I, R, R,
	0
};

static int char_O[] = {
	U|I, U|I, U|I, U|I, U|I,
	U|R|I, R|I, R|I,
	D|R|I, D|I, D|I, D|I, D|I,
	L, L, D|L|I, R|I, R|I, R, R, R,
	0
};

static int char_P[] = {
	I, U|I, U|I, U|I, U|I, U|I, U|I,
	R|I, R|I, R|I,
	D|R|I, D|I,
	D|L|I, L|I, L|I,
	D|R, D|R, D|R, R, R,
	0
};

static int char_Q[] = {
	U|I, U|I, U|I, U|I, U|I,
	U|R|I, R|I, R|I,
	D|R|I, D|I, D|I, D|I,
	L, L|I, D|L, D|I,
	R|I, R|I, U|I, R|I, D|I, R, R,
	0
};

static int char_R[] = {
	I, U|I, U|I, U|I, U|I, U|I, U|I,
	R|I, R|I, R|I,
	D|R|I, D|I,
	D|L|I, L|I, L|I,
	D|R|I, D|R|I, D|R|I, R, R,
	0
};

static int char_S[] = {
	U|I, D|R|I, R|I, R|I, U|R|I, U|I,
	U|L|I, L|I, L|I, U|L|I, U|I,
	U|R|I, R|I, R|I, D|R|I, D|R, D|R, D, D, D,
	0
};

static int char_T[] = {
	U, U, U, U, U, U|I,
	R|I, R|I, R|I, R|I,
	D|L, L|I,
	D|I, D|I, D|I, D|I, D|I,
	R, R, R, R,
	0
};

static int char_U[] = {
	U|I, U|I, U|I, U|I, U|I, U|I,
	R, R, R, R|I,
	D|I, D|I, D|I, D|I, D|I,
	L, L, L|D|I, R|I, R|I, R, R, R,
	0
};

static int char_V[] = {
	R, R|I, U|L|I, U|L|I, U|I, U|I, U|I, U|I,
	R, R, R, R|I,
	D|I, D|I, D|I, D|I, D|L|I, D|L|I, R, R, R, R,
	0
};

static int char_W[] = {
	I, U|I, R|I, U|R|I, L, L|I, U|I, U|I, U|I, U|I,
	R, R, R, R|I,
	D|I, D|I, D|I, D|I, D|I, L|I, D|R|I, R, R,
	0
};

static int char_X[] = {
	I, U|R|I, U|R|I, U|I, U|I, U|L|I, U|L|I,
	R, R, R, R|I,
	D|L|I, D, D, D, D|I, D|R|I, R, R,
	0
};

static int char_Y[] = {
	U, U, U, U, U, U|I,
	R, R, R, R|I, D|L|I,
	L, L|I,
	D|R|I, D|I, D|I, D|I, D|I, R, R, R, R,
	0
};

static int char_Z[] = {
	I, U|I, U|R|I, U|I, L|I,
	U, U, U|I,
	R|I, R|I, R|I, R|I,
	D|I, D|L|I, D|R|I, L|I, L|I,
	D, D, D|L|I,
	R|I, R|I, R|I, R, R,
	0
};

static int char_null[] = {
	U, U|I, U|I, R|I, U|L|I,
	U|R|I, R|I, D|I, U|R|I,
	D|R|I, D|I, L|I, D|R|I,
	L, L|I, D|L|I, R|I, R|I, D|R, R, R,
	0
};

static int char_ex[] = {
	R, R|I, U, U|I, U|I, U|I, U|I, U|I,
	D|R, D|R, D|R, D|R, D, D,
	0
};

static int char_dq[] = {
	U, U, U, U|R|I, U|I, U|I,
	R, R|I, D|I, D|I, D|R, D|R, D, D,
	0
};

static int char_sh[] = {
	R, R|I, U|I, U|I, L|I, U|R|I, U|I, L|I, U|R|I, U|I,
	R, R|I, D|I, D|R|I, L|I, L|I, D|R|I, D|R|I,
	L|I, L|I, D|R|I, D|I, R, R, R,
	0
};

static int char_dl[] = {
	U|I, D|R|I, R|I, U|I, D|R|I, U|R|I, U|I,
	U|L|I, D|L|I, U|I, U|I, D|L|I, U|L|I, U|I,
	U|R|I, R|I, D|I, U|R|I, D|R|I, D|R, D|R, D, D, D,
	0
};

static int char_pr[] = {
	I, U|I, U|R|I, U|R|I, U|R|I, U|R|I, U|I,
	L|I, L|I, L|I, L|I, D|I, R|I,
	D|R, D|R, D,
	D|I, R|I, D|L|I, R|I, R, R,
	0
};

static int char_et[] = {
	R|I, U|L|I, U|I, U|R, U|R|I, U|I, U|L|I,
	D|L|I, D|I, D|R|I, D|R|I, D|I, D|I, U|R|I, U|R|I,
	D, D|I, R, R,
	0
};

static int char_sq[] = {
	U, U, U, U, U|R|I, U|I,
	R|I, D|I, D|I, D|R, D|R, D|R, D|R,
	0
};

static int char_lp[] = {
	R, R|I, U|L|I, U|L|I, U|I, U|I, U|R|I, U|R|I,
	D|R, D|R, D|R, D|R, D, D,
	0
};

static int char_rp[] = {
	U, U, U, U, U|R, U|R|I,
	D|R|I, D|R|I, D|I, D|I, D|L|I, D|L|I,
	R, R, R, R,
	0
};

static int char_as[] = {
	U|I, U|R|I, U|I, L|I, U|R|I, U|L|I, R,
	R|I, R, R|I, D|L|I, L|I, D|I, R|I, R|I,
	D|L|I, L|I, D|I, R, R|I, D|R, R,
	0
};

static int char_pl[] = {
	U, U, U|I, R|I, U|R|I, U|I,
	D|R, D|R|I, L|I, L|I, D|I, D|I,
	D|R, R, R, R,
	0
};

static int char_cm[] = {
	U|R|I, U|I, R|I, D|I, D|I, R, R, R, R,
	0
};

static int char_mi[] = {
	U, U, U|I, R|I, R|I, R|I, R|I, D|R, D|R, D,
	0
};

static int char_pe[] = {
	R|I, U|I, R|I, D|I, R, R, R, R,
	0
};

static int char_sl[] = {
	U|I, U|R|I, U|R|I, U|R|I, U|R|I,
	D|R, D|R, D, D, D,
	0
};

static int char_0[] = {
	U|I, U|R|I, L|I, U|I, U|I, U|I,
	U|R|I, R|I, R|I, D|R|I, D|I, L|I, D|L|I,
	R, R|I, D|I, D|I, D|L|I, L|I, L|I,
	R, R, R, R, R,
	0
};

static int char_1[] = {
	U, U, U, U, U|R|I, U|R|I, D|I,
	D|I, D|I, D|I, D|I, D|L|I, R|I, R|I, R,
	R, R,
	0
};

static int char_2[] = {
	U, U, U, U, U|I, U|R|I, R|I, R|I,
	D|R|I, D|I, D|L|I, L|I, L|I, D|L|I, D|I, D|I,
	R|I, R|I, R|I, R|I, R, R,
	0
};

static int char_3[] = {
	U|I, D|R|I, R|I, R|I, U|R|I, U|I, U|L|I,
	U|R|I, U|I, U|L|I, L|I, L|I, D|L|I,
	D|R, D|R|I, D|R, D|R, D|R, R,
	0
};

static int char_4[] = {
	U, U, U|I, R|I, R|I, U|L|I, U|R|I, U|R|I,
	D|I, D|I, D|R|I, L|I, D|I, D|I, D|I, R, R, R,
	0
};

static int char_5[] = {
	U|I, D|R|I, R|I, R|I, U|R|I, U|I, U|L|I, L|I, L|I,
	U|L|I, U|I, U|I, R|I, R|I, R|I, R|I,
	D|R, D|R, D, D, D, D,
	0
};

static int char_6[] = {
	R|I, U|L|I, U|I, U|R|I, L|I, U|I, U|I,
	U|R|I, R|I, R|I, D|R|I, D|L, D|L|I, R|I,
	D|R|I, D|I, L, D|L|I, R|I, R, R, R,
	0
};

static int char_7[] = {
	I, U|I, U|R|I, U|L, U, U|I, U|I,
	R|I, R|I, R|I, R|I, D|I, D|L|I, D|L|I,
	D|R, D|R, D|R, R,
	0
};

static int char_8[] = {
	R|I, U|L|I, U|I, U|R|I, U|L|I, U|I, U|R|I,
	R|I, R|I, D|R|I, D|I, L, D|L|I, R|I, D|R|I,
	D|I, L, D|L|I, R|I, R, R, R,
	0
};

static int char_9[] = {
	U|I, D|R|I, R|I, R|I, U|R|I, U|I, U|I, U|I, U|I,
	U|L|I, L|I, L|I, D|L|I, D|I, D|R|I, R|I, R|I,
	D|R, D|R, D|R,
	0
};

static int char_cl[] = {
	U, U, U, U|R|I, R|I, D|I, L|I,
	D, D|I, R|I, D|L|I, R|I, R, R, R, R,
	0
};

static int char_sm[] = {
	U, U, U, U, U|R|I, R|I, D|I, L|I,
	D, D|I, R|I, D|L|I, R|I, D|I, R, R, R, R,
	0
};

static int char_la[] = {
	R, R, R|I, U|L|I, U|L|I, U|L|I,
	U|R|I, U|R|I, U|R|I,
	D|R, D|R, D|R, D, D, D,
	0
};

static int char_eq[] = {
	U, U|I, U, U|I, R|I, D, D|I,
	R|I, U, U|I, R|I, D, D|I, R|I,
	U, U|I, D|R, D|R, D, D,
	0
};

static int char_ra[] = {
	R|I, U|R|I, U|R|I, U|R|I,
	U|L|I, U|L|I, U|L|I,
	D|R, D|R, D|R, D|R, D|R, D,
	0
};

static int char_qs[] = {
	U, U, U, U, U|I, U|R|I, R|I, R|I,
	D|R|I, D|I, D|L|I, L|I,
	D|I, D|I, D|I, R, R, R, R,
	0
};


static int char_bar[] = {
	R, R|I, U|I, U|I, U|I, U|I, U|I, U|I,
	D|R, D|R, D|R, D|R, D, D,
	0
};

static int char_bs[] = {
	U, U, U, U, U|I,
	D|R|I, D|R|I, D|R|I, D|R|I,
	D|R, R,
	0
};

static int char_Lb[] = {
	I, R|I, R|I, U|L, L|I,
	U|I, U|I, U|I, U|I, U|I,
	R|I, R|I, D|R, D|R, D|R, D|R, D, D,
	0
};

static int char_Rb[] = {
	R|U, R|U, U, U, U, U|I, R|I, R|I,
	D|I, D|I, D|I, D|I, D|L, D|L|I,
	R|I, R|I, U|I, D|R, R,
	0
};

static int char_LB[] = {
	R, R|I, U|L|I, U|I, U|L|I, U|R|I, U|I, U|R|I,
	D|R, D|R, D|R, D|R, D, D,
	0
};

static int char_RB[] = {
	U|R, U|R, U, U, U, U|I,
	D|R|I, D|I, D|R|I, D|L|I, D|I, D|L|I,
	R, R, R, R,
	0
};

static int char_Ra[] = {
	U, U, U|I, R|I, R|I, U, U|I,
	D|R|I, D|R|I, L|I, D|I, D|L|I,
	D|R, R, R, R,
	0
};

static int char_La[] = {
	R, U|R|I, L|U|I, L|U|I, R|I, U|I, R|U|I,
	D, D|I, R|I, R|I, D|R, D|R, D,
	0
};

static int char_Da[] = {
	U, U|I, D|R|I, D|R|I, U|I, U|I, U|I, U|I, U|I, U|I,
	D|R, D|R, D, D|I, D|L|I, D|R, R, R,
	0
};

/*
static int char_Ua[] = {
	R, R|I, U|I, U|I, U|I, U|I, R, R|I,
	U|L|I, L|I, U|I, D|L|I, D|L|I,
	D, D, D, D,
	0
};
*/
static int char_Ua[] = {
	R, R|I, U|I, U|I, U|I, U|I, L, L|I,
	U|R|I, R|I, U|I, D|R|I, D|R|I,
	D|R, D|R, D, D,
	0
};

static int char_tl[] = {
	U, U, U, U, U|I, U|R|I, D|R|I, D|R|I, U|R|I,
	D|R, D|R, D, D, D,
	0
};

static int char_tk[] = {
	U, U, U, U, U, U|R|I, D|R|I, D|R|I,
	D|L, D|L, D|L, D,
	0
};

static int char_dn[] = {
	D, D, D, D,
	0
};

static int char_up[] = {
	U, U, U, U,
	0
};

static int char_sp[] = { R, R, R, R, R, R, 0 };
static int char_lf[] = { D, D, D, D, D, D, D, D, D, D, D, 0 };
static int char_cr[] = { 0100, 0 };
static int char_esc[] = { 040 };
static int char_si[] = { 0200 };
static int char_so[] = { 0400 };

static int *chars[] = {
	char_null, char_A, char_B, char_C, char_D, char_E, char_F, char_G,
	char_H, char_I, char_J, char_K, char_L, char_M, char_N, char_O,
	char_P, char_Q, char_R, char_S, char_T, char_U, char_V, char_W,
	char_X, char_Y, char_Z, char_lf, char_cr, char_si, char_so, char_esc,
	char_sp, char_ex, char_dq, char_sh, char_dl, char_pr, char_et, char_sq,
	char_lp, char_rp, char_as, char_pl, char_cm, char_mi, char_pe, char_sl,
	char_0, char_1, char_2, char_3, char_4, char_5, char_6, char_7,
	char_8, char_9, char_cl, char_sm, char_la, char_eq, char_ra, char_qs,

	/* TODO: lower case. some of the unknowns */
	char_null, char_na, char_na, char_na, char_na, char_na, char_na, char_na,
	char_na, char_na, char_na, char_na, char_na, char_na, char_na, char_na,
	char_na, char_na, char_na, char_na, char_na, char_na, char_na, char_na,
	char_na, char_na, char_na, char_lf, char_cr, char_si, char_so, char_esc,
	char_sp, char_na, char_na, char_tl, char_na, char_na, char_Ua, char_Ra,
	char_Da, char_La, char_bs, char_Lb, char_Rb, char_LB, char_RB, char_na,
	char_na, char_na, char_bar, char_na, char_na, char_na, char_tk, char_Ua,
	char_na, char_na, char_na, char_dn, char_na, char_na, char_na, char_up,
};
