// two's complement of long dividend isn't taken correctly
`define FIX_DS
// USER IOT isn't implemented fully
`define FIX_USER_IOT
// A LONG flip-flop is missing
`define FIX_A_LONG
// guard against firing MC RS T0 twice on
// MEM CONT when RUN is 0. Although clearing
// the SBR FF probably prevents the same after MC RS T1
`define FIX_MEMSTOP


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
	output wire power,
	output reg [0:17] ir,
	output reg [0:35] mi,
	output reg [0:35] ar,
	output reg [0:35] mb,
	output reg [0:35] mq,
	output reg [18:35] pc,
	output reg [18:35] ma,
	output reg run,
	output reg mc_stop,
	output reg pi_active,
	output reg [1:7] pih,
	output reg [1:7] pir,
	output reg [1:7] pio,
	output reg [18:25] pr,
	output reg [18:25] rlr,
	output wire [18:25] rla,
	output wire [0:7] ff0,
	output wire [0:7] ff1,
	output wire [0:7] ff2,
	output wire [0:7] ff3,
	output wire [0:7] ff4,
	output wire [0:7] ff5,
	output wire [0:7] ff6,
	output wire [0:7] ff7,
	output wire [0:7] ff8,
	output wire [0:7] ff9,
	output wire [0:7] ff10,
	output wire [0:7] ff11,
	output wire [0:7] ff12,
	output wire [0:7] ff13,

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

`ifdef simulation
	assign power = sw_power;
`else
	assign power = ~key_pwr_clr_enbl & ~reset & sw_power;
`endif
	wire rst = reset | ~sw_power;


	assign ff0 = { key_ex_st, key_ex_sync, key_dep_st, key_dep_sync, key_rdwr, mc_rd, mc_wr, mc_rq  };
	assign ff1 = { if1a, af0, af3, af3a, et4_ar_pse, f1a, f4a, f6a };
	assign ff2 = { sf3, sf5a, sf7, ar_com_cont, blt_f0a, blt_f3a, blt_f5a, iot_f0a };
	assign ff3 = { fpf1, fpf2, faf1, faf2, faf3, faf4, fmf1, fmf2 };
	assign ff4 = { fdf1, fdf2, nr_round, nrf1, nrf2, nrf3, fsf1, chf7 };
	assign ff5 = { dsf1, dsf2, dsf3, dsf4, dsf5, dsf6, dsf7, dsf8 };
	assign ff6 = { dsf9, msf1, mpf1, mpf2, mc_split_cyc_sync, mc_stop_sync, shf1, sc_eq_777 };
	assign ff7 = { chf1, chf2, chf3, chf4, chf5, chf6, lcf1, dcf1 };
	assign ff8 = { pi_ov, pi_cyc, pi_rq, iot_go, a_long, ma_eq_mas, uuo_f1, cpa_pdl_ov };
	assign ff9 = fe[1:8];
	assign ff10 = sc[1:8];
	assign ff11 = { ~ex_user, cpa_illeg_op, ex_ill_op, ex_uuo_sync, ex_pi_sync, mq36, sc[0], fe[0] };
	assign ff12 = { key_rim_sbr, ar_cry0_xor_cry1, ar_cry0, ar_cry1, ar_ov_flag, ar_cry0_flag, ar_cry1_flag, ar_pc_chg_flag };
	assign ff13 = { cpa_non_exist_mem, cpa_clock_enable, cpa_clock_flag, cpa_pc_chg_enable, cpa_arov_enable, cpa_pia[33:35] };

	/*
	 * KEY
	 */
	// TODO: repeat
	// reg run;
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
	wire key_ar_fm_datasw1;
	wire key_rd = kt3 & key_ex_OR_ex_nxt;
	wire key_wr = kt3 & key_dp_OR_dp_nxt;

	wire kt0a_D, kt1_D, kt2_D, kt3_D;
`ifdef simulation
	pg key_pg0(.clk(clk), .reset(reset), .in(sw_power), .p(mr_pwr_clr));
`else
	wire sw_power_pulse;
	wire key_pwr_clr_enbl;
	pg key_pg7(clk, reset, sw_power, sw_power_pulse);
	ldly5s key_dly4(.clk(clk), .reset(reset), .in(sw_power_pulse), .l(key_pwr_clr_enbl));
	clk25khz key_clk0(clk, key_pwr_clr_enbl, mr_pwr_clr);
`endif
	pg key_pg1(.clk(clk), .reset(rst), .in(key_inst_stop), .p(run_clr));
	pg key_pg2(.clk(clk), .reset(rst), .in(key_manual), .p(kt0));
	pg key_pg3(.clk(clk), .reset(rst),
		.in(kt2 & key_execute_OR_dp_OR_dp_nxt |
		    cpa & iobus_iob_fm_datai),
		.p(key_ar_fm_datasw1));

	pa key_pa0(.clk(clk), .reset(rst), .in(kt0), .p(kt0a));
	pa key_pa1(.clk(clk), .reset(rst),
		.in(kt0a_D & ~run |
		    kt0a & key_mem_cont |
		    st7 & run & key_ex_OR_dep_st),
		.p(kt1));
	pa key_pa2(.clk(clk), .reset(rst), .in(kt1_D), .p(kt2));
	pa key_pa3(.clk(clk), .reset(rst), .in(kt2_D), .p(kt3));
	pa key_pa4(.clk(clk), .reset(rst),
		.in(kt3 & key_execute |
		    key_rdwr_ret |
		    mc_stop_set & key_mem_cont |
		    st7 & key_start_OR_cont_OR_read_in),
		.p(kt4));
	pa key_pa5(.clk(clk), .reset(rst),
		.in(kt3 & key_start_OR_cont_OR_read_in |
		    key_run_AND_ex_OR_dep),
		.p(key_go));
	pa key_pa6(.clk(clk), .reset(rst),
		.in(key_rdwr & mc_rs_t1),
		.p(key_rdwr_ret));

	dly100ns key_dly0(.clk(clk), .reset(rst), .in(kt0a), .p(kt0a_D));
	dly200ns key_dly1(.clk(clk), .reset(rst), .in(kt1), .p(kt1_D));
	dly200ns key_dly2(.clk(clk), .reset(rst), .in(kt2), .p(kt2_D));
	dly100ns key_dly3(.clk(clk), .reset(rst), .in(kt3), .p(kt3_D));

`ifdef simulation
	/* add to this as needed */
	always @(posedge reset) begin
		run <= 0;
		key_ex_st <= 0;
		key_dep_st <= 0;
		ar <= 0;
		mb <= 0;
		mq <= 0;
	end
`endif

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

	pa i_pa0(.clk(clk), .reset(rst),
		.in(key_go | (st7 & key_run_AND_NOT_ex_OR_dep)),
		.p(it0));
	pa i_pa1(.clk(clk), .reset(rst),
		.in(pi_sync_D & pi_rq & ~pi_cyc),
		.p(iat0));
	pa i_pa2(.clk(clk), .reset(rst),
		.in(iat0_D1 |
		    pi_sync_D & if1a & ia_NOT_int),
		.p(it1));
	pa i_pa3(.clk(clk), .reset(rst),
		.in(mc_rs_t1 & if1a |
		    kt3_D & key_execute |
		    xct_t0_D),
		.p(it1a));

	wire it0_D, iat0_D0, iat0_D1;
`ifdef simulation
	ldly2us i_dly0(.clk(clk), .reset(rst),
		.in(run_clr),
		.l(at1_inh));
`else
	ldly100us i_dly0(.clk(clk), .reset(rst),
		.in(run_clr),
		.l(at1_inh));
`endif
	dly50ns i_dly1(.clk(clk), .reset(rst),
		.in(it0),
		.p(it0_D));
	dly100ns i_dly2(.clk(clk), .reset(rst),
		.in(iat0),
		.p(iat0_D0));
	dly200ns i_dly23(.clk(clk), .reset(rst),
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

	reg a_long = 1'b0;

	pa a_pa0(.clk(clk), .reset(rst),
		.in(it1a | cht9 | mc_rs_t1 & af0),
		.p(at0));
	pa a_pa1(.clk(clk), .reset(rst),
		.in(pi_sync_D & ~if1a & ia_NOT_int),
		.p(at1));
	pa a_pa2(.clk(clk), .reset(rst),
		.in(at1 & ~ir14_17_eq_0),
		.p(at2));
	pa a_pa3(.clk(clk), .reset(rst),
		.in(mc_rs_t1 & af3),
		.p(at3));
	pa a_pa4(.clk(clk), .reset(rst),
		.in(ar_t3 & af3a),
		.p(at3a));
	pa a_pa5(.clk(clk), .reset(rst),
		.in(at1 & ir14_17_eq_0 | at3a_D),
		.p(at4));
	pa a_pa6(.clk(clk), .reset(rst),
		.in(at4 & ir[13]),
		.p(at5));

	wire at3a_D, at5_D;
	dly100ns a_dly0(.clk(clk), .reset(rst),
		.in(at3a),
		.p(at3a_D));
	dly50ns a_dly1(.clk(clk), .reset(rst),
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

`ifdef FIX_A_LONG
		if(mr_clr)
			a_long <= 0;
		if(at2 | at5)
			a_long <= 1;
