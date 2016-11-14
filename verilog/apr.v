`default_nettype none

module apr(
	input wire clk,
	input wire reset,

	// keys
	input wire key_start,
	input wire key_read_in,
	input wire key_mem_cont,
	input wire key_inst_cont,
	input wire key_mem_stop,
	input wire key_inst_stop,
	input wire key_exec,
	input wire key_io_reset,
	input wire key_dep,
	input wire key_dep_nxt,
	input wire key_ex,
	input wire key_ex_nxt,

	// switches
	input wire sw_addr_stop,
	input wire sw_mem_disable,
	input wire sw_repeat,
	input wire sw_power,
	input wire [0:35] datasw,
	input wire [18:35] mas,

	// maintenance switches
	input wire sw_rim_maint,
	input wire sw_repeat_bypass,
	input wire sw_art3_maint,
	input wire sw_sct_maint,
	input wire sw_split_cyc,

	// lights
	output [0:17] ir,
	output [0:35] mi,
	output [0:35] ar,
	output [0:35] mb,
	output [0:35] mq,
	output [18:35] pc,
	output [18:35] ma,
	output [0:8] fe,
	output [0:8] sc,
	output run,
	output mc_stop,
	output pi_active,
	output [1:7] pih,
	output [1:7] pir,
	output [1:7] pio,
	output [18:25] pr,
	output [18:25] rlr,
	output [18:25] rla,
	// TODO: all the flipflops?

	// membus
	output wire membus_wr_rs,
	output wire membus_rq_cyc,
	output wire membus_rd_rq,
	output wire membus_wr_rq,
	output wire [21:35] membus_ma,
	output wire [18:21] membus_sel,
	output wire membus_fmc_select,
	output wire [0:35] membus_mb_out,
	input  wire membus_addr_ack,
	input  wire membus_rd_rs,
	input  wire [0:35] membus_mb_in,

	// IO bus
	output wire iobus_iob_poweron,
	output wire iobus_iob_reset,
	output wire iobus_datao_clear,
	output wire iobus_datao_set,
	output wire iobus_cono_clear,
	output wire iobus_cono_set,
	output wire iobus_iob_fm_datai,
	output wire iobus_iob_fm_status,
	output wire [3:9]  iobus_ios,
	output wire [0:35] iobus_iob_out,
	input  wire [1:7]  iobus_pi_req,
	input  wire [0:35] iobus_iob_in
);

	/*
	 * KEY
	 */
	reg run;
	reg key_ex_st;
	reg key_dep_st;
	reg key_ex_sync;
	reg key_dep_sync;
	reg key_rim_sbr;
	reg key_rdwr;

	wire key_clr_rim = ~key_read_in |
		~key_mem_cont & ~key_inst_cont;
	wire key_ma_fm_mas = key_ex_sync | key_dep_sync |
		key_start_OR_read_in;
	wire key_execute = ~run & key_exec;
	wire key_start_OR_read_in = key_start | key_read_in;
	wire key_start_OR_cont_OR_read_in = key_inst_cont |
		key_start_OR_read_in;
	wire key_ex_OR_dep_nxt = key_ex_nxt | key_dep_nxt;
	wire key_dp_OR_dp_nxt = key_dep_sync | key_dep_nxt;
	wire key_run_AND_NOT_ex_OR_dep = run & ~key_ex_OR_dep_st;
	wire key_ex_OR_ex_nxt = key_ex_sync | key_ex_nxt;
	wire key_manual = key_ex | key_ex_nxt |
		key_dep | key_dep_nxt |
		key_start | key_inst_cont | key_mem_cont |
		key_io_reset | key_execute | key_read_in;
	wire key_ex_OR_dep_st = key_ex_st | key_dep_st;
	wire key_run_AND_ex_OR_dep = run & key_ex_OR_dep_st;
	wire key_execute_OR_dp_OR_dp_nxt = key_execute | key_dp_OR_dp_nxt;
	wire run_clr;
	wire mr_pwr_clr;
	wire mr_start = kt1 & key_io_reset | mr_pwr_clr;
	wire mr_clr = kt1 & key_manual & ~key_mem_cont |
		mr_pwr_clr | uuo_t1 | iat0 | xct_t0 | it0;
	wire kt0;
	wire kt0a;
	wire kt1;
	wire kt2;
	wire kt3;
	wire kt4;
	wire key_go;
	wire key_rdwr_ret;
	wire key_ma_clr = kt1 & key_ma_fm_mas;
	wire key_ma_fm_masw1 = kt2 & key_ma_fm_mas;
	wire key_ma_inc = kt1 & key_ex_OR_dep_nxt;
	wire key_ar_clr = kt1 & key_execute_OR_dp_OR_dp_nxt;
	wire key_ar_fm_datasw1 = kt2 & key_execute_OR_dp_OR_dp_nxt |
		cpa & iobus_iob_fm_datai;
	wire key_rd = kt3 & key_ex_OR_ex_nxt;
	wire key_wr = kt3 & key_dp_OR_dp_nxt;

	wire kt0a_D, kt1_D, kt2_D;
	pg key_pg0(.clk(clk), .reset(reset), .in(key_inst_stop), .p(run_clr));
	pg key_pg1(.clk(clk), .reset(reset), .in(sw_power), .p(mr_pwr_clr));
	pg key_pg2(.clk(clk), .reset(reset), .in(key_manual), .p(kt0));
	pa key_pa0(.clk(clk), .reset(reset), .in(kt0), .p(kt0a));
	dly100ns key_dly0(.clk(clk), .reset(reset), .in(kt0a), .p(kt0a_D));
	pa key_pa1(.clk(clk), .reset(reset),
		.in(kt0a_D & ~run |
		    kt0a & key_mem_cont |	// TODO: check run?
		    st7 & run & key_ex_OR_dep_st),
		.p(kt1));
	dly200ns key_dly1(.clk(clk), .reset(reset), .in(kt1), .p(kt1_D));
	pa key_pa2(.clk(clk), .reset(reset), .in(kt1_D), .p(kt2));
	dly200ns key_dly2(.clk(clk), .reset(reset), .in(kt2), .p(kt2_D));
	pa key_pa3(.clk(clk), .reset(reset), .in(kt2_D), .p(kt3));
	pa key_pa4(.clk(clk), .reset(reset),
		.in(kt3 & key_execute |
		    key_rdwr_ret |
		    mc_stop_set & key_mem_cont |
		    st7 & key_start_OR_cont_OR_read_in),
		.p(kt4));
	pa key_pa5(.clk(clk), .reset(reset),
		.in(kt3 & key_start_OR_cont_OR_read_in |
		    key_run_AND_ex_OR_dep),
		.p(key_go));
	pa key_pa6(.clk(clk), .reset(reset),
		.in(key_rdwr & mc_rs_t1),
		.p(key_rdwr_ret));

	/* add to this as needed */
	always @(posedge reset) begin
		run <= 0;
		key_ex_st <= 0;
		key_dep_st <= 0;
		ar <= 0;
		mb <= 0;
		mq <= 0;
	end

	always @(posedge clk) begin
		if(run_clr |
		   et0a & key_inst_stop |
		   et0a & ir_jrst & ir[10] & ~ex_user |
		   mr_pwr_clr)
			run <= 0;
		if(key_go)
			run <= 1;

		if(kt0a | key_go) begin
			key_ex_st <= 0;
			key_dep_st <= 0;
			key_ex_sync <= 0;
			key_dep_sync <= 0;
		end
		if(et0a) begin
			if(key_ex_sync) key_ex_st <= 1;
			if(key_dep_sync) key_dep_st <= 1;
		end
		if(kt0a) begin
			if(key_ex) key_ex_sync <= 1;
			if(key_dep) key_dep_sync <= 1;
		end

		if(key_rd | key_wr)
			key_rdwr <= 1;
		if(mr_clr | key_rdwr_ret)
			key_rdwr <= 0;	

		if(kt1 & key_read_in | sw_rim_maint)
			key_rim_sbr <= 1;
		else if(kt1 & key_clr_rim | it1a & ~ma18_31_eq_0)
			key_rim_sbr <= 0;
	end

	/*
	 * I
	 */
	reg if1a;
	wire at1_inh;
	// equivalent: ia_inh = at1_inh | (pi_rq & ~pi_cyc)
	wire ia_NOT_int = ~at1_inh & (~pi_rq | pi_cyc);
	wire iat0;
	wire it0;
	wire it1;
	wire it1a;

	pa i_pa0(.clk(clk), .reset(reset),
		.in(key_go | (st7 & key_run_AND_NOT_ex_OR_dep)),
		.p(it0));
	pa i_pa1(.clk(clk), .reset(reset),
		.in(pi_sync_D & pi_rq & ~pi_cyc),
		.p(iat0));
	pa i_pa2(.clk(clk), .reset(reset),
		.in(iat0_D1 |
		    pi_sync_D & if1a & ia_NOT_int),
		.p(it1));
	pa i_pa3(.clk(clk), .reset(reset),
		.in(mc_rs_t1 & if1a),
		.p(it1a));

	wire it0_D, iat0_D0, iat0_D1;
	ldly100us i_dly0(.clk(clk), .reset(reset),
		.in(run_clr),
		.l(at1_inh));
	dly50ns i_dly1(.clk(clk), .reset(reset),
		.in(it0),
		.p(it0_D));
	dly100ns i_dly2(.clk(clk), .reset(reset),
		.in(iat0),
		.p(iat0_D0));
	dly200ns i_dly23(.clk(clk), .reset(reset),
		.in(iat0),
		.p(iat0_D1));

	always @(posedge clk) begin
		if(mr_clr | it1a)
			if1a <= 0;
		if(it0_D | it1 | uuo_t2)
			if1a <= 1;
	end

	/*
	 * A
	 */
	reg af0;
	reg af3;
	reg af3a;
	wire at0;
	wire at1;
	wire at2;
	wire at3;
	wire at3a;
	wire at4;
	wire at5;

	pa a_pa0(.clk(clk), .reset(reset),
		.in(it1a | cht9 | mc_rs_t1 & af0),
		.p(at0));
	pa a_pa1(.clk(clk), .reset(reset),
		.in(pi_sync_D & ~if1a & ia_NOT_int),
		.p(at1));
	pa a_pa2(.clk(clk), .reset(reset),
		.in(at1 & ~ir14_17_eq_0),
		.p(at2));
	pa a_pa3(.clk(clk), .reset(reset),
		.in(mc_rs_t1 & af3),
		.p(at3));
	pa a_pa4(.clk(clk), .reset(reset),
		.in(ar_t3 & af3a),
		.p(at3a));
	pa a_pa5(.clk(clk), .reset(reset),
		.in(at1 & ir14_17_eq_0 | at3a_D),
		.p(at4));
	pa a_pa6(.clk(clk), .reset(reset),
		.in(at4 & ir[13]),
		.p(at5));

	wire at3a_D, at5_D;
	dly100ns a_dly0(.clk(clk), .reset(reset),
		.in(at3a),
		.p(at3a_D));
	dly50ns a_dly1(.clk(clk), .reset(reset),
		.in(at5),
		.p(at5_D));

	always @(posedge clk) begin
		if(mr_clr | at0)
			af0 <= 0;
		if(at5)
			af0 <= 1;
		if(mr_clr | at3)
			af3 <= 0;
		if(at2)
			af3 <= 1;
		if(mr_clr | at3a)
			af3a <= 0;
		if(at3)
			af3a <= 1;
	end

	/*
	 * F
	 */
	reg f1a;
	reg f4a;
	reg f6a;
	wire ft0;
	wire ft1 = 0;
	wire ft1a = 0;
	wire ft3 = 0;
	wire ft4 = 0;
	wire ft4a = 0;
	wire ft5 = 0;
	wire ft6 = 0;
	wire ft6a = 0;
	wire ft7 = 0;
	wire f_c_c_acrt = 0;
	wire f_c_c_aclt = 0;
	wire f_ac_2 = 0;
	wire f_c_c_aclt_OR_rt = 0;
	wire f_ac_inh = 0;
	wire f_c_e = 0;
	wire f_c_e_pse = 0;
	wire f_c_e_OR_pse = 0;

	pa f_pa0(.clk(clk), .reset(reset),
		.in(at4 & ~ir[13] | iot_t0a_D),
		.p(ft0));

	/*
	 * E
	 */
	reg et4_ar_pse;
	wire et0a = 0;
	wire et0 = 0;
	wire et1 = 0;
	wire et3 = 0;
	wire et4 = 0;
	wire et5 = 0;
	wire et6 = 0;
	wire et7 = 0;
	wire et8 = 0;
	wire et9 = 0;
	wire et10 = 0;
	wire et4_inh = 0;
	wire et5_inh = 0;
	wire e_long = 0;

	/*
	 * S
	 */
	reg sf3;
	reg sf5a;
	reg sf7;
	wire st1 = 0;
	wire st2 = 0;
	wire st3 = 0;
	wire st3a = 0;
	wire st5 = 0;
	wire st5a = 0;
	wire st6 = 0;
	wire st7 = 0;
	wire s_c_e = 0;
	wire s_ac_inh_if_ac_0 = 0;
	wire s_ac_inh = 0;
	wire s_ac_2 = 0;
	wire s_ac_0 = 0;

	/*
	 * IR
	 */
	reg [0:17] ir;
	assign iobus_ios = ir[3:9];
	wire ir0_12_clr = mr_clr;
	wire ir13_17_clr = mr_clr | at5_D | cht8a;
	wire ir0_12_fm_mb1 = it1a;
	wire ir13_17_fm_mb1 = at0;

	always @(posedge clk) begin
		if(ir0_12_clr)
			ir[0:12] <= 0;
		if(ir13_17_clr)
			ir[13:17] <= 0;
		if(ir0_12_fm_mb1)
			ir[0:12] <= ir[0:12] | mb[0:12];
		if(ir13_17_fm_mb1)
			ir[13:17] <= ir[13:17] | mb[13:17];
		if(iot_t0a)
			ir[12] <= 1;
	end

	wire ir_uuo_a = 0;
	wire ir_fpch = 0;
	wire ir_2xx = 0;
	wire ir_accp_OR_memac = 0;
	wire ir_boole = 0;
	wire ir_hwt = 0;
	wire ir_acbm = 0;
	wire ir_iot_a = 0;

	wire ir_130 = 0;
	wire ir_131 = 0;
	wire ir_fsc = 0;
	wire ir_cao = 0;
	wire ir_ldci = 0;
	wire ir_ldc = 0;
	wire ir_dpci = 0;
	wire ir_dpc = 0;

	wire ir_fwt_mov_s = 0;
	wire ir_fwt_movn_m = 0;
	wire ir_fwt = 0;
	wire ir_mul = 0;
	wire ir_div = 0;
	wire ir_sh = 0;
	wire ir_25x = 0;
	wire ir_jp = 0;
	wire ir_as = 0;

	wire ir_ash = 0;
	wire ir_rot = 0;
	wire ir_lsh = 0;
	wire ir_243 = 0;
	wire ir_ashc = 0;
	wire ir_rotc = 0;
	wire ir_lshc = 0;
	wire ir_247 = 0;

	wire ir_exch = 0;
	wire ir_blt = 0;
	wire ir_aobjp = 0;
	wire ir_aobjn = 0;
	wire ir_jrst_a = 0;
	wire ir_jfcl = 0;
	wire ir_xct = 0;
	wire ir_257 = 0;

	wire ir_ash_OR_ashc = 0;
	wire ir_md = 0;
	wire ir_md_s_c_e = 0;
	wire ir_md_f_c_e = 0;
	wire ir_md_sac_inh = 0;
	wire ir_254_7 = 0;
	wire ir_md_f_ac_2 = 0;
	wire ir_md_s_ac_2 = 0;
	wire ir_iot = 0;
	wire ir_jrst = 0;
	wire ir_9_OR_10_1 = 0;
	wire ir_fp = 0;
	wire ir_fp_dir = 0;
	wire ir_fp_rem = 0;
	wire ir_fp_mem = 0;
	wire ir_fp_both = 0;
	wire ir_fad = 0;
	wire ir_fsb = 0;
	wire ir_fmp = 0;
	wire ir_fdv = 0;
	wire ir14_17_eq_0 = ir[14:17] == 0;

	/* ACCP V MEM AC */
	wire accp = 0;
	wire memac_tst = 0;
	wire memac_inc = 0;
	wire memac_dec = 0;
	wire memac = 0;
	wire memac_mem = 0;
	wire memac_ac = 0;
	wire accp_etc_cond = 0;
	wire accp_etal_test = 0;
	wire accp_dir = 0;

	/* ACBM */
	wire acbm_swap = 0;
	wire acbm_dir = 0;
	wire acbm_cl = 0;
	wire acbm_dn = 0;
	wire acbm_com = 0;
	wire acbm_set = 0;

	/* FWT */
	wire fwt_swap = 0;
	wire fwt_negate = 0;
	wire fwt_00 = 0;
	wire fwt_01 = 0;
	wire fwt_10 = 0;
	wire fwt_11 = 0;

	/* HWT */
	wire hwt_lt_set = 0;
	wire hwt_rt_set = 0;
	wire hwt_lt = 0;
	wire hwt_rt = 0;
	wire hwt_swap = 0;
	wire hwt_ar_clr = 0;
	wire hwt_00 = 0;
	wire hwt_01 = 0;
	wire hwt_10 = 0;
	wire hwt_11 = 0;

	/* BOOLE */
	wire boole_as_00 = 0;
	wire boole_as_01 = 0;
	wire boole_as_10 = 0;
	wire boole_as_11 = 0;
	wire boole_0 = 0;
	wire boole_1 = 0;
	wire boole_2 = 0;
	wire boole_3 = 0;
	wire boole_4 = 0;
	wire boole_5 = 0;
	wire boole_6 = 0;
	wire boole_7 = 0;
	wire boole_10 = 0;
	wire boole_11 = 0;
	wire boole_12 = 0;
	wire boole_13 = 0;
	wire boole_14 = 0;
	wire boole_15 = 0;
	wire boole_16 = 0;
	wire boole_17 = 0;

	/* JUMP V PUSH */
	wire jp_flag_stor = 0;
	wire jp_AND_NOT_jsr = 0;
	wire jp_AND_ir6_0 = 0;
	wire jp_jmp = 0;
	wire jp_pushj = 0;
	wire jp_push = 0;
	wire jp_pop = 0;
	wire jp_popj = 0;
	wire jp_jsr = 0;
	wire jp_jsp = 0;
	wire jp_jsa = 0;
	wire jp_jra = 0;

	/* AS */
	wire as_plus = 0;
	wire as_minus = 0;

	/* XCT */
	wire xct_t0 = 0;

	/*
	 * UUO
	 */
	reg uuo_f1;
	wire uuo_t1 = 0;
	wire uuo_t2 = 0;

	/*
	 * PC
	 */
	reg [18:35] pc;
	wire pc_clr = et7 & pc_set | kt1 & key_start_OR_read_in;
	wire pc_fm_ma1 = et8 & pc_set | kt3 & key_start_OR_read_in;
	wire pc_inc = 0;
	wire pc_set_OR_pc_inc = 0;
	wire pc_set = 0;
	wire pc_inc_et9 = 0;
	wire pc_inc_inh_et0 = 0;
	wire pc_inc_enable = 0;
	wire pc_set_enable = 0;

	always @(posedge clk) begin
		if(pc_clr)
			pc <= 0;
		if(pc_fm_ma1)
			pc <= pc | ma;
	end

	/*
	 * EX
	 */
	reg ex_user;
	reg ex_mode_sync;
	reg ex_uuo_sync;
	reg ex_pi_sync;
	reg ex_ill_op;
	wire ex_clr = mr_start | cpa & iobus_datao_clear;
	wire ex_set = mr_start | cpa & iobus_datao_set;
	wire ex_ir_uuo = 0;
	wire ex_inh_rel = ~ex_user | ex_pi_sync | ma18_31_eq_0 | ex_ill_op;

	always @(posedge clk) begin
		if(mr_start)
			ex_ill_op <= 0;
		if(mr_start | et7 & jp_jsr & (ex_pi_sync | ex_ill_op))
			ex_user <= 0;
		if(mr_clr) begin
			if(ex_mode_sync)
				ex_user <= 1;
			ex_mode_sync <= 0;
			ex_uuo_sync <= 0;
			ex_pi_sync <= 0;
		end
		if(pi_cyc)
			ex_pi_sync <= 1;
		if(at1)
			ex_uuo_sync <= 1;
	end

	/*
	 * MB
	 */
	reg [0:35] mb;
	wire mblt_clr = mb_clr;
	wire mblt_fm_ar0 = mb_fm_arJ | mb_ar_swap | mb_fm_ar0 | cfac_mb_ar_swap;
	wire mblt_fm_ar1 = mb_fm_arJ | mb_ar_swap | cfac_mb_ar_swap;
	wire mblt_fm_mq0 = 0;
	wire mblt_fm_mq1 = 0;
	wire mblt_fm_mbrtJ = 0;
	wire mb_fm_ir1 = 0;
	wire mbrt_clr = mb_clr;
	wire mbrt_fm_ar0 = mb_fm_arJ | mb_ar_swap | mb_fm_ar0 | cfac_mb_ar_swap;
	wire mbrt_fm_ar1 = mb_fm_arJ | mb_ar_swap | cfac_mb_ar_swap;
	wire mbrt_fm_mq0 = 0;
	wire mbrt_fm_mq1 = 0;
	wire mbrt_fm_mbltJ = 0;
	wire mb_fm_pc1 = 0;

	wire mb_clr = et1 & ex_ir_uuo |
		et5 & mb_pc_sto |
		mc_mb_clr_D;
	wire mblt_mbrt_swap = 0;
	wire mblt_mbrt_swap_et0 = 0;
	wire mblt_mbrt_swap_et1 = 0;
	wire mb_fm_misc_bits1 = 0;
	wire mb_fm_ar0 = 0;
	wire mb_fm_ar0_et1 = 0;
	wire mb_fm_arJ = at3a | st5 | key_wr | dst1 | mst1 |
		et0a & mb_fm_arJ_et0 |
		et10 & mb_fm_arJ_et10 |
		kt3 & key_execute;
	wire mb_fm_arJ_et0 = 0;
	wire mb_fm_arJ_et10 = 0;
	wire mb_fm_arJ_inh_et10 = 0;
	wire mb_ar_swap = 0;
	wire mb_ar_swap_et0 = 0;
	wire mb_ar_swap_et4 = 0;
	wire mb_ar_swap_et9 = 0;
	wire mb_ar_swap_et10 = 0;
	wire mb_fm_mqJ = 0;
	wire mb_fm_mqJ_et0 = 0;
	wire mb1_8_clr = 0;
	wire mb1_8_set = 0;
	wire mblt_fm_ir1_uuo_t0 = 0;
	wire mblt_fm_pc1_init = 0;
	wire mb_pc_sto = 0;
	wire mb_fm_pc1_et6 = 0;

	wire mc_mb_clr_D;
	dly100ns mb_dly0(.clk(clk), .reset(reset), .in(mc_mb_clr), .p(mc_mb_clr_D));

	wire membus_mb_pulse;
	pg mb_pg0(.clk(clk), .reset(reset), .in(| membus_mb_in), .p(membus_mb_pulse));


	always @(posedge clk) begin: mbctl
		integer i;
		if(mblt_clr)
			mb[0:17] <= 0;
		if(mbrt_clr)
			mb[18:35] <= 0;
		for(i = 0; i < 18; i = i+1) begin
			if(mblt_fm_ar0 & ~ar[i])
				mb[i] <= 0;
			if(mblt_fm_ar1 & ar[i])
				mb[i] <= 1;
			if(mbrt_fm_ar0 & ~ar[i+18])
				mb[i+18] <= 0;
			if(mbrt_fm_ar1 & ar[i+18])
				mb[i+18] <= 1;
		end
		if(membus_mb_pulse & mc_mb_membus_enable)
			mb <= mb | membus_mb_in;
	end

	/*
	 * AR
	 */
	reg [0:35] ar;
	reg ar_com_cont;
	reg ar_pc_chg_flg;
	reg ar_ov_flag;
	reg ar_cry0_flag;
	reg ar_cry1_flag;
	reg ar_cry0;
	reg ar_cry1;
	wire arlt_clr = ar_clr | at4 | blt_t2 | key_ar_clr;
	wire arlt_com = 0;
	wire arlt_fm_mb_xor = ar_as_t1 | ar_fm_mb_xor;
	wire arlt_fm_mb0 = 0;
	wire arlt_fm_mb1 = 0;
	wire arlt_shlt = 0;
	wire arlt_shrt = 0;
	wire arlt_fm_datasw1 = key_ar_fm_datasw1;
	wire arlt_fm_iob1 = 0;
	wire arrt_clr = ar_clr | key_ar_clr;
	wire arrt_com = 0;
	wire arrt_fm_mb_xor = ar_as_t1 | ar_fm_mb_xor;
	wire arrt_fm_mb0 = at0;	// TODO
	wire arrt_fm_mb1 = at0;	// TODO
	wire arrt_shlt = 0;
	wire arrt_shrt = 0;
	wire arrt_fm_datasw1 = key_ar_fm_datasw1;
	wire arrt_iob1 = 0;
	wire ar0_shl_inp = 0;
	wire ar0_shr_inp = 0;
	wire ar35_shl_inp = 0;
	// just one for simplicity
	wire ar_cry_initiate = ar_as_t2;

	wire shc_ashc = 0;
	wire shc_lshc_OR_div = 0;
	wire shc_div = 0;

	wire ar_clr = dst2 | fat6 | et0a & ar_clr_et0 | et1 & ar_clr_et1 |
		0;	// TODO: MST1 50ns delay
	wire ar_clr_et0 = 0;
	wire ar_clr_et1 = 0;
	wire ar_com = 0;
	wire ar_com_et0 = 0;
	wire ar_com_et4 = 0;
	wire ar_com_et5 = 0;
	wire ar_com_et7 = 0;
	wire ar_fm_mb0 = 0;
	wire ar_fm_mb0_et1 = 0;
	wire ar_fm_mb0_et6 = 0;
	wire ar_fm_mb1 = 0;
	wire ar_fm_mb1_et1 = 0;
	wire ar_fm_mb_xor = et1 & ar_fm_mb_xor_et1;
	wire ar_fm_mb_xor_et1 = boole_14 | boole_6 | boole_11 | acbm_com;
	wire ar_fm_mbJ = 0;
	wire ar_fm_mbJ_et0 = 0;
	wire ar_fm_mbltJ_et4 = 0;
	wire ar_fm_mbrtJ_et4 = 0;

	wire ar1_8_clr = 0;
	wire ar1_8_set = 0;
	wire ar_add = 0;
	wire ar_sub = 0;
	wire ar_inc = 0;
	wire ar_incdec_lt_rt = 0;
	wire ar_dec = 0;
	wire ar_sbr = 0;
	wire ar_cry_comp;
	wire ar_fm_sc1_8J = 0;
	wire ar0_5_fm_sc3_8J = 0;
	wire ar_incdec_t0 = 0;
	wire ar_negate_t0 = 0;
	wire ar_incdec_t1 = 0;
	wire ar17_cry_in = 0;
	wire ar_as_t0 = 0;
	wire ar_as_t1;
	wire ar_as_t2;
	wire ar_t3;
	wire ar_eq_fp_half = 0;
	wire ar_eq_0 = 0;
	wire ar0_xor_ar1 = 0;
	wire ar_ov_set = 0;
	wire ar_cry0_xor_cry1 = 0;
	wire ar0_xor_ar_ov = 0;
	wire ar0_xor_mb0 = 0;
	wire ar0_eq_sc0 = 0;
	wire ar_flag_clr = 0;
	wire ar_flag_set = 0;
	wire ar_jfcl_clr = 0;

	pa ar_pa0();	// AR+-1 T0
	pa ar_pa1();	// AR NEGATE T0
	pa ar_pa2();	// AR+-1 T1
	pa ar_pa3();	// AR17 CRY IN
	pa ar_pa4();	// AR AS T0
	pa ar_pa5(.clk(clk), .reset(reset),
		.in(ar_as_t0_D | et3 & ar_add |
		    at3 | cfac_ar_add),
		.p(ar_as_t1));
	pa ar_pa6(.clk(clk), .reset(reset),
		.in(ar_as_t1_D),
		.p(ar_as_t2));
	pa ar_pa7(.clk(clk), .reset(reset),
		.in(ar_as_t2 | ar_incdec_t1 | ar17_cry_in),
		.p(ar_cry_comp));
	pa ar_pa8(.clk(clk), .reset(reset),
		.in(ar_cry_comp & ~ar_com_cont |
		    ar_cry_comp_D & ar_com_cont),
		.p(ar_t3));

	wire ar_incdec_t0_D, ar_negate_t0_D;
	wire ar_as_t0_D, ar_as_t1_D, ar_cry_comp_D;
	dly100ns ar_dly0(.clk(clk), .reset(reset),
		.in(ar_incdec_t0),
		.p(ar_incdec_t0_D));
	dly100ns ar_dly1(.clk(clk), .reset(reset),
		.in(ar_negate_t0),
		.p(ar_negate_t0_D));
	dly100ns ar_dly2(.clk(clk), .reset(reset),
		.in(ar_as_t0),
		.p(ar_as_t0_D));
	dly100ns ar_dly3(.clk(clk), .reset(reset),
		.in(ar_as_t1),
		.p(ar_as_t1_D));
	dly100ns ar_dly4(.clk(clk), .reset(reset),
		.in(ar_cry_comp),
		.p(ar_cry_comp_D));

	wire [0:35] ar_mb_cry = mb & ~ar;

	always @(posedge clk) begin: arctl
		integer i;
		if(arlt_clr)
			ar[0:17] <= 0;
		if(arrt_clr)
			ar[18:35] <= 0;
		if(ar_cry_initiate)
			ar <= ar + { ar_mb_cry[1:35], 1'b0 };
		if(arlt_fm_mb_xor)
			ar[0:17] <= ar[0:17] ^ mb[0:17];
		if(arrt_fm_mb_xor)
			ar[18:35] <= ar[18:35] ^ mb[18:35];
		for(i = 0; i < 18; i = i+1) begin
			if(arlt_fm_mb0 & ~mb[i])
				ar[i] <= 0;
			if(arlt_fm_mb1 & mb[i])
				ar[i] <= 1;
			if(arrt_fm_mb0 & ~mb[i+18])
				ar[i+18] <= 0;
			if(arrt_fm_mb1 & mb[i+18])
				ar[i+18] <= 1;
		end
		if(arlt_fm_datasw1)
			ar[0:17] <= ar[0:17] | datasw[0:17];
		if(arrt_fm_datasw1)
			ar[18:35] <= ar[18:35] | datasw[18:35];

		if(mr_clr | ar_t3)
			ar_com_cont <= 0;
	end

	/*
	 * MQ
	 */
	reg [0:35] mq;
	reg mq36;
	wire mqlt_clr = 0;
	wire mqlt_fm_mb0 = 0;
	wire mqlt_fm_mb1 = 0;
	wire mqrt_clr = 0;
	wire mqrt_fm_mb0 = 0;
	wire mqrt_fm_mb1 = 0;
	wire mq_shl = 0;
	wire mq_shr = 0;

	wire mq0_shl_inp = 0;
	wire mq0_shr_inp = 0;
	wire mq1_shr_inp = 0;
	wire mq35_shl_inp = 0;

	wire mq0_clr = 0;
	wire mq0_set = 0;
	wire mq_fm_mbJ = 0;
	wire mq35_xor_mb0 = 0;
	wire mq35_eq_mq36 = 0;

	/*
	 * FE, SC
	 */
	reg [0:8] fe;
	wire fe_clr = 0;
	wire fe_fm_sc1 = 0;
	wire fe_fm_mb0_5_1 = 0;

	reg [0:8] sc;
	wire sc_clr = 0;
	wire sc_inc = 0;
	wire sc_com = 0;
	wire sc_pad = 0;
	wire sc_cry = 0;
	wire sc_fm_fe1 = 0;
	wire sc_fm_mb18_28_35_0 = 0;
	wire sc_mb0_5_0_enable = 0;
	wire sc_mb6_11_1_enable = 0;
	wire sc_ar0_8_1_enable = 0;
	wire sc_mb0_8_1_enable = 0;
	// TODO: figure out what's going on here...
	wire sc_eq_777 = 0;
	wire sc0_2_eq_7 = 0;

	wire sat0 = 0;
	wire sat1 = 0;
	wire sat2 = 0;
	wire sat21 = 0;
	wire sat3 = 0;

	wire sct0 = 0;
	wire sct1 = 0;
	wire sct2 = 0;

	/*
	 * CFAC
	 */
	wire cfac_ar_negate = 0;
	wire cfac_ar_add = 0;
	wire cfac_ar_sub = 0;
	wire cfac_mb_fm_mqJ = 0;
	wire cfac_mb_mq_swap = 0;
	wire cfac_mb_ar_swap = 0;
	wire cfac_ar_com = 0;
	wire cfac_overflow = 0;
	wire cfac_ar_sh_lt = 0;
	wire cfac_mq_sh_lt = 0;
	wire cfac_ar_sh_rt = 0;
	wire cfac_mq_sh_rt = 0;

	/*
	 * BLT
	 */
	reg blt_f0a;
	reg blt_f3a;
	reg blt_f5a;
	wire blt_done = 0;
	wire blt_last = 0;

	wire blt_t0 = 0;
	wire blt_t0a = 0;
	wire blt_t1 = 0;
	wire blt_t2 = 0;
	wire blt_t3 = 0;
	wire blt_t3a = 0;
	wire blt_t4 = 0;
	wire blt_t5 = 0;
	wire blt_t5a = 0;
	wire blt_t6 = 0;

	/*
	 * FS
	 */
	reg fsf1;
	wire fsc = 0;
	wire fst0 = 0;
	wire fst0a = 0;
	wire fst1 = 0;

	/*
	 * CH
	 */
	reg chf1;
	reg chf2;
	reg chf3;
	reg chf4;
	reg chf5;
	reg chf6;
	reg chf7;
	wire ch_inc_op = 0;
	wire ch_NOT_inc_op = 0;

	wire cht1 = 0;
	wire cht2 = 0;
	wire cht3 = 0;
	wire cht3a = 0;
	wire cht4 = 0;
	wire cht4a = 0;
	wire cht5 = 0;
	wire cht6 = 0;
	wire cht7 = 0;
	wire cht8 = 0;
	wire cht8a = 0;
	wire cht8b = 0;
	wire cht9 = 0;

	/*
	 * LC
	 */
	reg lcf1;

	wire lct0 = 0;
	wire lct0a = 0;

	/*
	 * DC
	 */
	reg dcf1;
	wire ch_dep = 0;

	wire dct0 = 0;
	wire dct0a = 0;
	wire dct0b = 0;
	wire dct1 = 0;
	wire dct2 = 0;
	wire dct3 = 0;

	/*
	 * SH
	 */
	reg shf1;
	wire shift_op = 0;
	wire sh_ac_2 = 0;

	wire sht0 = 0;
	wire sht1 = 0;
	wire sht1a = 0;

	/*
	 * MP
	 */
	reg mpf1;
	wire mp_clr = 0;

	wire mpt0 = 0;
	wire mpt0a = 0;
	wire mpt1 = 0;
	wire mpt2 = 0;

	/*
	 * FA
	 */
	reg faf1;
	reg faf2;
	reg faf3;
	reg faf4;

	wire fat0 = 0;
	wire fat1 = 0;
	wire fat1a = 0;
	wire fat1b = 0;
	wire fat2 = 0;
	wire fat3 = 0;
	wire fat4 = 0;
	wire fat5 = 0;
	wire fat5a = 0;
	wire fat6 = 0;
	wire fat7 = 0;
	wire fat8 = 0;
	wire fat8a = 0;
	wire fat9 = 0;
	wire fat10 = 0;

	/*
	 * FM
	 */
	reg fmf1;
	reg fmf2;

	wire fmt0 = 0;
	wire fmt0a = 0;
	wire fmt0b = 0;

	/*
	 * FD
	 */
	reg fdf1;
	reg fdf2;

	wire fdt0 = 0;
	wire fdt0a = 0;
	wire fdt0b = 0;
	wire fdt1 = 0;

	/*
	 * FP
	 */
	reg fpf1;
	reg fpf2;
	wire fp_ar0_xor_fmf1 = 0;
	wire fp_ar0_xor_mb0_xor_fmf1 = 0;
	wire fp_mb0_eq_fmf1 = 0;

	wire fpt0 = 0;
	wire fpt01 = 0;
	wire fpt1 = 0;
	wire fpt1a = 0;
	wire fpt1aa = 0;
	wire fpt1b = 0;
	wire fpt2 = 0;
	wire fpt3 = 0;
	wire fpt4 = 0;

	/*
	 * MS
	 */
	reg msf1;
	wire ms_mult = 0;

	wire mst1 = 0;
	wire mst2 = 0;
	wire mst3 = 0;
	wire mst3a = 0;
	wire mst4 = 0;
	wire mst5 = 0;
	wire mst6 = 0;

	/*
	 * DS
	 */
	reg dsf1;
	reg dsf2;
	reg dsf3;
	reg dsf4;
	reg dsf5;
	reg dsf6;
	reg dsf7;
	reg dsf8;
	reg dsf9;
	wire ds_div = 0;
	wire ds_divi = 0;
	wire ds_clr = 0;
	wire dsf7_xor_mq0 = 0;

	wire dst0 = 0;
	wire dst0a = 0;
	wire dst1 = 0;
	wire dst2 = 0;
	wire dst3 = 0;
	wire dst4 = 0;
	wire dst5 = 0;
	wire dst6 = 0;
	wire dst7 = 0;
	wire dst8 = 0;
	wire dst9 = 0;
	wire dst10 = 0;
	wire dst10a = 0;
	wire dst10b = 0;
	wire dst11 = 0;
	wire dst11a = 0;
	wire dst12 = 0;
	wire dst13 = 0;
	wire dst14 = 0;
	wire dst14a = 0;
	wire dst14b = 0;
	wire dst15 = 0;
	wire dst16 = 0;
	wire dst17 = 0;
	wire dst17a = 0;
	wire dst18 = 0;
	wire dst19 = 0;
	wire dst20 = 0;
	wire dst21 = 0;
	wire dst21a = 0;

	wire ds_div_t0 = 0;

	/*
	 * NR
	 */
	reg nrf1;
	reg nrf2;
	reg nrf3;
	wire nr_ar9_eq_ar0 = 0;
	wire nr_round = 0;
	wire nr_ar_eq_0_AND_mq1_0;

	wire nrt05 = 0;
	wire nrt0 = 0;
	wire nrt01 = 0;
	wire nrt1 = 0;
	wire nrt2 = 0;
	wire nrt3 = 0;
	wire nrt31 = 0;
	wire nrt4 = 0;
	wire nrt5 = 0;
	wire nrt5a = 0;
	wire nrt6 = 0;

	/*
	 * MA
	 */
	reg [18:35] ma;
	reg ma32_cry_out;
	wire ma_clr = it0 | at0 | at3 | ft4a | key_ma_clr | iot_t0a |
		et1 & ma_clr_et1 |
		et9 & ma_reset_et9 |
		st3 & ~s_ac_inh |
		ft1a & ~f_ac_2;
	wire ma_inc = st6 | key_ma_inc | uuo_t1 |
		ft1a & f_ac_2 |
		it1 & pi_ov;
	wire ma_fm_mbrt1 = at5 | ft3 | ft5 |
		et3 & ma_fm_mbrt_et3 | et10 & ma_reset_et9;
	wire ma_fm_pc1 = it1 & ~pi_cyc;
	wire ma31_cry_in_en = ~s_ac_2 & ~f_ac_2;
	wire [30:35] maN_set;

	wire ma_eq_mas = ma == mas;
	wire ma18_31_eq_0 = ma[18:31] == 0;
	wire ma_fmc_select = ~key_rim_sbr & ma18_31_eq_0;
	wire ma_fm_mbrt_et3 = 0;
	wire ma_clr_et1 = 0;
	wire ma_reset_et9 = 0;
	wire ma_fm_ir14_17 = at2;
	wire ma_fm_ir9_12 = 0;
	wire ma_fm_pich = it1 & pi_cyc;

	assign maN_set[30] = ma_fm_pich | et3 & ex_ir_uuo;
	assign maN_set[31] = 0;
	// TODO
	assign maN_set[32] = pi_enc_32 | ma_fm_ir14_17 & ir[14];
	assign maN_set[33] = pi_enc_33 | ma_fm_ir14_17 & ir[15];
	assign maN_set[34] = pi_enc_34 | ma_fm_ir14_17 & ir[16];
	assign maN_set[35] = ma_fm_ir14_17 & ir[17];

	always @(posedge clk) begin
		if(ma_clr)
			ma <= 0;
		if(ma_inc) begin
			{ma32_cry_out, ma[32:35]} = ma[32:35]+1;
			ma[18:31] = ma[18:31] + (ma32_cry_out & ma31_cry_in_en);
		end
		if(ma_fm_mbrt1)
			ma <= ma | mb[18:35];
		if(ma_fm_pc1)
			ma <= ma | pc;
		if(key_ma_fm_masw1)
			ma <= ma | mas;
		if(maN_set[30]) ma[30] <= 1;
		if(maN_set[32]) ma[32] <= 1;
		if(maN_set[33]) ma[33] <= 1;
		if(maN_set[34]) ma[34] <= 1;
		if(maN_set[35]) ma[35] <= 1;
	end

	/*
	 * PR
	 */
	reg [18:25] pr;
	wire pr18_ok = ma[18:25] <= pr;
	wire pr_rel_AND_ma_ok = ~ex_inh_rel & pr18_ok;
	wire pr_rel_AND_NOT_ma_ok = ~ex_inh_rel & ~pr18_ok;

	/*
	 * RLR, RLA
	 */
	reg [18:25] rlr;
	wire [18:25] rla = ma[18:25] + (ex_inh_rel ? 0 : rlr);

	always @(posedge clk) begin
		if(ex_clr) begin
			pr <= 0;
			rlr <= 0;
		end
	end

	/*
	 * MI
	 */
	reg [0:35] mi;
	wire milt_clr = 0;
	wire milt_fm_mblt1 = 0;
	wire mirt_clr = 0;
	wire mirt_fm_mbrt1 = 0;

	/*
	 * MC
	 */
	reg mc_rd;
	reg mc_wr;
	reg mc_rq;
	reg mc_stop;
	reg mc_stop_sync;
	reg mc_split_cyc_sync;
	wire mc_sw_stop = key_mem_stop | sw_addr_stop;
	wire mc_rd_rq_pulse;
	wire mc_wr_rq_pulse;
	wire mc_rdwr_rq_pulse;
	wire mc_rq_pulse;
	wire mc_rdwr_rs_pulse;
	wire mc_split_rd_rq;
	wire mc_split_wr_rq;
	wire mc_mb_clr;
	wire mc_rs_t0;
	wire mc_rs_t1;
	wire mc_wr_rs;
	wire mai_addr_ack;
	wire mai_rd_rs;
	wire mc_addr_ack;
	wire mc_non_exist_mem;
	wire mc_non_exist_mem_rst;
	wire mc_non_exist_rd;
	wire mc_illeg_address;
	wire mc_rq_set;
	wire mc_stop_set = mc_rq_pulse_D2 &
		(key_mem_stop | sw_addr_stop & ma_eq_mas);

	wire mc_membus_fm_mb1;
	wire mc_mb_membus_enable = mc_rd;

	pg mc_pg0(.clk(clk), .reset(reset),
		.in(membus_addr_ack), .p(mai_addr_ack));
	pg mc_pg1(.clk(clk), .reset(reset),
		.in(membus_rd_rs), .p(mai_rd_rs));

	pa mc_pa0(.clk(clk), .reset(reset),
		.in(it1 | at2 | at5 |
		    ft1 | ft4 | ft6 |
		    key_rd | uuo_t2 | mc_split_rd_rq),
		.p(mc_rd_rq_pulse));
	pa mc_pa1(.clk(clk), .reset(reset),
		.in(st1 | st5 | st6 | key_wr |
		    mblt_fm_ir1_uuo_t0 | mc_split_wr_rq | blt_t0),
		.p(mc_wr_rq_pulse));
	pa mc_pa2(.clk(clk), .reset(reset),
		.in(ft7 & ~mc_split_cyc_sync),
		.p(mc_rdwr_rq_pulse));
	pa mc_pa3(.clk(clk), .reset(reset),
		.in(mc_rdwr_rq_pulse | mc_rd_rq_pulse | mc_wr_rq_pulse),
		.p(mc_rq_pulse));
	pa mc_pa4(.clk(clk), .reset(reset),
		.in(st2 | iot_t0 | cht8),
		.p(mc_rdwr_rs_pulse));
	pa mc_pa5(.clk(clk), .reset(reset),
		.in(ft7 & mc_split_cyc_sync),
		.p(mc_split_rd_rq));
	pa mc_pa6(.clk(clk), .reset(reset),
		.in(mc_rdwr_rs_pulse & mc_split_cyc_sync),
		.p(mc_split_wr_rq));
	pa mc_pa7(.clk(clk), .reset(reset),
		.in(mc_rd_rq_pulse | mc_rdwr_rq_pulse),
		.p(mc_mb_clr));
	pa mc_pa8(.clk(clk), .reset(reset),
		.in(mc_rq_pulse_D3 & mc_rq & ~mc_stop),
		.p(mc_non_exist_mem));
	pa mc_pa9(.clk(clk), .reset(reset),
		.in(mc_non_exist_mem & ~sw_mem_disable),
		.p(mc_non_exist_mem_rst));
	pa mc_pa10(.clk(clk), .reset(reset),
		.in(mc_non_exist_mem_rst & mc_rd),
		.p(mc_non_exist_rd));
	pa mc_pa11(.clk(clk), .reset(reset),
		.in(mc_rq_pulse_D0 & ex_inh_rel |
		    mc_rq_pulse_D1 & pr_rel_AND_ma_ok),
		.p(mc_rq_set));
	pa mc_pa12(.clk(clk), .reset(reset),
		.in(mc_rq_pulse_D1 & pr_rel_AND_NOT_ma_ok),
		.p(mc_illeg_address));
	pa mc_pa13(.clk(clk), .reset(reset),
		.in(mai_addr_ack | mc_non_exist_mem_rst),
		.p(mc_addr_ack));
	pa mc_pa14(.clk(clk), .reset(reset),
		.in(mc_addr_ack & ~mc_rd & mc_wr |
		    mc_rdwr_rs_pulse_D & ~mc_split_cyc_sync |
		    kt1 & key_manual & mc_stop & mc_stop_sync & ~key_mem_cont),
		.p(mc_wr_rs));
	pa mc_pa15(.clk(clk), .reset(reset),
		.in(kt1 & key_mem_cont & mc_stop |
		    ~mc_stop & (mc_wr_rs | mai_rd_rs | mc_non_exist_rd)),
		.p(mc_rs_t0));
	pa mc_pa16(.clk(clk), .reset(reset), .in(mc_rs_t0_D), .p(mc_rs_t1));

	bd  mc_bd0(.clk(clk), .reset(reset), .in(mc_wr_rs), .p(membus_wr_rs));
	bd2 mb_bd1(.clk(clk), .reset(reset), .in(mc_wr_rs), .p(mc_membus_fm_mb1));

	wire mc_rdwr_rs_pulse_D, mc_rs_t0_D;
	wire mc_rq_pulse_D0, mc_rq_pulse_D1, mc_rq_pulse_D2, mc_rq_pulse_D3;
	dly100ns mc_dly0(.clk(clk), .reset(reset),
		.in(mc_rdwr_rs_pulse),
		.p(mc_rdwr_rs_pulse_D));
	dly50ns mc_dly1(.clk(clk), .reset(reset),
		.in(mc_rq_pulse),
		.p(mc_rq_pulse_D0));
	dly150ns mc_dly2(.clk(clk), .reset(reset),
		.in(mc_rq_pulse),
		.p(mc_rq_pulse_D1));
	dly200ns mc_dly3(.clk(clk), .reset(reset),
		.in(mc_rq_pulse),
		.p(mc_rq_pulse_D2));
	dly100us mc_dly4(.clk(clk), .reset(reset), .in(mc_rq_pulse),
		.p(mc_rq_pulse_D3));
	dly50ns mc_dly5(.clk(clk), .reset(reset), .in(mc_rs_t0),
		.p(mc_rs_t0_D));

	assign membus_rq_cyc = mc_rq & (mc_rd | mc_wr);
	assign membus_wr_rq = mc_wr;
	assign membus_rd_rq = mc_rd;
	assign membus_ma = { rla[21:25], ma[26:35] };
	assign membus_sel = rla[18:21];
	assign membus_fmc_select = ma_fmc_select;
	assign membus_mb_out = mc_membus_fm_mb1 ? mb : 0;

	always @(posedge clk) begin
		if(mr_clr) begin
			mc_rd <= 0;
			mc_wr <= 0;
			mc_rq <= 0;
			mc_stop <= 0;
			mc_stop_sync <= 0;
			mc_split_cyc_sync <= 0;
		end
		if(mc_wr_rq_pulse | mc_rs_t1)
			mc_rd <= 0;
		if(mc_rd_rq_pulse | mc_rdwr_rq_pulse)
			mc_rd <= 1;
		if(mc_rd_rq_pulse)
			mc_wr <= 0;
		if(mc_wr_rq_pulse | mc_rdwr_rq_pulse)
			mc_wr <= 1;
		if(mc_addr_ack)
			mc_rq <= 0;
		if(mc_rq_set)
			mc_rq <= 1;
		if(mc_rq_pulse)
			mc_stop <= 0;
		if(mc_stop_set)
			mc_stop <= 1;
		if(mc_rdwr_rq_pulse)
			mc_stop_sync <= 1;
		if(at4 & mc_sw_stop)
			mc_split_cyc_sync <= 1;
	end

	/*
	 * IOT
	 */
	reg iot_go;
	reg iot_f0a;

	wire iot_blki = 0;
	wire iot_datai = 0;
	wire iot_blko = 0;
	wire iot_datao = 0;
	wire iot_cono = 0;
	wire iot_coni = 0;
	wire iot_consz = 0;
	wire iot_conso = 0;

	wire iot_blk = 0;
	wire iot_outgoing = 0;
	wire iot_status = 0;
	wire iot_datai_o = 0;
	wire iot_init_setup = 0;
	wire iot_final_setup = 0;

	wire iot_t0 = 0;
	wire iot_t0a = 0;
	wire iot_t2 = 0;
	wire iot_t3 = 0;
	wire iot_t3a = 0;
	wire iot_t4 = 0;

	wire iot_t0a_D;
	dly200ns iot_dly0(.clk(clk), .reset(reset),
		.in(iot_t0a),
		.p(iot_t0a_D));

	/* IOB */
	assign iobus_iob_poweron = 0;
	assign iobus_iob_reset = 0;
	assign iobus_datao_clear = 0;
	assign iobus_datao_set = 0;
	assign iobus_cono_clear = 0;
	assign iobus_cono_set = 0;
	assign iobus_iob_fm_datai = 0;
	assign iobus_iob_fm_status = 0;
	assign iobus_iob_out = 0;
/*
	wire [1:7]  iobus_pi_req;
*/

	/*
	 * PIH, PIR, PIO
	 */
	// pih contains the currently serviced pi reqs.
	// lower channels override higher ones.
	reg [1:7] pih;
	wire pih_clr = pi_reset;
	wire pih_fm_pi_ch_rq = 0;
	wire pih0_fm_pi_ok1 = 0;

	// pir contains all current and allowed pi reqs.
	reg [1:7] pir;
	wire pir_clr = pi_reset;
	wire pir_fm_iob1 = 0;
	wire pir_stb;

	// pio is a mask of which pi reqs are allowed.
	reg [1:7] pio;
	wire pio_fm_iob1 = 0;
	wire pio0_fm_iob1 = 0;

	// pi_req has the currently highest priority request.
	wire [1:7] pi_req;
	// pi_ok is used to mask out low priority reqs
	wire [1:8] pi_ok;

	// requests coming from the bust
	wire [1:7] iob_pi_req = iobus_pi_req;	// TODO: apr reqs

	genvar i;
	assign pi_ok[1] = pi_active;
	for(i = 1; i <= 7; i = i + 1) begin
		assign pi_req[i] =  pi_ok[i] & ~pih[i] &  pir[i];
		assign pi_ok[i+1] = pi_ok[i] & ~pih[i] & ~pir[i];
	end

	always @(posedge clk) begin: pirctl
		integer i;
		if(pih_clr)
			pih <= 0;
		if(pir_clr)
			pir <= 0;
		if(pir_stb)
			for(i = 1; i <= 7; i = i+1) begin
				if(iob_pi_req[i] & pio[i])
					pir[i] <= 1;
			end
		if(pi_reset)
			pio <= 0;
	end

	/*
	 * PI
	 */
	reg pi_ov;
	reg pi_cyc;
	reg pi_active;
	wire pi_select = 0;
	wire pi_status = 0;
	wire pi_cono_set = 0;
	wire pi_rq = | pi_req;
	wire pi_enc_32 = pi_req[4] | pi_req[5] | pi_req[6] | pi_req[7];
	wire pi_enc_33 = pi_req[2] | pi_req[3] | pi_req[6] | pi_req[7];
	wire pi_enc_34 = pi_req[1] | pi_req[3] | pi_req[5] | pi_req[7];
	wire pi_blk_rst = ~pi_ov & iot_datai_o;
	wire pi_hold = pi_cyc & (~ir_iot | pi_blk_rst);
	wire pi_rst = (ir_jrst & ir[9]) | (pi_cyc & pi_blk_rst);
	wire pi_sync;
	wire pi_reset;

	pa pi_pa0(.clk(clk), .reset(reset),
		.in(it0 | at0),
		.p(pi_sync));
	pa pi_pa1(.clk(clk), .reset(reset),
		.in(pi_sync & ~pi_cyc | blt_t4),
		.p(pir_stb));
	pa pi_pa2(.clk(clk), .reset(reset),
		.in(mr_start | 1'b0),	// TODO
		.p(pi_reset));

	wire pi_sync_D;
	dly200ns pi_dly0(.clk(clk), .reset(reset),
		.in(pi_sync),
		.p(pi_sync_D));

	always @(posedge clk) begin
		if(mr_start | et10 & pi_hold) begin
			pi_ov <= 0;
			pi_cyc <= 0;
		end
		if(iot_t0a & ar_cry0 & pi_cyc)
			pi_ov <= 1;
		if(iat0_D0)
			pi_cyc <= 1;
		if(pi_reset | pi_cono_set & iobus_iob_in[27])
			pi_active <= 0;
		if(pi_cono_set & iobus_iob_in[28])
			pi_active <= 1;
	end

	/*
	 * CPA
	 */
	reg cpa_iot_user;
	reg cpa_illeg_op;
	reg cpa_non_exist_mem;
	reg cpa_clock_enable;
	reg cpa_clock_flag;
	reg cpa_pc_chg_enable;
	reg cpa_pdl_ov;
	reg cpa_arov_enable;
	reg [33:35] cpa_pia;
	wire cpa = iobus_ios == 0;
	wire cpa_cono_set = 0;
	wire cpa_status = 0;

endmodule