`endif
	end

	/*
	 * F
	 */
	reg f1a;
	reg f4a;
	reg f6a;
	wire ft0;
	wire ft1;
	wire ft1a;
	wire ft3;
	wire ft4;
	wire ft4a;
	wire ft5;
	wire ft6;
	wire ft6a;
	wire ft7;
	wire f_c_c_aclt = jp_jra | ir_blt;
	wire f_c_c_acrt = jp_pop | jp_popj;
	wire f_ac_2 = sh_ac_2 | ir_md_f_ac_2;
	wire f_c_c_aclt_OR_rt = f_c_c_aclt | f_c_c_acrt;
	wire f_ac_2_etc = f_c_c_aclt_OR_rt | f_ac_2;
	wire f_ac_inh = hwt_11 | fwt_00 | fwt_01 | fwt_11 |
		ir_xct | ex_ir_uuo | jp_jsp | jp_jsr |
		ir_iot | ir_254_7 | memac_mem |
		ch_load | ch_inc_op | ch_NOT_inc_op;
	wire f_c_e = hwt_00 | fwt_00 | ir_xct | jp_push |
		iot_datao | ir_fp | ir_md_f_c_e | ch_load |
		ch_NOT_inc_op | accp_dir | acbm_dir | boole_as_00;
	wire f_c_e_pse = hwt_10 | hwt_11 | fwt_11 | iot_blk |
		ir_exch | ch_dep | ch_inc_op | memac_mem |
		boole_as_10 | boole_as_11;
	wire f_c_e_OR_pse = f_c_e | f_c_e_pse;

	pa f_pa0(.clk(clk), .reset(rst),
		.in(at4 & ~ir[13] | iot_t0a_D),
		.p(ft0));
	pa f_pa1(.clk(clk), .reset(rst),
		.in(ft0 & ~f_ac_inh),
		.p(ft1));
	pa f_pa2(.clk(clk), .reset(rst),
		.in(f1a & mc_rs_t1 | blt_t6_D),
		.p(ft1a));
	pa f_pa4(.clk(clk), .reset(rst),
		.in(ft1a_D & f_c_c_aclt_OR_rt),
		.p(ft3));
	pa f_pa5(.clk(clk), .reset(rst),
		.in(ft3_D | ft1a_D & f_ac_2),
		.p(ft4));
	pa f_pa6(.clk(clk), .reset(rst),
		.in(f4a & mc_rs_t1),
		.p(ft4a));
	pa f_pa7(.clk(clk), .reset(rst),
		.in(ft0 & f_ac_inh |
		    ft1a_D & ~f_ac_2_etc |
		    ft4a_D),
		.p(ft5));
	pa f_pa8(.clk(clk), .reset(rst),
		.in(ft5 & f_c_e),
		.p(ft6));
	pa f_pa9(.clk(clk), .reset(rst),
		.in(ft5 & f_c_e_pse),
		.p(ft7));
	pa f_pa10(.clk(clk), .reset(rst),
		.in(f6a & mc_rs_t1 |
		    ft5 & ~f_c_e_OR_pse),
		.p(ft6a));

	wire ft1a_D, ft3_D, ft4a_D;
	dly100ns f_dly0(.clk(clk), .reset(rst),
		.in(ft1a),
		.p(ft1a_D));
	dly100ns f_dly1(.clk(clk), .reset(rst),
		.in(ft3),
		.p(ft3_D));
	dly100ns f_dly2(.clk(clk), .reset(rst),
		.in(ft4a),
		.p(ft4a_D));

	always @(posedge clk) begin
		if(mr_clr | ft1a)
			f1a <= 0;
		if(ft1)
			f1a <= 1;
		if(mr_clr | ft4a)
			f4a <= 0;
		if(ft4)
			f4a <= 1;
		if(mr_clr | ft6a)
			f6a <= 0;
		if(ft6 | ft7)
			f6a <= 1;
	end

	/*
	 * E
	 */
	reg et4_ar_pse;
	wire et0a;
	wire et0;
	wire et1;
	wire et3;
	wire et4;
	wire et5;
	wire et6;
	wire et7;
	wire et8;
	wire et9;
	wire et10;
	wire et4_inh = ir_blt | ir_xct | ex_ir_uuo |
		shift_op | ar_sbr | ir_md | ir_fpch;
	wire et5_inh = ir_iot | ir_fsb;
	wire e_long = iot_consz | ir_jp | ir_acbm | pc_set |
		mb_pc_sto | pc_inc_et9 | iot_conso | ir_accp_OR_memac;

	pa e_pa0(.clk(clk), .reset(rst),
		.in(ft6a),
		.p(et0a));
	pa e_pa1(.clk(clk), .reset(rst),
		.in(ft6a),
		.p(et0));
	pa e_pa2(.clk(clk), .reset(rst),
		.in(ft6a_D),
		.p(et1));
	pa e_pa3(.clk(clk), .reset(rst),
		.in(et1_D),
		.p(et3));
	pa e_pa4(.clk(clk), .reset(rst),
		.in(et3 & ~et4_inh |
		    ar_t3 & et4_ar_pse),
		.p(et4));
	pa e_pa5(.clk(clk), .reset(rst),
		.in(et4_D & ~et5_inh | iot_t3_D),
		.p(et5));
	pa e_pa6(.clk(clk), .reset(rst),
		.in(et5_D & e_long),
		.p(et6));
	pa e_pa7(.clk(clk), .reset(rst),
		.in(et6_D),
		.p(et7));
	pa e_pa8(.clk(clk), .reset(rst),
		.in(et7_D),
		.p(et8));
	pa e_pa9(.clk(clk), .reset(rst),
		.in(et8_D | dst21a & ir_div),
		.p(et9));
	pa e_pa10(.clk(clk), .reset(rst),
		.in(et9_D | et5 & ~e_long |
		    lct0a | dct3 |
		    nrt6 | fst0a | sht1a |
		    blt_t5a & blt_done),
		.p(et10));

	wire ft6a_D, et1_D, et4_D, et5_D;
	wire et6_D, et7_D, et8_D, et9_D;
	wire iot_t3_D;
	dly100ns e_dly0(.clk(clk), .reset(rst),
		.in(ft6a),
		.p(ft6a_D));
	dly100ns e_dly1(.clk(clk), .reset(rst),
		.in(et1),
		.p(et1_D));
	dly200ns e_dly2(.clk(clk), .reset(rst),
		.in(iot_t3),
		.p(iot_t3_D));
	dly100ns e_dly3(.clk(clk), .reset(rst),
		.in(et4),
		.p(et4_D));
	dly100ns e_dly4(.clk(clk), .reset(rst),
		.in(et5),
		.p(et5_D));
	dly100ns e_dly5(.clk(clk), .reset(rst),
		.in(et6),
		.p(et6_D));
	dly100ns e_dly6(.clk(clk), .reset(rst),
		.in(et7),
		.p(et7_D));
	dly200ns e_dly7(.clk(clk), .reset(rst),
		.in(et8),
		.p(et8_D));
	dly200ns e_dly8(.clk(clk), .reset(rst),
		.in(et9),
		.p(et9_D));

	always @(posedge clk) begin
		if(et3 & ar_sbr)
			et4_ar_pse <= 1;
		if(mr_clr | et4)
			et4_ar_pse <= 0;
	end

	/*
	 * S
	 */
	reg sf3;
	reg sf5a;
	reg sf7;
	wire st1;
	wire st2;
	wire st3;
	wire st3a;
	wire st5;
	wire st5a;
	wire st6;
	wire st6a;
	wire st7;
	wire s_c_e = jp_pushj | jp_push | jp_pop | jp_jsr | jp_jsa |
		iot_datai | iot_coni | fwt_10 | ir_md_s_c_e |
		ir_fp_mem | ir_fp_both;
	wire s_ac_inh_if_ac_0 = s_ac_0 & (fwt_11 | hwt_11 | memac_mem);
	wire s_ac_inh = boole_as_10 | hwt_10 | blt_last |
		accp | jp_jsr | fwt_10 |
		acbm_dn | ir_iot | ir_md_s_ac_inh |
		ch_dep | ir_fp_mem | ir_254_7 |
		s_ac_inh_if_ac_0;
	wire s_ac_2 = sh_ac_2 | ir_fp_rem | ir_md_s_ac_2;
	wire s_ac_0 = ir[9:12] == 0;

	pa s_pa0(.clk(clk), .reset(rst),
		.in(et10 & s_c_e),
		.p(st1));
	pa s_pa1(.clk(clk), .reset(rst),
		.in(et10 & f_c_e_pse),
		.p(st2));
	pa s_pa2(.clk(clk), .reset(rst),
		.in(mc_rs_t1 & sf3 |
		    et10 & ~f_c_e_pse & ~s_c_e),
		.p(st3));
	pa s_pa3(.clk(clk), .reset(rst),
		.in(st3 & ~s_ac_inh),
		.p(st3a));
	pa s_pa4(.clk(clk), .reset(rst),
		.in(st3a_D),
		.p(st5));
	pa s_pa5(.clk(clk), .reset(rst),
		.in(mc_rs_t1 & sf5a),
		.p(st5a));
	pa s_pa6(.clk(clk), .reset(rst),
		.in(st5a & s_ac_2),
		.p(st6));
	pa s_pa7(.clk(clk), .reset(rst),
		.in(st6_D),
		.p(st6a));
	pa s_pa8(.clk(clk), .reset(rst),
		.in(mc_rs_t1 & sf7 |
		    st3 & s_ac_inh |
		    st5a & ~s_ac_2 |
		    cht8b & ir_cao |
		    dst13 |
		    mc_illeg_address_D |
		    et3 & ir_fpch & ~ir[3] &
		     (ir_130 | ir_131 | ~ir[4] | ~ir[5])),
		.p(st7));

	wire st3a_D, st6_D, mc_illeg_address_D;
	dly100ns s_dly0(.clk(clk), .reset(rst),
		.in(st3a),
		.p(st3a_D));
	dly50ns s_dly1(.clk(clk), .reset(rst),
		.in(st6),
		.p(st6_D));
	dly100ns s_dly2(.clk(clk), .reset(rst),
		.in(mc_illeg_address),
		.p(mc_illeg_address_D));

	always @(posedge clk) begin
		if(mr_clr | st3)
			sf3 <= 0;
		if(st1 | st2)
			sf3 <= 1;
		if(mr_clr | st5a)
			sf5a <= 0;
		if(st5)
			sf5a <= 1;
		if(mr_clr | st7)
			sf7 <= 0;
		if(st6a)
			sf7 <= 1;
	end

	/*
	 * IR
	 */
	// reg [0:17] ir;
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

	wire ir_uuo_a = ir[0:2] == 0;
	wire ir_fpch = ir[0:2] == 1;
	wire ir_2xx = ir[0:2] == 2;
	wire ir_accp_OR_memac = ir[0:2] == 3;
	wire ir_boole = ir[0:2] == 4;
	wire ir_hwt = ir[0:2] == 5;
	wire ir_acbm = ir[0:2] == 6;
	wire ir_iot_a = ir[0:2] == 7;

	wire ir_130 = ir[0:8] == 9'o130;
	wire ir_131 = ir[0:8] == 9'o131;
	wire ir_fsc = ir[0:8] == 9'o132;
	wire ir_cao = ir[0:8] == 9'o133;
	wire ir_ldci = ir[0:8] == 9'o134;
	wire ir_ldc = ir[0:8] == 9'o135;
	wire ir_dpci = ir[0:8] == 9'o136;
	wire ir_dpc = ir[0:8] == 9'o137;

	wire ir_fwt_mov_s = ir_2xx & ir[3:5] == 0;
	wire ir_fwt_movn_m = ir_2xx & ir[3:5] == 1;
	wire ir_fwt = ir_fwt_mov_s | ir_fwt_movn_m;
	wire ir_mul = ir_2xx & ir[3:5] == 2;
	wire ir_div = ir_2xx & ir[3:5] == 3;
	wire ir_sh = ir_2xx & ir[3:5] == 4;
	wire ir_25x = ir_2xx & ir[3:5] == 5;
	wire ir_jp = ir_2xx & ir[3:5] == 6;
	wire ir_as = ir_2xx & ir[3:5] == 7;

	wire ir_ash = ir_sh & ir[6:8] == 0;
	wire ir_rot = ir_sh & ir[6:8] == 1;
	wire ir_lsh = ir_sh & ir[6:8] == 2;
	wire ir_243 = ir_sh & ir[6:8] == 3;
	wire ir_ashc = ir_sh & ir[6:8] == 4;
	wire ir_rotc = ir_sh & ir[6:8] == 5;
	wire ir_lshc = ir_sh & ir[6:8] == 6;
	wire ir_247 = ir_sh & ir[6:8] == 7;

	wire ir_exch = ir_25x & ir[6:8] == 0;
	wire ir_blt = ir_25x & ir[6:8] == 1;
	wire ir_aobjp = ir_25x & ir[6:8] == 2;
	wire ir_aobjn = ir_25x & ir[6:8] == 3;
	wire ir_jrst_a = ir_25x & ir[6:8] == 4;
	wire ir_jfcl = ir_25x & ir[6:8] == 5;
	wire ir_xct = ir_25x & ir[6:8] == 6;
	wire ir_257 = ir_25x & ir[6:8] == 7;

	wire ir_ash_OR_ashc = ir_ash | ir_ashc;
	wire ir_md = ir_2xx & ir[3:4] == 1;
	wire ir_md_s_c_e = ir_md & ir[7];
	wire ir_md_f_c_e = ir_md & (ir[7] | ~ir[8]);
	wire ir_md_s_ac_inh = ir_md & ir[7:8] == 2;
	wire ir_md_f_ac_2 = ir_div & ir[6];
	wire ir_md_s_ac_2 = (ir_div | ir_mul & ir[6]) & ~ir_md_s_ac_inh;
	wire ir_254_7 = ir_25x & ir[6];
	wire ir_iot = ir_iot_a & ~ex_ir_uuo;
	wire ir_jrst = ir_jrst_a & ~ex_ir_uuo;
	wire ir_9_OR_10 = ir[9] | ir[10];
	wire ir_fp = ir_fpch & ir[3];
	wire ir_fp_dir = ir_fp & ir[7:8] == 0;
	wire ir_fp_rem = ir_fp & ir[7:8] == 1;
	wire ir_fp_mem = ir_fp & ir[7:8] == 2;
	wire ir_fp_both = ir_fp & ir[7:8] == 3;
	wire ir_fad = ir_fp & ir[4:5] == 0;
	wire ir_fsb = ir_fp & ir[4:5] == 1;
	wire ir_fmp = ir_fp & ir[4:5] == 2;
	wire ir_fdv = ir_fp & ir[4:5] == 3;
	wire ir14_17_eq_0 = ir[14:17] == 0;

	/* ACCP V MEM AC */
	wire accp = ir_accp_OR_memac & ir[3:4] == 0;
	wire memac_tst = ir_accp_OR_memac & ir[3:4] == 1;
	wire memac_inc = ir_accp_OR_memac & ir[3:4] == 2;
	wire memac_dec = ir_accp_OR_memac & ir[3:4] == 3;
	wire memac = memac_tst | memac_inc | memac_dec;
	wire memac_mem = memac & ir[5];
	wire memac_ac = memac & ~ir[5];
	wire accp_etc_cond = ir_accp_OR_memac & ir[8] & ar0_xor_ar_ov |
		ir[7] & ar_eq_0;
	wire accp_etal_test = accp_etc_cond ^ ir[6];
	wire accp_dir = accp & ir[5];

	/* ACBM */
	wire acbm_swap = ir_acbm & ir[8];
	wire acbm_dir = ir_acbm & ir[5];
	wire acbm_dn = ir_acbm & ir[3:4] == 0;
	wire acbm_cl = ir_acbm & ir[3:4] == 1;
	wire acbm_com = ir_acbm & ir[3:4] == 2;
	wire acbm_set = ir_acbm & ir[3:4] == 3;

	/* FWT */
	wire fwt_swap = ir_fwt & ir[5:6] == 1;
	wire fwt_negate = ir_fwt_movn_m & (~ir[6] | ar[0]);
	wire fwt_00 = ir_fwt & ir[7:8] == 0;
	wire fwt_01 = ir_fwt & ir[7:8] == 1;
	wire fwt_10 = ir_fwt & ir[7:8] == 2;
	wire fwt_11 = ir_fwt & ir[7:8] == 3;

	/* HWT */
	wire hwt_lt_set = hwt_rt & ir[4] & (~ir[5] | mb[18]);
	wire hwt_rt_set = hwt_lt & ir[4] & (~ir[5] | mb[0]);
	wire hwt_lt = ir_hwt & ~ir[3];
	wire hwt_rt = ir_hwt & ir[3];
	wire hwt_swap = ir_hwt & ir[6];
	wire hwt_ar_clr = ir_hwt & (ir[4] | ir[5]);
	wire hwt_00 = ir_hwt & ir[7:8] == 0;
	wire hwt_01 = ir_hwt & ir[7:8] == 1;
	wire hwt_10 = ir_hwt & ir[7:8] == 2;
	wire hwt_11 = ir_hwt & ir[7:8] == 3;

	/* BOOLE */
	wire boole_as_00 = (ir_boole | ir_as) & ir[7:8] == 0;
	wire boole_as_01 = (ir_boole | ir_as) & ir[7:8] == 1;
	wire boole_as_10 = (ir_boole | ir_as) & ir[7:8] == 2;
	wire boole_as_11 = (ir_boole | ir_as) & ir[7:8] == 3;
	wire boole_0 = ir_boole & ir[3:6] == 0;
	wire boole_1 = ir_boole & ir[3:6] == 1;
	wire boole_2 = ir_boole & ir[3:6] == 2;
	wire boole_3 = ir_boole & ir[3:6] == 3;
	wire boole_4 = ir_boole & ir[3:6] == 4;
	wire boole_5 = ir_boole & ir[3:6] == 5;
	wire boole_6 = ir_boole & ir[3:6] == 6;
	wire boole_7 = ir_boole & ir[3:6] == 7;
	wire boole_10 = ir_boole & ir[3:6] == 8;
	wire boole_11 = ir_boole & ir[3:6] == 9;
	wire boole_12 = ir_boole & ir[3:6] == 10;
	wire boole_13 = ir_boole & ir[3:6] == 11;
	wire boole_14 = ir_boole & ir[3:6] == 12;
	wire boole_15 = ir_boole & ir[3:6] == 13;
	wire boole_16 = ir_boole & ir[3:6] == 14;
	wire boole_17 = ir_boole & ir[3:6] == 15;

	/* JUMP V PUSH */
	wire jp_pushj = ir_jp & ir[6:8] == 0;
	wire jp_push = ir_jp & ir[6:8] == 1;
	wire jp_pop = ir_jp & ir[6:8] == 2;
	wire jp_popj = ir_jp & ir[6:8] == 3;
	wire jp_jsr = ir_jp & ir[6:8] == 4;
	wire jp_jsp = ir_jp & ir[6:8] == 5;
	wire jp_jsa = ir_jp & ir[6:8] == 6;
	wire jp_jra = ir_jp & ir[6:8] == 7;
	wire jp_flag_stor = jp_pushj | jp_jsr | jp_jsp;
	wire jp_AND_NOT_jsr = ir_jp & ~jp_jsr;
	wire jp_AND_ir6_0 = ir_jp & ~ir[6];
	wire jp_jmp = ir_jp & ~jp_push & ~jp_pop;

	/* AS */
	wire as_plus = ir_as & ~ir[6];
	wire as_minus = ir_as & ir[6];

	/* XCT */
	wire xct_t0;
	pa xct_pa0(.clk(clk), .reset(rst),
		.in(et3 & ir_xct),
		.p(xct_t0));

	wire xct_t0_D;
	dly200ns xct_dly0(.clk(clk), .reset(rst),
		.in(xct_t0),
		.p(xct_t0_D));

	/*
	 * UUO
	 */
	reg uuo_f1;
	wire uuo_t1;
	wire uuo_t2;

	pa uuo_pa0(.clk(clk), .reset(rst),
		.in(uuo_f1 & mc_rs_t1),
		.p(uuo_t1));
	pa uuo_pa1(.clk(clk), .reset(rst),
		.in(uuo_t1_D),
		.p(uuo_t2));

	wire uuo_t1_D;
	dly100ns uuo_dly0(.clk(clk), .reset(rst),
		.in(uuo_t1),
		.p(uuo_t1_D));

	always @(posedge clk) begin
		if(mr_clr | uuo_t1)
			uuo_f1 <= 0;
		if(mblt_fm_ir1_uuo_t0)
			uuo_f1 <= 1;
	end

	/*
	 * PC
	 */
	// reg [18:35] pc;
	wire pc_clr = et7 & pc_set | kt1 & key_start_OR_read_in;
	wire pc_fm_ma1 = et8 & pc_set | kt3 & key_start_OR_read_in;
	wire pc_inc = et0 & ~pc_inc_inh_et0 |
		blt_t5a & ~mq[0] |
		et9 & pc_inc_et9 |
		iot_t0a & ~ar_cry0 & ~pi_cyc;
	wire pc_set = pc_set_enable | jp_jmp | ir_jrst;
	wire pc_inc_et9 = jp_jsr | jp_jsa | pc_inc_enable;
	wire pc_inc_inh_et0 = key_execute |
		~ir_cao & (ch_NOT_inc_op | ch_inc_op) |
		ir_xct | ex_ir_uuo | pi_cyc | iot_blk | ir_blt;
	wire pc_inc_enable = memac_mem & accp_etal_test |
		accp & accp_etal_test |
		ir_acbm & accp_etal_test |
		iot_conso & ~ar_eq_0 |
		iot_consz & ar_eq_0;
	wire selected_flag = ar_ov_flag & ir[9] |
		ar_cry0_flag & ir[10] |
		ar_cry1_flag & ir[11] |
		ar_pc_chg_flag & ir[12];
	wire pc_set_enable = memac_ac & accp_etal_test |
		ir_aobjn & ar[0] |
		ir_aobjp & ~ar[0] |
		ir_jfcl & selected_flag;
	wire pc_set_OR_pc_inc = pc_set | pc_inc_et9;

	always @(posedge clk) begin
		if(pc_clr)
			pc <= 0;
		if(pc_inc)
			pc <= pc + 18'b1;
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
	wire ex_set = cpa & iobus_datao_set;
	wire ex_ir_uuo = ir_jrst_a & ir_9_OR_10 & ex_user |
		ir_iot_a & ~ex_pi_sync & ex_user & ~cpa_iot_user |
		ex_uuo_sync & ir_uuo_a;
	wire ex_inh_rel = ~ex_user | ex_pi_sync | ma18_31_eq_0 | ex_ill_op;

	always @(posedge clk) begin
		if(mr_start | et7 & jp_jsr & (ex_pi_sync | ex_ill_op))
			ex_user <= 0;
		if(mr_clr) begin
			if(ex_mode_sync)
				ex_user <= 1;
			ex_mode_sync <= 0;
			ex_uuo_sync <= 0;
			ex_pi_sync <= 0;
		end
		if(et1 & ir_jrst & ir[12] |
		   ar_flag_set & mb[5])
			ex_mode_sync <= 1;
		if(at1)
			ex_uuo_sync <= 1;
		if(pi_cyc)
			ex_pi_sync <= 1;
		if(mr_start | et8 & jp_jsr | et4 & iot_blk)
			ex_ill_op <= 0;
		if(et1 & ex_ir_uuo)
			ex_ill_op <= 1;
	end

	/*
	 * MB
	 */
	// reg [0:35] mb;
	wire mblt_clr = et1 & ex_ir_uuo | mb_clr;
	wire mblt_fm_ar0 = mb_fm_arJ | mb_ar_swap | mb_fm_ar0 | cfac_mb_ar_swap;
	wire mblt_fm_ar1 = mb_fm_arJ | mb_ar_swap | cfac_mb_ar_swap;
	wire mblt_fm_mq0 = cfac_mb_mq_swap | cfac_mb_fm_mqJ | mb_fm_mqJ;
	wire mblt_fm_mq1 = cfac_mb_mq_swap | cfac_mb_fm_mqJ | mb_fm_mqJ;
	wire mblt_fm_mbrtJ = mblt_mbrt_swap;
	wire mb_fm_ir1 = mblt_fm_ir1_uuo_t0;
	wire mbrt_clr = mb_clr;
	wire mbrt_fm_ar0 = mb_fm_arJ | mb_ar_swap | mb_fm_ar0 | cfac_mb_ar_swap;
	wire mbrt_fm_ar1 = mb_fm_arJ | mb_ar_swap | cfac_mb_ar_swap;
	wire mbrt_fm_mq0 = cfac_mb_mq_swap | cfac_mb_fm_mqJ | mb_fm_mqJ;
	wire mbrt_fm_mq1 = cfac_mb_mq_swap | cfac_mb_fm_mqJ | mb_fm_mqJ;
	wire mbrt_fm_mbltJ = mblt_mbrt_swap;
	wire mb_fm_pc1 = mb_fm_pc1_init;

	// ET0
	wire mb_fm_mqJ_et0 = jp_pop | jp_popj | jp_jra;
	wire mb_ar_swap_et0 = hwt_10 | jp_jsp | ir_exch | ir_blt | ir_fsb;
	wire mb_fm_arJ_et0 = fwt_01 | fwt_10 | iot_status;
	wire mbltrtJ_et0 = acbm_swap | iot_cono | jp_jsa;
	// ET1
	wire mb_fm_ar0_et1 = ir_acbm;
	wire mbltrtJ_et1 = hwt_swap | fwt_swap | ir_blt;
	// ET4
	wire mb_ar_swap_et4 = fwt_swap | iot_blk | ir_acbm;
	// ET6
	wire mb_fm_pc1_et6 = jp_jsa;
	// ET9
	wire mb_ar_swap_et9 = ir_acbm | jp_AND_NOT_jsr;
	// ET10
	wire mb_fm_arJ_inh_et10 = ir_jp | ir_exch | ch_dep;
	wire mb_ar_swap_et10 = jp_AND_ir6_0;
	wire mb_fm_arJ_et10 = (f_c_e_pse | s_c_e) & ~mb_fm_arJ_inh_et10;

	wire mb_clr = et5 & mb_pc_sto |
		mc_mb_clr_D;
	wire mblt_mbrt_swap = et0a & mbltrtJ_et0 |
		et1 & mbltrtJ_et1 |
		ft1a & f_c_c_aclt;
	wire mb_fm_misc_bits1 = et6 & jp_flag_stor;
	wire mb_fm_ar0 = et1 & mb_fm_ar0_et1 |
		dct3 |
		et6 & acbm_cl;
	wire mb_fm_arJ = at3a | st5 | key_wr | dst1 | mst1 |
		et0a & mb_fm_arJ_et0 |
		et10 & mb_fm_arJ_et10 |
		kt3 & key_execute;
	wire mb_ar_swap = ft3 | blt_t1 | blt_t4 | blt_t6 |
		et0a & mb_ar_swap_et0 |
		et4 & mb_ar_swap_et4 |
		et9 & mb_ar_swap_et9 |
		et10 & mb_ar_swap_et10 |
		ft1a & ~f_c_c_aclt_OR_rt;
	wire mb_fm_mqJ = st6 | ft4a | blt_t0a |
		et0a & mb_fm_mqJ_et0;
	wire mb1_8_clr = (fpt3 | fat8a) & ~mb[0];
	wire mb1_8_set = (fpt3 | fat8a) & mb[0];
	wire mblt_fm_ir1_uuo_t0 = et3 & ex_ir_uuo;
	wire mb_fm_pc1_init = et6 & mb_pc_sto |
		et6 & mb_fm_pc1_et6;
	wire mb_pc_sto = jp_pushj | jp_jsr | jp_jsp | ir_jrst;

	wire mc_mb_clr_D;
	dly100ns mb_dly0(.clk(clk), .reset(rst), .in(mc_mb_clr), .p(mc_mb_clr_D));


	always @(posedge clk) begin: mbctl
		integer i;
		if(mc_mb_membus_enable)
			mb <= mb | membus_mb_in;
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
		for(i = 0; i < 18; i = i+1) begin
			if(mblt_fm_mq0 & ~mq[i])
				mb[i] <= 0;
			if(mblt_fm_mq1 & mq[i])
				mb[i] <= 1;
			if(mbrt_fm_mq0 & ~mq[i+18])
				mb[i+18] <= 0;
			if(mbrt_fm_mq1 & mq[i+18])
				mb[i+18] <= 1;
		end
		if(mblt_fm_mbrtJ)
			mb[0:17] <= mb[18:35];
		if(mbrt_fm_mbltJ)
			mb[18:35] <= mb[0:17];
		if(mb_fm_pc1)
			mb[18:35] <= mb[18:35] | pc;
		if(mb_fm_ir1)
			mb[0:17] <= mb[0:17] | { ir[0:12], 5'b0 };
		if(mb1_8_clr)
			mb[1:8] <= 0;
		if(mb1_8_set)
			mb[1:8] <= 8'o377;
		if(mb_fm_misc_bits1) begin
			if(ar_ov_flag) mb[0] <= 1;
			if(ar_cry0_flag) mb[1] <= 1;
			if(ar_cry1_flag) mb[2] <= 1;
			if(ar_pc_chg_flag) mb[3] <= 1;
			if(chf7) mb[4] <= 1;
			if(ex_user) mb[5] <= 1;
`ifdef FIX_USER_IOT
			if(cpa_iot_user) mb[6] <= 1;
`endif
		end
	end

	/*
	 * AR
	 */
	// reg [0:35] ar;
	reg ar_com_cont;
	reg ar_pc_chg_flag;
	reg ar_ov_flag;
	reg ar_cry0_flag;
	reg ar_cry1_flag;
	reg ar_cry0;
	reg ar_cry1;
	wire arlt_clr = ar_clr | at4 | blt_t2 | key_ar_clr;
	wire arlt_com = ar_com | ar_as_t0 | cfac_ar_com | et4 & hwt_lt_set;
	wire arlt_fm_mb_xor = ar_as_t1 | ar_fm_mb_xor;
	wire arlt_fm_mb0 = cfac_mb_ar_swap | mb_ar_swap | ar_fm_mbJ | ar_fm_mb0 |
		et4 & ar_fm_mbltJ_et4;
	wire arlt_fm_mb1 = cfac_mb_ar_swap | mb_ar_swap | ar_fm_mbJ | ar_fm_mb1 |
		et4 & ar_fm_mbltJ_et4;
	wire arlt_fm_datasw1 = key_ar_fm_datasw1;
	wire arlt_fm_iob1 = iot_t3;
	wire arrt_clr = ar_clr | key_ar_clr;
	wire arrt_com = ar_com | ar_as_t0 | cfac_ar_com | et4 & hwt_rt_set;
	wire arrt_fm_mb_xor = ar_as_t1 | ar_fm_mb_xor;
	wire arrt_fm_mb0 = cfac_mb_ar_swap | mb_ar_swap | ar_fm_mbJ | ar_fm_mb0 |
		et4 & ar_fm_mbrtJ_et4 | at0;
	wire arrt_fm_mb1 = cfac_mb_ar_swap | mb_ar_swap | ar_fm_mbJ | ar_fm_mb1 |
		et4 & ar_fm_mbrtJ_et4 | at0;
	wire arrt_fm_datasw1 = key_ar_fm_datasw1;
	wire arrt_fm_iob1 = iot_t3;
	// just one for simplicity
	wire ar_shlt = cfac_ar_sh_lt;
	wire ar_shrt = cfac_ar_sh_rt;
	wire ar_cry_initiate = ar_as_t2;


	wire ar0_shl_inp = ~ir_ash & ~shc_ashc ? ar[1] : ar[0];
	wire ar0_shr_inp = ir_rotc ? mq[35] :
		ir_rot ? ar[35] :
		(ir_ash | shc_ashc | ms_mult | ir_fdv) ? ar[0] :
		ir_div ? ~mq[35] :
		ch_load | ir_lsh | ir_lshc ? 1'b0 :
		1'b0;	// shouldn't happen
	wire ar35_shl_inp = ir_rot ? ar[0] :
		shc_ashc ? mq[1] :
		ir_rotc | shc_lshc_OR_div ? mq[0] :
		ch_dep | ir_lsh | ir_ash ? 1'b0 :
		1'b0;	// shouldn't happen

	wire shc_ashc = ir_ashc | nrf2 | faf3;
	wire shc_lshc_OR_div = ir_lshc | shc_div;
	wire shc_div = (ir_div | ir_fdv) & ~nrf2;

	// ET0
	wire ar_clr_et0 = boole_0 | boole_3 | boole_14 | boole_17;
	wire ar_com_et0 = boole_2 | boole_4 |
		boole_12 | boole_13 | boole_15;
	wire ar_fm_mbJ_et0 = hwt_11 | fwt_00 | fwt_11 |
		memac_mem | iot_blk | iot_datao;
	// ET1
	wire ar_fm_mb0_et1 = boole_1 | boole_2 | boole_15 | boole_16;
	wire ar_fm_mb1_et1 = boole_3 | boole_4 | boole_7 |
		boole_10 | boole_13 | acbm_set;
	wire ar_fm_mb_xor_et1 = boole_6 | boole_11 | boole_14 | acbm_com;
	wire ar_clr_et1 = hwt_ar_clr | iot_status | iot_datai;
	// ET4
	wire ar_com_et4 = boole_4 | boole_10 | boole_11 | boole_14 |
		boole_15 | boole_16 | boole_17;
	wire ar_fm_mbltJ_et4 = hwt_lt | iot_cono;
	wire ar_fm_mbrtJ_et4 = hwt_rt;
	// ET5
	wire ar_com_et5 = ir_acbm;
	// ET6
	wire ar_fm_mb0_et6 = iot_consz | iot_conso;
	// ET7
	wire ar_com_et7 = ir_acbm;

	wire ar_clr = dst2 | fat6 |
		et0a & ar_clr_et0 |
		et1 & ar_clr_et1 |
		mst1_D0;
	wire ar_com = ar_incdec_t0 | ar_negate_t0 |
		et0a & ar_com_et0 |
		et4 & ar_com_et4 |
		et5 & ar_com_et5 |
		et7 & ar_com_et7 |
		ar_cry_comp & ar_com_cont;
	wire ar_fm_mb0 = dct1 | lct0a |
		et1 & ar_fm_mb0_et1 |
		et6 & ar_fm_mb0_et6;
	wire ar_fm_mb_xor = et1 & ar_fm_mb_xor_et1;
	wire ar_fm_mb1 = et1 & ar_fm_mb1_et1;
	wire ar_fm_mbJ = cht1 | lct0 | mpt2 |
		et0a & ar_fm_mbJ_et0;

	wire ar_incdec_t0;
	wire ar_negate_t0;
	wire ar_incdec_t1;
	// these two are a bit of a hack
	wire ar35_cry_in = ar_incdec_t1;
	wire ar17_cry_in = ar_incdec_t1 & (ar_incdec_lt_rt | ir_blt);
	wire ar_as_t0;
	wire ar_as_t1;
	wire ar_as_t2;
	wire ar_t3;
	wire ar_cry_comp;
	wire ar1_8_clr = (fpt3 | fat5) & ~ar[0];
	wire ar1_8_set = (fpt3 | fat5) & ar[0];
	wire ar_fm_sc1_8J = nrt4 | fst0a;
	wire ar0_5_fm_sc3_8J = cht6 & ch_inc_op;

	wire ar_add = as_plus;
	wire ar_sub = as_minus | accp;
	wire ar_inc = memac_inc | jp_push | jp_pushj | iot_blk |
		ir_aobjp | ir_aobjn;
	wire ar_incdec_lt_rt = iot_blk | ir_aobjp | ir_aobjn | jp_AND_ir6_0;
	wire ar_dec = memac_dec | jp_pop | jp_popj;
	wire ar_sbr = fwt_negate | ar_add | ar_sub | ar_inc | ar_dec | ir_fsb;

	wire ar_flag_clr = mr_start | et0 & ir_jrst & ir[11];
	wire ar_jrst_AND_ir11 = ir_jrst & ir[11];
	wire ar_flag_set = et1 & ar_jrst_AND_ir11;
	wire ar_jfcl_clr = et10 & ir_jfcl;
	wire ar_eq_fp_half = ar == 36'o000400000000;
	wire ar_eq_0 = ar == 0;
	wire ar0_xor_ar1 = ar[0] ^ ar[1];
	wire ar_ov_set = ar_cry0 ^ ar_cry1;
	wire ar_cry0_xor_cry1 = ar_ov_set & ~memac;
	wire ar0_xor_ar_ov = ar[0] ^ ar_cry0_xor_cry1;
	wire ar0_xor_mb0 = ar[0] ^ mb[0];
	wire ar0_eq_sc0 = ar[0] == sc[0];

	wire set_flags_et10 = et10 & (memac | ir_as);

	pa ar_pa0(.clk(clk), .reset(rst),
		.in(et3 & ar_dec),
		.p(ar_incdec_t0));
	pa ar_pa1(.clk(clk), .reset(rst),
		.in(et3 & fwt_negate |
		    et3 & ir_fsb |
		    cfac_ar_negate),
		.p(ar_negate_t0));
	pa ar_pa2(.clk(clk), .reset(rst),
		.in(ar_incdec_t0_D |
		    ar_negate_t0_D |
		    et3 & ar_inc |
		    blt_t5 | nrt5 | cht4),
		.p(ar_incdec_t1));
	pa ar_pa3();	// AR17 CRY IN
	pa ar_pa4(.clk(clk), .reset(rst),
		.in(et3 & ar_sub |
		    cfac_ar_sub |
		    blt_t3),
		.p(ar_as_t0));
	pa ar_pa5(.clk(clk), .reset(rst),
		.in(ar_as_t0_D | et3 & ar_add |
		    at3 | cfac_ar_add),
		.p(ar_as_t1));
	pa ar_pa6(.clk(clk), .reset(rst),
		.in(ar_as_t1_D),
		.p(ar_as_t2));
	pa ar_pa7(.clk(clk), .reset(rst),
		.in(ar_as_t2 | ar_incdec_t1 | ar17_cry_in),
		.p(ar_cry_comp));
	pa ar_pa8(.clk(clk), .reset(rst),
		.in(ar_cry_comp & ~ar_com_cont |
		    ar_cry_comp_D & ar_com_cont),
		.p(ar_t3));

	wire ar_incdec_t0_D, ar_negate_t0_D;
	wire ar_as_t0_D, ar_as_t1_D, ar_cry_comp_D;
	dly100ns ar_dly0(.clk(clk), .reset(rst),
		.in(ar_incdec_t0),
		.p(ar_incdec_t0_D));
	dly100ns ar_dly1(.clk(clk), .reset(rst),
		.in(ar_negate_t0),
		.p(ar_negate_t0_D));
	dly100ns ar_dly2(.clk(clk), .reset(rst),
		.in(ar_as_t0),
		.p(ar_as_t0_D));
	dly100ns ar_dly3(.clk(clk), .reset(rst),
		.in(ar_as_t1),
		.p(ar_as_t1_D));
	dly100ns ar_dly4(.clk(clk), .reset(rst),
		.in(ar_cry_comp),
		.p(ar_cry_comp_D));

	wire [0:35] ar_mb_cry = mb & ~ar;
	wire [-1:35] ar_cry_in = ar_cry_initiate ? { mb[0:35]&~ar[0:35], 1'b0 } :
		ar35_cry_in & ar17_cry_in ? 36'o000001000001 :
		ar35_cry_in ? 36'o000000000001 :
		ar17_cry_in ? 36'o000001000000 :
		36'b0;

	// hold the cry out temporarily
	reg cry0, cry1;
	wire [0:35] ar_cry1_add = ar[1:35] + ar_cry_in[0:35];

	always @(posedge clk) begin: arctl
		integer i;
		if(arlt_clr)
			ar[0:17] <= 0;
		if(arrt_clr)
			ar[18:35] <= 0;
		if(ar_cry_in) begin
			cry1 <= ar_cry1_add[0];
			{cry0, ar} <= ar + ar_cry_in;
		end
		if(arlt_com)
			ar[0:17] <= ~ar[0:17];
		if(arrt_com)
			ar[18:35] <= ~ar[18:35];
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
		if(ar_shlt)
			ar <= { ar0_shl_inp, ar[2:35], ar35_shl_inp };
		if(ar_shrt)
			ar <= { ar0_shr_inp, ar[0:34] };
		if(arlt_fm_datasw1)
			ar[0:17] <= ar[0:17] | datasw[0:17];
		if(arrt_fm_datasw1)
			ar[18:35] <= ar[18:35] | datasw[18:35];
		if(arlt_fm_iob1)
			ar[0:17] <= ar[0:17] | iob[0:17];
		if(arrt_fm_iob1)
			ar[18:35] <= ar[18:35] | iob[18:35];
		if(ar1_8_clr)
			ar[1:8] <= 0;
		if(ar1_8_set)
			ar[1:8] <= 8'o377;
		if(ar_fm_sc1_8J)
			ar[1:8] <= sc[1:8];
		if(ar0_5_fm_sc3_8J)
			ar[0:5] <= sc[3:8];

		if(mr_clr | ar_t3)
			ar_com_cont <= 0;
		if(ar_as_t0 | ar_incdec_t0)
			ar_com_cont <= 1;
		if(ar_flag_clr) begin
			ar_pc_chg_flag <= 0;
			ar_ov_flag <= 0;
			ar_cry0_flag <= 0;
			ar_cry1_flag <= 0;
		end

		// Flags
		if(ar_jfcl_clr & ir[12] |
		   cpa_cono_set & iob[29])
			ar_pc_chg_flag <= 0;
		if(ar_flag_set & mb[3] |
		   et9 & pc_set_OR_pc_inc & ~ar_jrst_AND_ir11)
			ar_pc_chg_flag <= 1;

		if(ar_jfcl_clr & ir[9] |
		   cpa_cono_set & iob[32])
			ar_ov_flag <= 0;
		if(ar_flag_set & mb[0] |
		   set_flags_et10 & ar_ov_set |
		   cfac_overflow |
		   et10 & ir_fwt & ~ar_cry0 & ar_cry1 |
		   sct1 & ~mb[18] & ir_ash_OR_ashc & ar0_xor_ar1)
			ar_ov_flag <= 1;

		if(ar_jfcl_clr & ir[10])
			ar_cry0_flag <= 0;
		if(ar_flag_set & mb[1] |
		   set_flags_et10 & ar_cry0)
			ar_cry0_flag <= 1;

		if(ar_jfcl_clr & ir[11])
			ar_cry1_flag <= 0;
		if(ar_flag_set & mb[2] |
		   set_flags_et10 & ar_cry1)
			ar_cry1_flag <= 1;

		if(mr_start) begin
			cry0 <= 0;
			cry1 <= 0;
		end
		if(cry0) begin
			cry0 <= 0;
			ar_cry0 <= 1;
		end
		if(cry1) begin
			cry1 <= 0;
			ar_cry1 <= 1;
		end
		if(et0) begin
			ar_cry0 <= 0;
			ar_cry1 <= 0;
		end
	end

	/*
	 * MQ
	 */
	// reg [0:35] mq;
	reg mq36;
	wire mqlt_clr = mr_clr;
	wire mqlt_fm_mb0 = mq_fm_mbJ | cfac_mb_mq_swap;
	wire mqlt_fm_mb1 = mq_fm_mbJ | cfac_mb_mq_swap | dct0b;
	wire mqrt_clr = mr_clr;
	wire mqrt_fm_mb0 = mq_fm_mbJ | cfac_mb_mq_swap;
	wire mqrt_fm_mb1 = mq_fm_mbJ | cfac_mb_mq_swap | dct0b;
	wire mq_shl = cfac_mq_sh_lt;
	wire mq_shr = cfac_mq_sh_rt;
	wire mq0_set = dst10a & ar[35];
	wire mq0_clr = dst10a & ~ar[35];

	wire mq0_shl_inp = shc_ashc ? ar[0] : mq[1];
	wire mq0_shr_inp = ms_mult & sc_eq_777 | shc_ashc ?
		ar[0] : ar[35];
	wire mq1_shr_inp = shc_ashc ? ar[35] : mq[0];
	wire mq35_shl_inp = ir_rotc ? ar[0] :
		shc_div ? ~ar[0] :
		ch_inc_op | ch_NOT_inc_op ? 1'b1 :
		ir_lshc | shc_ashc | ch_dep ? 1'b0 :
		1'b0;	// shouldn't happen

	wire mq_fm_mbJ = ft4 | ft4a | dst1 | mst1;
	wire mq35_xor_mb0 = mq[35] ^ mb[0];
	wire mq35_eq_mq36 = mq[35] == mq36;

	always @(posedge clk) begin: mqctl
		integer i;
		if(mqlt_clr)
			mq[0:17] <= 0;
		if(mqrt_clr)
			mq[18:35] <= 0;
		for(i = 0; i < 18; i = i+1) begin
			if(mqlt_fm_mb0 & ~mb[i])
				mq[i] <= 0;
			if(mqlt_fm_mb1 & mb[i])
				mq[i] <= 1;
			if(mqrt_fm_mb0 & ~mb[i+18])
				mq[i+18] <= 0;
			if(mqrt_fm_mb1 & mb[i+18])
				mq[i+18] <= 1;
		end
		if(mq_shl)
			mq <= { mq0_shl_inp, mq[2:35], mq35_shl_inp };
		if(mq_shr)
			mq <= { mq0_shr_inp, mq1_shr_inp, mq[1:34] };
		if(mq0_set)
			mq[0] <= 1;
		if(mq0_clr)
			mq[0] <= 0;

		if(mr_clr | cfac_mq_sh_rt & ~mq[35])
			mq36 <= 0;
		if(cfac_mq_sh_rt & mq[35])
			mq36 <= 1;
	end

	/*
	 * FE, SC
	 */
	reg [0:8] fe;
	wire fe_clr = mr_clr;
	wire fe_fm_sc1 = fpt3;
	wire fe_fm_mb0_5_1 = cht8b;

	reg [0:8] sc;
	wire sc_clr = mr_clr | cht4 | cht8a |
		fat5a | fpt3 | mst5 | dst20 |
		cht6 & ch_inc_op;
	wire sc_inc = fpt2 | fat2 | sht0 | fst1 | nrt0 |
		sct1 | mst2 | dst14a | nrt2;
	wire sc_com = cht8b | cht5 | fat3 | nrt1 | lct0 | dct0 |
		et1 & ir_fsc & ~ar[0] |
		fpt1 & fp_ar0_xor_fmf1 |
		fat0 & ~ar0_xor_mb0 |
		fpt1a & ~fp_ar0_xor_mb0_xor_fmf1 |
		sht1 & mb[18] |
		fpt1b & fp_mb0_eq_fmf1 |
		fat7 & mb[0] |
		nrt3 & (~ar[0] | nr_round);
	wire sc_pad = cht2 | cht7 | sat1 | fat8 | fat1;
	wire sc_cry = sat21;
	wire sc_fm_fe1 = cht9 | fmt0b | fdt0b;
	wire sc_fm_mb18_28_35_0 = et0a & (fsc | shift_op);
	wire sc_mb0_5_0_enable = chf1;
	wire sc_mb6_11_1_enable = chf2;
	wire sc_ar0_8_1_enable = fsf1 | fpf1 | faf2;
	wire sc_mb0_8_1_enable = fpf2 | faf1;
	// TODO: what the hell is sc8b?
	wire sc_eq_777 = sc == 9'o777;
	wire sc0_2_eq_7 = sc[0:2] == 3'o7;

	wire [0:8] sc_data =
		{9{sc_mb0_5_0_enable}} & ~{ 3'b0, mb[0:5] } |
		{9{sc_mb6_11_1_enable}} & { 2'b0, mb[6:11] } |
		{9{sc_ar0_8_1_enable}} & ar[0:8] |
		{9{sc_mb0_8_1_enable}} & mb[0:8];
	wire [0:8] scN_cry = ~sc & sc_data;

	wire sat0;
	wire sat1;
	wire sat2;
	wire sat21;
	wire sat3;

	pa sa_pa0(.clk(clk), .reset(rst),
		.in(cht3 | fst0 | fat1 | fpt1 | fpt1aa),
		.p(sat0));
	pa sa_pa1(.clk(clk), .reset(rst),
		.in(sat0_D),
		.p(sat1));
	pa sa_pa2(.clk(clk), .reset(rst),
		.in(sat1_D),
		.p(sat2));
	pa sa_pa3(.clk(clk), .reset(rst),
		.in(sat2_D),
		.p(sat21));
	pa sa_pa4(.clk(clk), .reset(rst),
		.in(sat21_D),
		.p(sat3));

	wire sat0_D, sat1_D, sat2_D, sat21_D;
	dly150ns sa_dly0(.clk(clk), .reset(rst),
		.in(sat0),
		.p(sat0_D));
	dly200ns sa_dly1(.clk(clk), .reset(rst),
		.in(sat1),
		.p(sat1_D));
	dly50ns sa_dly2(.clk(clk), .reset(rst),
		.in(sat2),
		.p(sat2_D));
	dly100ns sa_dly3(.clk(clk), .reset(rst),
		.in(sat21),
		.p(sat21_D));

	wire sct0;
	wire sct1;
	wire sct2;

	pa sc_pa0(.clk(clk), .reset(rst),
		.in(lct0 | dct0 | sht1 | fat5 |
		    cht8b & ~ir_cao),
		.p(sct0));
	pa sc_pa1(.clk(clk), .reset(rst),
		.in((sct0_D | sct1_D) & ~sc_eq_777),
		.p(sct1));
	pa sc_pa2(.clk(clk), .reset(rst),
		.in((sct0_D | sct1_D) & sc_eq_777),
		.p(sct2));

	wire sct0_D, sct1_D;
	dly200ns sc_dly0(.clk(clk), .reset(rst),
		.in(sct0),
		.p(sct0_D));
	// should be 75ns
	dly70ns sc_dly1(.clk(clk), .reset(rst),
		.in(sct1),
		.p(sct1_D));

	always @(posedge clk) begin
		if(fe_clr)
			fe <= 0;
		if(fe_fm_sc1)
			fe <= fe | sc;
		if(fe_fm_mb0_5_1)
			fe <= fe | { 3'b0, mb[0:5]};

		if(sc_clr)
			sc <= 0;
		if(sc_inc)
			sc <= sc + 9'b1;
		if(sc_com)
			sc <= ~sc;
		if(sc_pad)
			sc <= sc ^ sc_data;
		if(sc_cry)
			sc <= sc + { scN_cry[1:8], 1'b0 };
		if(sc_fm_fe1)
			sc <= sc | fe;
		if(sc_fm_mb18_28_35_0)
			sc <= sc | ~{ mb[18], mb[28:35] };
		if(cht4a | fmt0a | fdt0a | mpt0 | ds_div_t0)
			sc[0] <= 1;
		if(fpt01 | fmt0a | fdt0a | mpt0 | ds_div_t0)
			sc[1] <= 1;
		if(fmt0a | fdt0a | mpt0 | ds_div_t0)
			sc[2] <= 1;
		if(fmt0a | fdt0a)
			sc[3] <= 1;
		if(cht4a | mpt0 | ds_div_t0)
			sc[4:5] <= 2'b11;
		if(fmt0a | mpt0)
			sc[6] <= 1;
		if(cht4a | ds_div_t0)
			sc[7] <= 1;
		if(cht4a | fdt0a | ds_div_t0)
			sc[8] <= 1;
	end

	/*
	 * CFAC
	 */
	wire cfac_ar_negate = dst0 | dst5 | dst9 | dst19 | dst21;
	wire cfac_ar_add = dst11 | dst15 | dst17 | mst3 | fat9;
	wire cfac_ar_sub = dst12 | dst14 | dst18 | mst4;
	wire cfac_mb_fm_mqJ = mpt1 | lct0 | dct0a | dct1;
	wire cfac_mb_mq_swap = dst2 | dst3 | dst7 |
		dst9 | dst19a | dst21a |
		blt_t0 | blt_t3a | blt_t5;
	wire cfac_mb_ar_swap = dst4 | dst6 | dst8 | dst20 |
		ch_inc_op & cht7 |
		fat1a & ar0_eq_sc0;
	wire cfac_ar_com = dst7 | dct0a | dct2 |
		mpt0a & ~ir[6] & ar[0];
	wire cfac_overflow = mpt0a & mpf2 & ar[0] |
		mpt1 & ~ar_eq_0 |
		nrt3 & ~sc[0] |
		fst0a & ~ar0_eq_sc0 |
		dst13;
	wire cfac_ar_sh_lt = dst14a | nrt2 |
		sct1 & (dcf1 | shift_op & ~mb[18]);
	wire cfac_mq_sh_lt = dst14a | nrt2 | dst10b |
		sct1 & (dcf1 | chf4 | shift_op & ~mb[18]);
	wire cfac_ar_sh_rt = nrt0 | mst2 | dst16 | dst10a | fdt1 |
		sct1 & (faf3 | lcf1 | shift_op & mb[18]);
	wire cfac_mq_sh_rt = nrt0 | mst2 | mst5 | fdt1 |
		sct1 & (faf3 | shift_op & mb[18]);

	/*
	 * BLT
	 */
	reg blt_f0a;
	reg blt_f3a;
	reg blt_f5a;
	wire blt_done = ~mq[0] | pi_rq;
	wire blt_last = ir_blt & ~mq[0];

	wire blt_t0;
	wire blt_t0a;
	wire blt_t1;
	wire blt_t2;
	wire blt_t3;
	wire blt_t3a;
	wire blt_t4;
	wire blt_t5;
	wire blt_t5a;
	wire blt_t6;

	pa blt_pa0(.clk(clk), .reset(rst),
		.in(et3 & ir_blt),
		.p(blt_t0));
	pa blt_pa1(.clk(clk), .reset(rst),
		.in(mc_rs_t1 & blt_f0a),
		.p(blt_t0a));
	pa blt_pa2(.clk(clk), .reset(rst),
		.in(blt_t0a_D),
		.p(blt_t1));
	pa blt_pa3(.clk(clk), .reset(rst),
		.in(blt_t1_D),
		.p(blt_t2));
	pa blt_pa4(.clk(clk), .reset(rst),
		.in(blt_t2_D),
		.p(blt_t3));
	pa blt_pa5(.clk(clk), .reset(rst),
		.in(ar_t3 & blt_f3a),
		.p(blt_t3a));
	pa blt_pa6(.clk(clk), .reset(rst),
		.in(blt_t3a_D),
		.p(blt_t4));
	pa blt_pa7(.clk(clk), .reset(rst),
		.in(blt_t4_D),
		.p(blt_t5));
	pa blt_pa8(.clk(clk), .reset(rst),
		.in(ar_t3 & blt_f5a),
		.p(blt_t5a));
	pa blt_pa9(.clk(clk), .reset(rst),
		.in(blt_t5a & ~blt_done),
		.p(blt_t6));

	wire blt_t0a_D, blt_t1_D;
	wire blt_t2_D, blt_t3a_D;
	wire blt_t4_D, blt_t6_D;
	dly100ns blt_dly0(.clk(clk), .reset(rst),
		.in(blt_t0a),
		.p(blt_t0a_D));
	dly100ns blt_dly1(.clk(clk), .reset(rst),
		.in(blt_t1),
		.p(blt_t1_D));
	dly100ns blt_dly2(.clk(clk), .reset(rst),
		.in(blt_t2),
		.p(blt_t2_D));
	dly100ns blt_dly3(.clk(clk), .reset(rst),
		.in(blt_t3a),
		.p(blt_t3a_D));
	dly100ns blt_dly4(.clk(clk), .reset(rst),
		.in(blt_t4),
		.p(blt_t4_D));
	dly100ns blt_dly5(.clk(clk), .reset(rst),
		.in(blt_t6),
		.p(blt_t6_D));

	always @(posedge clk) begin
		if(mr_clr | blt_t0a)
			blt_f0a <= 0;
		if(blt_t0)
			blt_f0a <= 1;

		if(mr_clr | blt_t3a)
			blt_f3a <= 0;
		if(blt_t3)
			blt_f3a <= 1;

		if(mr_clr | blt_t5a)
			blt_f5a <= 0;
		if(blt_t5)
			blt_f5a <= 1;
	end

	/*
	 * FS
	 */
	reg fsf1;
	wire fsc = ir_fsc;
	wire fst0;
	wire fst0a;
	wire fst1;

	pa fs_pa0(.clk(clk), .reset(rst),
		.in(et3 & fsc),
		.p(fst0));
	pa fs_pa1(.clk(clk), .reset(rst),
		.in(et3 & fsc & ar[0]),
		.p(fst1));
	pa fs_pa2(.clk(clk), .reset(rst),
		.in(sat3 & fsf1),
		.p(fst0a));

	always @(posedge clk) begin
		if(ds_clr | fst0a)
			fsf1 <= 0;
		if(fst0)
			fsf1 <= 1;
	end

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
	wire ch_inc = (ir_ldci | ir_dpci | ir_cao) & ~chf5;
	wire ch_inc_op = ch_inc & ~chf7;
	wire ch_NOT_inc_op = (ir_ldc | ir_dpc) & ~chf5 |
		ch_inc & chf7;

	wire cht1;
	wire cht2;
	wire cht3;
	wire cht3a;
	wire cht4;
	wire cht4a;
	wire cht5;
	wire cht6;
	wire cht7;
	wire cht8;
	wire cht8a;
	wire cht8b;
	wire cht9;

	pa ch_pa0(.clk(clk), .reset(rst),
		.in(et0 & ch_inc_op),
		.p(cht1));
	pa ch_pa1(.clk(clk), .reset(rst),
		.in(cht1_D),
		.p(cht2));
	pa ch_pa2(.clk(clk), .reset(rst),
		.in(cht2 | cht4a),
		.p(cht3));
	pa ch_pa3(.clk(clk), .reset(rst),
		.in(sat3 & chf2),
		.p(cht3a));
	pa ch_pa4(.clk(clk), .reset(rst),
		.in(cht3a & ~sc[0]),
		.p(cht4));
	pa ch_pa5(.clk(clk), .reset(rst),
		.in(ar_t3 & chf3),
		.p(cht4a));
	pa ch_pa6(.clk(clk), .reset(rst),
		.in(cht3a & sc[0]),
		.p(cht5));
	pa ch_pa7(.clk(clk), .reset(rst),
		.in(et0 & ch_NOT_inc_op | cht5_D),
		.p(cht6));
	pa ch_pa8(.clk(clk), .reset(rst),
		.in(cht6_D),
		.p(cht7));
	pa ch_pa9(.clk(clk), .reset(rst),
		.in(cht7_D & ch_inc_op),
		.p(cht8));
	pa ch_pa10(.clk(clk), .reset(rst),
		.in(cht7_D & ch_NOT_inc_op |
		    mc_rs_t1 & chf6),
		.p(cht8b));
	pa ch_pa11(.clk(clk), .reset(rst),
		.in(sct2 & chf4),
		.p(cht8a));
	pa ch_pa12(.clk(clk), .reset(rst),
		.in(cht8a),
		.p(cht9));

	wire cht1_D, cht5_D, cht6_D, cht7_D, cht8a_D;
	dly100ns ch_dly0(.clk(clk), .reset(rst),
		.in(cht1),
		.p(cht1_D));
	dly100ns ch_dly1(.clk(clk), .reset(rst),
		.in(cht5),
		.p(cht5_D));
	dly150ns ch_dly2(.clk(clk), .reset(rst),
		.in(cht6),
		.p(cht6_D));
	dly100ns ch_dly3(.clk(clk), .reset(rst),
		.in(cht7),
		.p(cht7_D));
	dly100ns ch_dly4(.clk(clk), .reset(rst),
		.in(cht8a),
		.p(cht8a_D));

	always @(posedge clk) begin
		if(mp_clr | sat0)
			chf1 <= 0;
		if(cht1)
			chf1 <= 1;

		if(mp_clr | cht3a | cht8b)
			chf2 <= 0;
		if(cht3 | cht6)
			chf2 <= 1;

		if(mp_clr | cht4a)
			chf3 <= 0;
		if(cht4)
			chf3 <= 1;

		if(mp_clr | cht8a)
			chf4 <= 0;
		if(cht8b & ~ir_cao)
			chf4 <= 1;

		if(mp_clr)
			chf5 <= 0;
		if(cht9)
			chf5 <= 1;

		if(mp_clr | cht8b)
			chf6 <= 0;
		if(cht8)
			chf6 <= 1;

		if(ar_flag_clr | dct3 | lct0a |
		   et9 & jp_jsr)
			chf7 <= 0;
		if(cht9 |
		   ar_flag_set & mb[4])
			chf7 <= 1;
	end

	/*
	 * LC
	 */
	reg lcf1;
	wire ch_load = (ir_ldc | ir_ldci) & chf5;

	wire lct0;
	wire lct0a;

	pa lc_pa0(.clk(clk), .reset(rst),
		.in(et0 & ch_load),
		.p(lct0));
	pa lc_pa1(.clk(clk), .reset(rst),
		.in(sct2 & lcf1),
		.p(lct0a));

	always @(posedge clk) begin
		if(mp_clr | lct0a)
			lcf1 <= 0;
		if(lct0)
			lcf1 <= 1;
	end

	/*
	 * DC
	 */
	reg dcf1;
	wire ch_dep = (ir_dpc | ir_dpci) & chf5;

	wire dct0;
	wire dct0a;
	wire dct0b = dct0a;	// otherwise we get timing problems
	wire dct1;
	wire dct2;
	wire dct3;

	pa dc_pa0(.clk(clk), .reset(rst),
		.in(et0 & ch_dep),
		.p(dct0));
	pa dc_pa1(.clk(clk), .reset(rst),
		.in(sct2 & dcf1),
		.p(dct0a));
	pa dc_pa2(.clk(clk), .reset(rst),
		.in(dct0a_D),
		.p(dct1));
	pa dc_pa3(.clk(clk), .reset(rst),
		.in(dct1_D),
		.p(dct2));
	pa dc_pa4(.clk(clk), .reset(rst),
		.in(dct2_D),
		.p(dct3));

	wire dct0a_D, dct1_D, dct2_D;
	dly150ns dc_dly0(.clk(clk), .reset(rst),
		.in(dct0a),
		.p(dct0a_D));
	dly100ns dc_dly1(.clk(clk), .reset(rst),
		.in(dct1),
		.p(dct1_D));
	dly100ns dc_dly2(.clk(clk), .reset(rst),
		.in(dct2),
		.p(dct2_D));

	always @(posedge clk) begin
		if(mr_clr | dct0a)
			dcf1 <= 0;
		if(dct0)
			dcf1 <= 1;
	end

	/*
	 * SH
	 */
	reg shf1;
	wire shift_op =  ir_ash | ir_ashc |
		ir_lsh | ir_lshc |
		ir_rot | ir_rotc;
	wire sh_ac_2 = ir_ashc | ir_lshc | ir_rotc;

	wire sht0;
	wire sht1;
	wire sht1a;

	pa sh_p0(.clk(clk), .reset(rst),
		.in(et1 & shift_op & mb[18]),
		.p(sht0));
	pa sh_p1(.clk(clk), .reset(rst),
		.in(et3_D & shift_op),
		.p(sht1));
	pa sh_p2(.clk(clk), .reset(rst),
		.in(sct2 & shf1),
		.p(sht1a));

	wire et3_D;
	dly100ns sh_dly(.clk(clk), .reset(rst),
		.in(et3),
		.p(et3_D));

	always @(posedge clk) begin
		if(mp_clr | sht1a)
			shf1 <= 0;
		if(sht1)
			shf1 <= 1;
	end

	/*
	 * MP
	 */
	reg mpf1;
	reg mpf2;
	wire mp_clr = mr_clr;

	wire mpt0;
	wire mpt0a;
	wire mpt1;
	wire mpt2;

	pa mp_pa0(.clk(clk), .reset(rst),
		.in(et0 & ir_mul),
		.p(mpt0));
	pa mp_pa1(.clk(clk), .reset(rst),
		.in(mst6 & mpf1),
		.p(mpt0a));
	pa mp_pa2(.clk(clk), .reset(rst),
		.in(mpt0a_D & ~ir[6]),
		.p(mpt1));
	pa mp_pa3(.clk(clk), .reset(rst),
		.in(mpt1_D),
		.p(mpt2));

	wire mpt0a_D, mpt1_D, mpt2_D;
	dly200ns mp_dly0(.clk(clk), .reset(rst),
		.in(mpt0a),
		.p(mpt0a_D));
	dly100ns mp_dly1(.clk(clk), .reset(rst),
		.in(mpt1),
		.p(mpt1_D));
	dly100ns mp_dly2(.clk(clk), .reset(rst),
		.in(mpt2),
		.p(mpt2_D));

	always @(posedge clk) begin
		if(mp_clr | mpt0a)
			mpf1 <= 0;
		if(mpt0)
			mpf1 <= 1;

		if(mp_clr)
			mpf2 <= 0;
		if(mpt0 & ar[0] & mb[0])
			mpf2 <= 1;
	end

	/*
	 * FA
	 */
	reg faf1;
	reg faf2;
	reg faf3;
	reg faf4;

	wire fat0;
	wire fat1;
	wire fat1a;
	wire fat1b;
	wire fat2;
	wire fat3;
	wire fat4;
	wire fat5;
	wire fat5a;
	wire fat6;
	wire fat7;
	wire fat8;
	wire fat8a;
	wire fat9;
	wire fat10;

	pa fa_pa0(.clk(clk), .reset(rst),
		.in(et0 & ir_fad |
		    et4 & ir_fsb),
		.p(fat0));
	pa fa_pa1(.clk(clk), .reset(rst),
		.in(fat0_D),
		.p(fat1));
	pa fa_pa2(.clk(clk), .reset(rst),
		.in(sat3 & faf2),
		.p(fat1a));
	pa fa_pa3(.clk(clk), .reset(rst),
		.in(fat1_D),
		.p(fat1b));
	pa fa_pa4(.clk(clk), .reset(rst),
		.in(fat1a & ~sc[0]),
		.p(fat2));
	pa fa_pa5(.clk(clk), .reset(rst),
		.in(fat2_D),
		.p(fat3));
	pa fa_pa6(.clk(clk), .reset(rst),
		.in(fat1a & sc[0]),
		.p(fat4));
	pa fa_pa7(.clk(clk), .reset(rst),
		.in((fat3_D | fat4_D) & sc0_2_eq_7),
		.p(fat5));
	pa fa_pa8(.clk(clk), .reset(rst),
		.in(sct2 & faf3 |
		    fat6),
		.p(fat5a));
	pa fa_pa9(.clk(clk), .reset(rst),
		.in((fat3_D | fat4_D) & ~sc0_2_eq_7),
		.p(fat6));
	pa fa_pa10(.clk(clk), .reset(rst),
		.in(fat5a_D),
		.p(fat7));
	pa fa_pa11(.clk(clk), .reset(rst),
		.in(fat7_D),
		.p(fat8));
	pa fa_pa12(.clk(clk), .reset(rst),
		.in(fat8_D),
		.p(fat8a));
	pa fa_pa13(.clk(clk), .reset(rst),
		.in(fat8a_D),
		.p(fat9));
	pa fa_pa14(.clk(clk), .reset(rst),
		.in(ar_t3 & faf4),
		.p(fat10));

	wire fat0_D, fat1_D, fat2_D, fat3_D, fat4_D;
	wire fat5a_D, fat7_D, fat8_D, fat8a_D;
	dly100ns fa_dly0(.clk(clk), .reset(rst),
		.in(fat0),
		.p(fat0_D));
	dly50ns fa_dly1(.clk(clk), .reset(rst),
		.in(fat1),
		.p(fat1_D));
	dly150ns fa_dly2(.clk(clk), .reset(rst),
		.in(fat2),
		.p(fat2_D));
	dly150ns fa_dly3(.clk(clk), .reset(rst),
		.in(fat3),
		.p(fat3_D));
	dly100ns fa_dly4(.clk(clk), .reset(rst),
		.in(fat4),
		.p(fat4_D));
	dly100ns fa_dly5(.clk(clk), .reset(rst),
		.in(fat5a),
		.p(fat5a_D));
	dly100ns fa_dly6(.clk(clk), .reset(rst),
		.in(fat7),
		.p(fat7_D));
	dly50ns fa_dly7(.clk(clk), .reset(rst),
		.in(fat8),
		.p(fat8_D));
	dly100ns fa_dly8(.clk(clk), .reset(rst),
		.in(fat8a),
		.p(fat8a_D));

	always @(posedge clk) begin
		if(mp_clr | fat1b | fat10)
			faf1 <= 0;
		if(fat0 | fat5a)
			faf1 <= 1;

		if(mp_clr | fat1a)
			faf2 <= 0;
		if(fat1b)
			faf2 <= 1;

		if(mp_clr | fat5a)
			faf3 <= 0;
		if(fat5)
			faf3 <= 1;

		if(mp_clr | fat10)
			faf4 <= 0;
		if(fat9)
			faf4 <= 1;
	end

	/*
	 * FM
	 */
	reg fmf1;
	reg fmf2;

	wire fmt0;
	wire fmt0a;
	wire fmt0b;

	pa fm_pa0(.clk(clk), .reset(rst),
		.in(et0 & ir_fmp),
		.p(fmt0));
	pa fm_pa1(.clk(clk), .reset(rst),
		.in(fpt4 & fmf1),
		.p(fmt0a));
	pa fm_pa2(.clk(clk), .reset(rst),
		.in(mst6 & fmf2),
		.p(fmt0b));

	wire fmt0b_D;
	dly100ns fm_dly0(.clk(clk), .reset(rst),
		.in(fmt0b),
		.p(fmt0b_D));

	always @(posedge clk) begin
		if(mp_clr | fmt0a)
			fmf1 <= 0;
		if(fmt0)
			fmf1 <= 1;

		if(mp_clr | fmt0b)
			fmf2 <= 0;
		if(fmt0a)
			fmf2 <= 1;
	end

	/*
	 * FD
	 */
	reg fdf1;
	reg fdf2;

	wire fdt0;
	wire fdt0a;
	wire fdt0b;
	wire fdt1;

	pa fd_pa0(.clk(clk), .reset(rst),
		.in(et0 & ir_fdv),
		.p(fdt0));
	pa fd_pa1(.clk(clk), .reset(rst),
		.in(fpt4 & fdf1),
		.p(fdt0a));
	pa fd_pa2(.clk(clk), .reset(rst),
		.in(dst21a & fdf2),
		.p(fdt0b));
	pa fd_pa3(.clk(clk), .reset(rst),
		.in(fdt0b_D),
		.p(fdt1));

	wire fdt0b_D;
	dly100ns fd_dly0(.clk(clk), .reset(rst),
		.in(fdt0b),
		.p(fdt0b_D));

	always @(posedge clk) begin
		if(mp_clr | fdt0a)
			fdf1 <= 0;
		if(fdt0)
			fdf1 <= 1;

		if(mp_clr | fdt0b)
			fdf2 <= 0;
		if(fdt0a)
			fdf2 <= 1;
	end

	/*
	 * FP
	 */
	reg fpf1;
	reg fpf2;
	wire fp_ar0_xor_fmf1 = ar[0] ^ fmf1;
	wire fp_ar0_xor_mb0_xor_fmf1 = fp_ar0_xor_fmf1 ^ mb[0];
	wire fp_mb0_eq_fmf1 = mb[0] == fmf1;

	wire fpt0;
	wire fpt01;
	wire fpt1;
	wire fpt1a;
	wire fpt1aa;
	wire fpt1b;
	wire fpt2;
	wire fpt3;
	wire fpt4;

	pa fp_pa0(.clk(clk), .reset(rst),
		.in(fmt0 | fdt0),
		.p(fpt0));
	pa fp_pa1(.clk(clk), .reset(rst),
		.in(fpt0),
		.p(fpt01));
	pa fp_pa2(.clk(clk), .reset(rst),
		.in(fpt01_D),
		.p(fpt1));
	pa fp_pa3(.clk(clk), .reset(rst),
		.in(sat3 & fpf1),
		.p(fpt1a));
	pa fp_pa4(.clk(clk), .reset(rst),
		.in(fpt1a_D),
		.p(fpt1aa));
	pa fp_pa5(.clk(clk), .reset(rst),
		.in(sat3 & fpf2),
		.p(fpt1b));
	pa fp_pa6(.clk(clk), .reset(rst),
		.in(fpt1a & fp_ar0_xor_mb0_xor_fmf1),
		.p(fpt2));
	pa fp_pa7(.clk(clk), .reset(rst),
		.in(fpt1b_D),
		.p(fpt3));
	pa fp_pa8(.clk(clk), .reset(rst),
		.in(fpt3_D),
		.p(fpt4));

	wire fpt01_D, fpt1a_D, fpt1b_D, fpt3_D;
	dly100ns fp_dly0(.clk(clk), .reset(rst),
		.in(fpt01),
		.p(fpt01_D));
	dly100ns fp_dly1(.clk(clk), .reset(rst),
		.in(fpt1a),
		.p(fpt1a_D));
	dly100ns fp_dly2(.clk(clk), .reset(rst),
		.in(fpt1b),
		.p(fpt1b_D));
	dly100ns fp_dly3(.clk(clk), .reset(rst),
		.in(fpt3),
		.p(fpt3_D));

	always @(posedge clk) begin
		if(mp_clr | fpt1a)
			fpf1 <= 0;
		if(fpt1)
			fpf1 <= 1;

		if(mp_clr | fpt1b)
			fpf2 <= 0;
		if(fpt1aa)
			fpf2 <= 1;
	end

	/*
	 * MS
	 */
	reg msf1;
	wire ms_mult = mpf1 | fmf2;

	wire mst1;
	wire mst2;
	wire mst3;
	wire mst3a;
	wire mst4;
	wire mst5;
	wire mst6;

	pa ms_pa0(.clk(clk), .reset(rst),
		.in(mpt0 | fmt0a),
		.p(mst1));
	pa ms_pa1(.clk(clk), .reset(rst),
		.in(((mst1_D1 | mst2_D) & mq35_eq_mq36 | mst3a) & ~sc_eq_777),
		.p(mst2));
	pa ms_pa2(.clk(clk), .reset(rst),
		.in((mst1_D1 | mst2_D) & ~mq[35] & mq36),
		.p(mst3));
	pa ms_pa3(.clk(clk), .reset(rst),
		.in((mst1_D1 | mst2_D) & mq[35] & ~mq36),
		.p(mst4));
	pa ms_pa4(.clk(clk), .reset(rst),
		.in(ar_t3 & msf1),
		.p(mst3a));
	pa ms_pa5(.clk(clk), .reset(rst),
		.in((mst2_D & mq35_eq_mq36 | mst3a) & sc_eq_777),
		.p(mst5));
	pa ms_pa6(.clk(clk), .reset(rst),
		.in(mst5_D),
		.p(mst6));

	wire mst1_D0, mst1_D1;
	wire mst2_D, mst5_D;
	dly50ns ms_dly0(.clk(clk), .reset(rst),
		.in(mst1),
		.p(mst1_D0));
	dly200ns ms_dly1(.clk(clk), .reset(rst),
		.in(mst1),
		.p(mst1_D1));
	dly150ns ms_dly2(.clk(clk), .reset(rst),
		.in(mst2),
		.p(mst2_D));
	dly100ns ms_dly3(.clk(clk), .reset(rst),
		.in(mst5),
		.p(mst5_D));

	always @(posedge clk) begin
		if(mp_clr | mst3a)
			msf1 <= 0;
		if(mst3 | mst4)
			msf1 <= 1;
	end

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
	wire ds_div = ir_div & ir[6];
	wire ds_divi = ir_div & ~ir[6];
	wire dsf7_xor_mq0 = dsf7 ^ mq[0];

	wire ds_clr;
	wire dst0;
	wire dst0a;
	wire dst1;
	wire dst2;
	wire dst3;
	wire dst4;
	wire dst5;
	wire dst5a;
	wire dst6;
	wire dst7;
	wire dst8;
	wire dst9;
	wire dst10;
	wire dst10a;
	wire dst10b;
	wire dst11;
	wire dst11a;
	wire dst12;
	wire dst13;
	wire dst14a;
	wire dst14b;
	wire dst14;
	wire dst15;
	wire dst16;
	wire dst17;
	wire dst17a;
	wire dst18;
	wire dst19;
	wire dst19a;
	wire dst19b;
	wire dst20;
	wire dst21;
	wire dst21a;

	wire ds_div_t0;

	pa ds_pa0(.clk(clk), .reset(rst),
		.in(mr_clr),
		.p(ds_clr));
	pa ds_pa1(.clk(clk), .reset(rst),
		.in(et0 & ir_div),
		.p(ds_div_t0));
	pa ds_pa2(.clk(clk), .reset(rst),
		.in((et0 & ds_divi | fdt0a) & ar[0]),
		.p(dst0));
	pa ds_pa3(.clk(clk), .reset(rst),
		.in(ar_t3 & dsf1),
		.p(dst0a));
	pa ds_pa4(.clk(clk), .reset(rst),
		.in((et0 & ~ar[0] | dst0a) & ds_divi),
		.p(dst1));
	pa ds_pa5(.clk(clk), .reset(rst),
		.in(dst1_D),
		.p(dst2));
	pa ds_pa6(.clk(clk), .reset(rst),
		.in(et0 & ar[0] & ds_div),
		.p(dst3));
	pa ds_pa7(.clk(clk), .reset(rst),
		.in(dst3_D),
		.p(dst4));
	pa ds_pa8(.clk(clk), .reset(rst),
		.in(dst4_D),
		.p(dst5));
	pa ds_pa9(.clk(clk), .reset(rst),
		.in(ar_t3 & dsf2),
		.p(dst5a));
	pa ds_pa10(.clk(clk), .reset(rst),
`ifdef FIX_DS
		.in(dst5a_D & ~ar_eq_0),
`else
		.in(dst5a_D & ~ar_cry1),
`endif
		.p(dst6));
	pa ds_pa11(.clk(clk), .reset(rst),
		.in(dst6_D),
		.p(dst7));
	pa ds_pa12(.clk(clk), .reset(rst),
`ifdef FIX_DS
		.in(dst5a_D & ar_eq_0),
`else
		.in(dst5a_D & ar_cry1),
`endif
		.p(dst8));
	pa ds_pa13(.clk(clk), .reset(rst),
		.in(dst8_D),
		.p(dst9));
	pa ds_pa14(.clk(clk), .reset(rst),
		.in(ar_t3 & dsf3 |
		    fdt0a & ~ar[0] |
		    dst0a & ~ds_divi |
		    et0 & ds_div & ~ar[0] |
		    dst2 | dst7),
		.p(dst10));
	pa ds_pa15(.clk(clk), .reset(rst),
		.in(dst10_D & ir_fdv),
		.p(dst10a));
	pa ds_pa16(.clk(clk), .reset(rst),
		.in(dst10_D & ir_div),
		.p(dst10b));
	pa ds_pa17(.clk(clk), .reset(rst),
		.in((dst10a_D | dst10b_D) & mb[0]),
		.p(dst11));
	pa ds_pa18(.clk(clk), .reset(rst),
		.in((dst10a_D | dst10b_D) & ~mb[0]),
		.p(dst12));
	pa ds_pa19(.clk(clk), .reset(rst),
		.in(ar_t3 & dsf4),
		.p(dst11a));
	pa ds_pa20(.clk(clk), .reset(rst),
		.in(dst11a & ~ar[0]),
		.p(dst13));
	pa ds_pa21(.clk(clk), .reset(rst),
		.in(ar_t3 & dsf5 | 
		    dst11a & ar[0]),
		.p(dst14a));
	pa ds_pa22(.clk(clk), .reset(rst),
		.in(dst14a),
		.p(dst14b));
	pa ds_pa23(.clk(clk), .reset(rst),
		.in(dst14b_D & ~sc_eq_777 & mq35_xor_mb0),
		.p(dst14));
	pa ds_pa24(.clk(clk), .reset(rst),
		.in(dst14b_D & ~sc_eq_777 & ~mq35_xor_mb0),
		.p(dst15));
	pa ds_pa25(.clk(clk), .reset(rst),
		.in(dst14b_D & sc_eq_777),
		.p(dst16));
	pa ds_pa26(.clk(clk), .reset(rst),
		.in(dst16_D & ar[0] & ~mb[0]),
		.p(dst17));
	pa ds_pa27(.clk(clk), .reset(rst),
		.in(dst16_D & ar[0] & mb[0]),
		.p(dst18));
	pa ds_pa28(.clk(clk), .reset(rst),
		.in(dst16_D & ~ar[0] |
		    ar_t3 & dsf6),
		.p(dst17a));
	pa ds_pa29(.clk(clk), .reset(rst),
		.in(dst17a & dsf7),
		.p(dst19));
	pa ds_pa30(.clk(clk), .reset(rst),
		.in(dst17a & ~dsf7 |
		    ar_t3 & dsf8),
		.p(dst19a));
	pa ds_pa31(.clk(clk), .reset(rst),
		.in(dst19_D),
		.p(dst19b));
	pa ds_pa32(.clk(clk), .reset(rst),
		.in(dst19a_D),
		.p(dst20));
	pa ds_pa33(.clk(clk), .reset(rst),
		.in(dst20_D & dsf7_xor_mq0),
		.p(dst21));
	pa ds_pa34(.clk(clk), .reset(rst),
		.in(dst20_D & ~dsf7_xor_mq0 |
		    ar_t3 & dsf9),
		.p(dst21a));


	wire dst1_D, dst3_D, dst4_D, dst5a_D, dst6_D, dst8_D;
	wire dst10_D, dst10a_D, dst10b_D, dst14b_D, dst16_D;
	wire dst19_D, dst19a_D, dst20_D;
	dly150ns ds_dly0(.clk(clk), .reset(rst),
		.in(dst1),
		.p(dst1_D));
	dly100ns ds_dly1(.clk(clk), .reset(rst),
		.in(dst3),
		.p(dst3_D));
	dly100ns ds_dly2(.clk(clk), .reset(rst),
		.in(dst4),
		.p(dst4_D));
	dly100ns ds_dly3(.clk(clk), .reset(rst),
		.in(dst5a),
		.p(dst5a_D));
	dly100ns ds_dly4(.clk(clk), .reset(rst),
		.in(dst6),
		.p(dst6_D));
	dly100ns ds_dly5(.clk(clk), .reset(rst),
		.in(dst8),
		.p(dst8_D));
	dly100ns ds_dly6(.clk(clk), .reset(rst),
		.in(dst10),
		.p(dst10_D));
	dly200ns ds_dly7(.clk(clk), .reset(rst),
		.in(dst10a),
		.p(dst10a_D));
	dly200ns ds_dly8(.clk(clk), .reset(rst),
		.in(dst10b),
		.p(dst10b_D));
	dly100ns ds_dly9(.clk(clk), .reset(rst),
		.in(dst14b),
		.p(dst14b_D));
	dly100ns ds_dly10(.clk(clk), .reset(rst),
		.in(dst16),
		.p(dst16_D));
	dly50ns ds_dly11(.clk(clk), .reset(rst),
		.in(dst19),
		.p(dst19_D));
	dly100ns ds_dly12(.clk(clk), .reset(rst),
		.in(dst19a),
		.p(dst19a_D));
	dly100ns ds_dly13(.clk(clk), .reset(rst),
		.in(dst20),
		.p(dst20_D));

	always @(posedge clk) begin
		if(ds_clr | dst0a)
			dsf1 <= 0;
		if(dst0)
			dsf1 <= 1;

		if(ds_clr | dst5a)
			dsf2 <= 0;
		if(dst5)
			dsf2 <= 1;

		if(ds_clr | dst10)
			dsf3 <= 0;
		if(dst9)
			dsf3 <= 1;

		if(ds_clr | dst11a)
			dsf4 <= 0;
		if(dst11 | dst12)
			dsf4 <= 1;

		if(ds_clr | dst14a)
			dsf5 <= 0;
		if(dst14 | dst15)
			dsf5 <= 1;

		if(ds_clr | dst17a)
			dsf6 <= 0;
		if(dst17 | dst18)
			dsf6 <= 1;

		if(mr_clr)
			dsf7 <= 0;
		if(dst0 | dst3)
			dsf7 <= 1;

		if(ds_clr | dst19a)
			dsf8 <= 0;
		if(dst19b)
			dsf8 <= 1;

		if(ds_clr | dst21a)
			dsf9 <= 0;
		if(dst21)
			dsf9 <= 1;
	end

	/*
	 * NR
	 */
	reg nrf1;
	reg nrf2;
	reg nrf3;
	wire nr_ar9_eq_ar0 = ar[9] == ar[0];
	wire nr_round = ~nrf3 & mq[1] & ir[6];
	wire nr_ar_eq_0_AND_mq1_0 = ar == 0 & ~mq[1];

	wire nrt05;
	wire nrt0;
	wire nrt01;
	wire nrt1;
	wire nrt2;
	wire nrt3;
	wire nrt31;
	wire nrt4;
	wire nrt5;
	wire nrt5a;
	wire nrt6;

	pa nr_pa0(.clk(clk), .reset(rst),
		.in(fdt1 | fat10 | nrt5a),
		.p(nrt05));
	pa nr_pa1(.clk(clk), .reset(rst),
		.in(nrt05_D),
		.p(nrt0));
	pa nr_pa2(.clk(clk), .reset(rst),
		.in(nrt0),
		.p(nrt01));
	pa nr_pa3(.clk(clk), .reset(rst),
		.in((fmt0b_D | nrt01_D) & ~nr_ar_eq_0_AND_mq1_0),
		.p(nrt1));
	pa nr_pa4(.clk(clk), .reset(rst),
		.in((nrt1_D | nrt2_D) & nr_ar9_eq_ar0 & ~ar_eq_fp_half),
		.p(nrt2));
	pa nr_pa5(.clk(clk), .reset(rst),
		.in((nrt1_D | nrt2_D) & (~nr_ar9_eq_ar0 | ar_eq_fp_half)),
		.p(nrt3));
	pa nr_pa6(.clk(clk), .reset(rst),
		.in(nrt3),
		.p(nrt31));
	pa nr_pa7(.clk(clk), .reset(rst),
		.in(nrt31_D & ~nr_round),
		.p(nrt4));
	pa nr_pa8(.clk(clk), .reset(rst),
		.in(nrt31_D & nr_round),
		.p(nrt5));
	pa nr_pa9(.clk(clk), .reset(rst),
		.in(ar_t3 & nrf1),
		.p(nrt5a));
	pa nr_pa10(.clk(clk), .reset(rst),
		.in((fmt0b_D | nrt01_D) & nr_ar_eq_0_AND_mq1_0 |
		    nrt4 |
		    mpt0a & ir[6] |
		    mpt2_D),
		.p(nrt6));

	wire nrt05_D, nrt01_D, nrt1_D, nrt2_D, nrt31_D;
	dly100ns nr_dly0(.clk(clk), .reset(rst),
		.in(nrt05),
		.p(nrt05_D));
	dly200ns nr_dly1(.clk(clk), .reset(rst),
		.in(nrt01),
		.p(nrt01_D));
	dly100ns nr_dly2(.clk(clk), .reset(rst),
		.in(nrt1),
		.p(nrt1_D));
	dly150ns nr_dly3(.clk(clk), .reset(rst),
		.in(nrt2),
		.p(nrt2_D));
	dly100ns nr_dly4(.clk(clk), .reset(rst),
		.in(nrt31),
		.p(nrt31_D));

	always @(posedge clk) begin
		if(mp_clr | nrt5a)
			nrf1 <= 0;
		if(nrt5)
			nrf1 <= 1;

		if(mp_clr) begin
			nrf2 <= 0;
			nrf3 <= 0;
		end
		if(nrt05 | fdt0b | fmt0b)
			nrf2 <= 1;
		if(nrt5a)
			nrf3 <= 1;
	end

	/*
	 * MA
	 */
	// reg [18:35] ma;
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
		et3 & ma_fm_mbrt_et3 |
		et10 & ma_reset_et9;
	wire ma_fm_pc1 = it1 & ~pi_cyc;
	wire ma31_cry_in_en = ~s_ac_2 & ~f_ac_2;
	wire [30:35] maN_set;

	wire ma_eq_mas = ma == mas;
	wire ma18_31_eq_0 = ma[18:31] == 0;
	wire ma_fmc_select = ~key_rim_sbr & ma18_31_eq_0;
	wire ma_fm_mbrt_et3 = jp_popj | ir_blt;
	wire ma_clr_et1 = jp_popj | ex_ir_uuo | ir_blt;
	wire ma_reset_et9 = jp_push | jp_pushj | ir_jrst;
	wire ma_fm_ir14_17 = at2;
	wire ma_fm_ir9_12 = ft1 | st5;
	wire ma_fm_pich = it1 & pi_cyc;

	assign maN_set[30] = ma_fm_pich |
		et3 & ex_ir_uuo;
	assign maN_set[31] = 0;
	assign maN_set[32] = ma_fm_pich & pi_enc_32 |
		ma_fm_ir14_17 & ir[14] |
		ma_fm_ir9_12 & ir[9];
	assign maN_set[33] = ma_fm_pich & pi_enc_33 |
		ma_fm_ir14_17 & ir[15] |
		ma_fm_ir9_12 & ir[10];
	assign maN_set[34] = ma_fm_pich & pi_enc_34 |
		ma_fm_ir14_17 & ir[16] |
		ma_fm_ir9_12 & ir[11];
	assign maN_set[35] =
		ma_fm_ir14_17 & ir[17] |
		ma_fm_ir9_12 & ir[12];

	always @(posedge clk) begin: mactl
		integer i;
		if(ma_clr)
			ma <= 0;
		if(ma_inc)
			{ma32_cry_out, ma[32:35]} <= ma[32:35] + 4'b1;
		if(ma32_cry_out) begin
			if(ma31_cry_in_en)
				ma[18:31] <= ma[18:31] + 14'b1;
			ma32_cry_out <= 1'b0;
		end
		if(ma_fm_mbrt1)
			ma <= ma | mb[18:35];
		if(ma_fm_pc1)
			ma <= ma | pc;
		if(key_ma_fm_masw1)
			ma <= ma | mas;
		for(i = 30; i <= 35; i = i + 1)
			if(maN_set[i]) ma[i] <= 1;
	end

	/*
	 * PR
	 */
	// reg [18:25] pr;
	wire pr18_ok = ma[18:25] <= pr;
	wire pr_rel_AND_ma_ok = ~ex_inh_rel & pr18_ok;
	wire pr_rel_AND_NOT_ma_ok = ~ex_inh_rel & ~pr18_ok;

	/*
	 * RLR, RLA
	 */
	// reg [18:25] rlr;
	// wire [18:25] rla = ma[18:25] + (ex_inh_rel ? 0 : rlr);
	assign rla = ma[18:25] + (ex_inh_rel ? 8'b0 : rlr);

	always @(posedge clk) begin
		if(ex_clr) begin
			pr <= 0;
			rlr <= 0;
		end
		if(ex_set) begin
			pr <= pr | iob[0:7];
			rlr <= rlr | iob[18:25];
		end
	end

	/*
	 * MI
	 */
	// reg [0:35] mi;
	wire milt_clr = mi_clr;
	wire milt_fm_mblt1 = mi_fm_mb1;
	wire mirt_clr = mi_clr;
	wire mirt_fm_mbrt1 = mi_fm_mb1;

	wire mi_clr;
	wire mi_fm_mb1;

	pa mi_pa0(.clk(clk), .reset(rst),
		.in(mc_rs_t1 & key_ex_OR_dep_nxt |
		    mc_wr_rs & ma_eq_mas |
		    mai_rd_rs & ma_eq_mas),
		.p(mi_clr));
	pa mi_pa1(.clk(clk), .reset(rst),
		.in(mi_clr_D),
		.p(mi_fm_mb1));

	wire mi_clr_D;
	dly100ns mi_dly0(.clk(clk), .reset(rst),
		.in(mi_clr),
		.p(mi_clr_D));

	always @(posedge clk) begin
		if(milt_clr)
			mi[0:17] <= 0;
		if(mirt_clr)
			mi[18:35] <= 0;
		if(milt_fm_mblt1)
			mi[0:17] <= mi[0:17] | mb[0:17];
		if(mirt_fm_mbrt1)
			mi[18:35] <= mi[18:35] | mb[18:35];
	end

	/*
	 * MC
	 */
	reg mc_rd;
	reg mc_wr;
	reg mc_rq;
	// reg mc_stop;
	reg mc_stop_sync;
	reg mc_split_cyc_sync;
	wire mc_dr_split = 0;	// we don't support drums right now
	wire mc_sw_stop = key_mem_stop | sw_addr_stop | mc_dr_split;
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

	pg mc_pg0(.clk(clk), .reset(rst),
		.in(membus_addr_ack), .p(mai_addr_ack));
	pg mc_pg1(.clk(clk), .reset(rst),
		.in(membus_rd_rs), .p(mai_rd_rs));

	pa mc_pa0(.clk(clk), .reset(rst),
		.in(it1 | at2 | at5 |
		    ft1 | ft4 | ft6 |
		    key_rd | uuo_t2 | mc_split_rd_rq),
		.p(mc_rd_rq_pulse));
	pa mc_pa1(.clk(clk), .reset(rst),
		.in(st1 | st5 | st6 | key_wr |
		    mblt_fm_ir1_uuo_t0 | mc_split_wr_rq | blt_t0),
		.p(mc_wr_rq_pulse));
	pa mc_pa2(.clk(clk), .reset(rst),
		.in(ft7 & ~mc_split_cyc_sync),
		.p(mc_rdwr_rq_pulse));
	pa mc_pa3(.clk(clk), .reset(rst),
		.in(mc_rdwr_rq_pulse | mc_rd_rq_pulse | mc_wr_rq_pulse),
		.p(mc_rq_pulse));
	pa mc_pa4(.clk(clk), .reset(rst),
		.in(st2 | iot_t0 | cht8),
		.p(mc_rdwr_rs_pulse));
	pa mc_pa5(.clk(clk), .reset(rst),
		.in(ft7 & mc_split_cyc_sync),
		.p(mc_split_rd_rq));
	pa mc_pa6(.clk(clk), .reset(rst),
		.in(mc_rdwr_rs_pulse & mc_split_cyc_sync),
		.p(mc_split_wr_rq));
	pa mc_pa7(.clk(clk), .reset(rst),
		.in(mc_rd_rq_pulse | mc_rdwr_rq_pulse),
		.p(mc_mb_clr));
	pa mc_pa8(.clk(clk), .reset(rst),
		.in(mc_rq_pulse_D3 & mc_rq & ~mc_stop),
		.p(mc_non_exist_mem));
	pa mc_pa9(.clk(clk), .reset(rst),
		.in(mc_non_exist_mem & ~sw_mem_disable),
		.p(mc_non_exist_mem_rst));
	pa mc_pa10(.clk(clk), .reset(rst),
		.in(mc_non_exist_mem_rst & mc_rd),
		.p(mc_non_exist_rd));
	pa mc_pa11(.clk(clk), .reset(rst),
		.in(mc_rq_pulse_D0 & ex_inh_rel |
		    mc_rq_pulse_D1 & pr_rel_AND_ma_ok),
		.p(mc_rq_set));
	pa mc_pa12(.clk(clk), .reset(rst),
		.in(mc_rq_pulse_D1 & pr_rel_AND_NOT_ma_ok),
		.p(mc_illeg_address));
	pa mc_pa13(.clk(clk), .reset(rst),
		.in(mai_addr_ack | mc_non_exist_mem_rst),
		.p(mc_addr_ack));
	pa mc_pa14(.clk(clk), .reset(rst),
		.in(mc_addr_ack & ~mc_rd & mc_wr |
		    mc_rdwr_rs_pulse_D & ~mc_split_cyc_sync |
		    kt1 & key_manual & mc_stop & mc_stop_sync & ~key_mem_cont),
		.p(mc_wr_rs));
	pa mc_pa15(.clk(clk), .reset(rst),
		.in(kt1 & key_mem_cont & mc_stop |
		    ~mc_stop & (mc_wr_rs | mai_rd_rs | mc_non_exist_rd)),
		.p(mc_rs_t0));
	pa mc_pa16(.clk(clk), .reset(rst), .in(mc_rs_t0_D), .p(mc_rs_t1));

	bd  mc_bd0(.clk(clk), .reset(rst), .in(mc_wr_rs), .p(membus_wr_rs));
	bd2 mb_bd1(.clk(clk), .reset(rst), .in(mc_wr_rs), .p(mc_membus_fm_mb1));

	wire mc_rdwr_rs_pulse_D, mc_rs_t0_D;
	wire mc_rq_pulse_D0, mc_rq_pulse_D1, mc_rq_pulse_D2, mc_rq_pulse_D3;
	dly100ns mc_dly0(.clk(clk), .reset(rst),
		.in(mc_rdwr_rs_pulse),
		.p(mc_rdwr_rs_pulse_D));
	dly50ns mc_dly1(.clk(clk), .reset(rst),
		.in(mc_rq_pulse),
		.p(mc_rq_pulse_D0));
	dly150ns mc_dly2(.clk(clk), .reset(rst),
		.in(mc_rq_pulse),
		.p(mc_rq_pulse_D1));
	dly200ns mc_dly3(.clk(clk), .reset(rst),
		.in(mc_rq_pulse),
		.p(mc_rq_pulse_D2));
	dly100us mc_dly4(.clk(clk), .reset(rst), .in(mc_rq_pulse),
		.p(mc_rq_pulse_D3));
	dly50ns mc_dly5(.clk(clk), .reset(rst), .in(mc_rs_t0),
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
		if(mc_rq_pulse
`ifdef FIX_MEMSTOP
		| mc_rs_t0
`endif
		)
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

	wire iot_blki = ir_iot & ir[10:12] == 0;
	wire iot_datai = ir_iot & ir[10:12] == 1;
	wire iot_blko = ir_iot & ir[10:12] == 2;
	wire iot_datao = ir_iot & ir[10:12] == 3;
	wire iot_cono = ir_iot & ir[10:12] == 4;
	wire iot_coni = ir_iot & ir[10:12] == 5;
	wire iot_consz = ir_iot & ir[10:12] == 6;
	wire iot_conso = ir_iot & ir[10:12] == 7;

	wire iot_blk = iot_blki | iot_blko;
	wire iot_outgoing = iot_datao | iot_cono;
	wire iot_status = iot_coni | iot_consz | iot_conso;
	wire iot_datai_o = iot_datai | iot_datao;
	wire iot_init_setup;
	wire iot_restart;
	wire iot_final_setup;
	wire iot_reset;

	wire iot_t0;
	wire iot_t0a;
	wire iot_t2;
	wire iot_t3;
	wire iot_t3a;
	wire iot_t4;

	wire iot_go_P;
	pg iot_pg0(.clk(clk), .reset(rst),
		.in(iot_go & ~iot_reset),
		.p(iot_go_P));

	assign iobus_iob_poweron = sw_power;
	pa iot_pa0(.clk(clk), .reset(rst),
		.in(mr_start | cpa_cono_set & iob[19]),
		.p(iobus_iob_reset));
	pa iot_pa1(.clk(clk), .reset(rst),
		.in(iot_t2 & iot_cono),
		.p(iobus_cono_clear));
	pa iot_pa2(.clk(clk), .reset(rst),
		.in(iot_t3 & iot_cono),
		.p(iobus_cono_set));
	pa iot_pa3(.clk(clk), .reset(rst),
		.in(iot_t2 & iot_datao),
		.p(iobus_datao_clear));
	pa iot_pa4(.clk(clk), .reset(rst),
		.in(iot_t3 & iot_datao),
		.p(iobus_datao_set));
	pa iot_pa5(.clk(clk), .reset(rst),
		.in(et4 & iot_blk),
		.p(iot_t0));
	pa iot_pa6(.clk(clk), .reset(rst),
		.in(mc_rs_t1 & iot_f0a),
		.p(iot_t0a));
	assign iobus_iob_fm_datai = iot_datai & iot_drive;
	assign iobus_iob_fm_status = iot_status & iot_drive;
	wire iob_fm_ar1 = iot_outgoing & iot_drive;

	wire iot_t0a_D;
	dly200ns iot_dly0(.clk(clk), .reset(rst),
		.in(iot_t0a),
		.p(iot_t0a_D));
	ldly1us iot_dly1(.clk(clk), .reset(rst),
		.in(iot_go_P),
		.p(iot_t2),
		.l(iot_init_setup));
	ldly1_5us iot_dly2(.clk(clk), .reset(rst),
		.in(iot_t2),
		.p(iot_t3a),
		.l(iot_final_setup));
	ldly2us iot_dly3(.clk(clk), .reset(rst),
		.in(iot_t3a),
		.p(iot_t4),
		.l(iot_reset));
	ldly1us iot_dly4(.clk(clk), .reset(rst),
		.in(iot_t2),
		.p(iot_t3),
		.l(iot_restart));
	wire iot_drive = iot_init_setup | iot_final_setup | iot_t2;

	always @(posedge clk) begin
		if(mr_clr | iot_t2)
			iot_go <= 0;
		if(et4 & ir_iot & ~iot_blk)
			iot_go <= 1;

		if(mr_clr | iot_t0a)
			iot_f0a <= 0;
		if(iot_t0)
			iot_f0a <= 1;
	end

	/* IOB */
	assign iobus_iob_out = iob_fm_ar1 ? ar :
		cpa_status ? cpa_iob :
		pi_status ? pi_iob :
		0;
	wire [0:35] iob = iobus_iob_in;

	/*
	 * PIH, PIR, PIO
	 */
	// pih contains the currently serviced pi reqs.
	// lower channels override higher ones.
	// reg [1:7] pih;
	wire pih_clr = pi_reset;
	wire pih_fm_pi_ch_rq = et0a & pi_hold;
	wire pih0_fm_pi_ok1 = et1 & pi_rst;

	// pir contains all current and allowed pi reqs.
	// reg [1:7] pir;
	wire pir_clr = pi_reset;
	wire pir_fm_iob1 = pi_cono_set & iob[24];
	wire pir_stb;

	// pio is a mask of which pi reqs are allowed.
	// reg [1:7] pio;
	wire pio_fm_iob1 = pi_cono_set & iob[25];
	wire pio0_fm_iob1 = pi_cono_set & iob[26];

	// pi_req has the currently highest priority request.
	wire [1:7] pi_req;
	// pi_ok is used to mask out low priority reqs
	wire [1:8] pi_ok;

	// requests coming from the bus
	wire [1:7] iob_pi_req = iobus_pi_req | cpa_req;

	// TODO: this can be simpler, same as in KA10 code
	assign pi_ok[1] = pi_active;
	genvar i;
	generate
		for(i = 1; i <= 7; i = i + 1) begin: pi_reg_gen
			assign pi_req[i] =  pi_ok[i] & ~pih[i] &  pir[i];
			assign pi_ok[i+1] = pi_ok[i] & ~pih[i] & ~pir[i];
		end
	endgenerate

	always @(posedge clk) begin: pirctl
		integer i;
		if(pih_clr)
			pih <= 0;
		if(pih_fm_pi_ch_rq)
			pih <= pih | pi_req;
		if(pih0_fm_pi_ok1)
			pih <= pih & ~pi_ok[1:7];

		if(pir_clr)
			pir <= 0;
		else if(pir_fm_iob1)
			pir <= pir | iob[29:35];
		else if(pir_stb)
			for(i = 1; i <= 7; i = i+1) begin
				if(iob_pi_req[i] & pio[i])
					pir[i] <= 1;
			end
		else
if(pih)	// HACK so it can be set from testbench
			pir <= pir & ~pih;

		if(pi_reset)
			pio <= 0;
		if(pio_fm_iob1)
			pio <= pio | iob[29:35];
		if(pio0_fm_iob1)
			pio <= pio & ~iob[29:35];
	end

	/*
	 * PI
	 */
	reg pi_ov;
	reg pi_cyc;
	// reg pi_active;
	wire pi_select = iobus_ios == 1;
	wire pi_status = pi_select & iobus_iob_fm_status;
	wire pi_cono_set = pi_select & iobus_cono_set;
	wire pi_rq = | pi_req;
	wire pi_enc_32 = pi_req[4] | pi_req[5] | pi_req[6] | pi_req[7];
	wire pi_enc_33 = pi_req[2] | pi_req[3] | pi_req[6] | pi_req[7];
	wire pi_enc_34 = pi_req[1] | pi_req[3] | pi_req[5] | pi_req[7];
	// rst (= restore) means the request is to be dismissed
	wire pi_blk_rst = ~pi_ov & iot_datai_o;		// BLK hasn't completed
	wire pi_rst = ir_jrst & ir[9] | pi_cyc & pi_blk_rst;
	wire pi_hold = pi_cyc & (~ir_iot | pi_blk_rst);
	wire pi_sync;
	wire pi_reset;

	wire [0:35] pi_iob = { 28'b0, pi_active, pio };

	pa pi_pa0(.clk(clk), .reset(rst),
		.in(it0 | at0),
		.p(pi_sync));
	pa pi_pa1(.clk(clk), .reset(rst),
		.in(pi_sync & ~pi_cyc | blt_t4),
		.p(pir_stb));
	pa pi_pa2(.clk(clk), .reset(rst),
		.in(mr_start |
		    pi_select & iobus_cono_clear & iob[23]),
		.p(pi_reset));

	wire pi_sync_D;
	dly200ns pi_dly0(.clk(clk), .reset(rst),
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
		if(pi_reset | pi_cono_set & iob[27])
			pi_active <= 0;
		if(pi_cono_set & iob[28])
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
	wire cpa_cono_set = cpa & iobus_cono_set;
	wire cpa_status = cpa & iobus_iob_fm_status;
	wire cpa_clock_flag_set;

	clk60hz cpa_clk(.clk(clk),
		.outclk(cpa_clock_flag_set));

	wire cpa_req_enable = cpa_illeg_op | cpa_non_exist_mem | cpa_pdl_ov |
		cpa_clock_enable & cpa_clock_flag |
		cpa_pc_chg_enable & ar_pc_chg_flag |
		cpa_arov_enable & ar_ov_flag;
	wire [1:7] cpa_req;
	// TODO: improve this
	genvar j;
	generate
		for(j = 1; j <= 7; j = j + 1) begin: cpa_req_gen
			assign cpa_req[j] = cpa_req_enable & (cpa_pia == j);
		end
	endgenerate

	wire [0:35] cpa_iob = { 18'b0,
		1'b0, cpa_pdl_ov, cpa_iot_user, ex_user,
		cpa_illeg_op, cpa_non_exist_mem, 1'b0, cpa_clock_enable,
		cpa_clock_flag, 1'b0, cpa_pc_chg_enable, ar_pc_chg_flag,
		1'b0, cpa_arov_enable, ar_ov_flag,
		cpa_pia };

	always @(posedge clk) begin
		if(mr_start) begin
			cpa_iot_user <= 0;
			cpa_illeg_op <= 0;
			cpa_non_exist_mem <= 0;
			cpa_clock_enable <= 0;
			cpa_clock_flag <= 0;
			cpa_pc_chg_enable <= 0;
			cpa_pdl_ov <= 0;
			cpa_arov_enable <= 0;
			cpa_pia <= 0;
		end

`ifdef FIX_USER_IOT
		// 20 and 21 seem to be flipped
		// IO reset seems to reset it too
		if(cpa_cono_set & iob[20] | iobus_iob_reset)
			cpa_iot_user <= 0;
		if(cpa_cono_set & iob[21])
			cpa_iot_user <= 1;

		if(ar_flag_set & ~ex_user)
			cpa_iot_user <= mb[6];
`else
		if(cpa_cono_set & iob[21])
			cpa_iot_user <= 0;
		if(cpa_cono_set & iob[20])
			cpa_iot_user <= 1;
`endif

		if(cpa_cono_set & iob[22])
			cpa_illeg_op <= 0;
		if(mc_illeg_address)
			cpa_illeg_op <= 1;

		if(cpa_cono_set & iob[23])
			cpa_non_exist_mem <= 0;
		if(mc_non_exist_mem)
			cpa_non_exist_mem <= 1;

		if(cpa_cono_set & iob[24])
			cpa_clock_enable <= 0;
		if(cpa_cono_set & iob[25])
			cpa_clock_enable <= 1;
		if(cpa_cono_set & iob[26])
			cpa_clock_flag <= 0;
		if(cpa_clock_flag_set)
			cpa_clock_flag <= 1;

		if(cpa_cono_set & iob[27])
			cpa_pc_chg_enable <= 0;
		if(cpa_cono_set & iob[28])
			cpa_pc_chg_enable <= 1;
		if(cpa_cono_set & iob[18])
			cpa_pdl_ov <= 0;
		if(et10 & mb_ar_swap_et10 & ar_cry0)
			cpa_pdl_ov <= 1;

		if(cpa_cono_set & iob[30])
			cpa_arov_enable <= 0;
		if(cpa_cono_set & iob[31])
			cpa_arov_enable <= 1;

		if(cpa_cono_set)
			cpa_pia <= iob[33:35];
	end

endmodule
