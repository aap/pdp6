//`default_nettype none

module ar_reg(
	input clk,
	input arlt_clr,
	input arrt_clr,
	input arlt_fm_mb0,
	input arrt_fm_mb0,
	input arlt_fm_mb1,
	input arrt_fm_mb1,
	input arlt_fm_datasw1,
	input arrt_fm_datasw1,
	input [0:35] mb,
	input [0:35] datasw,
	output [0:35] ar
);
	reg [0:17] arlt;
	reg [18:35] arrt;

	always @(posedge clk) begin: arctl
		integer i;
		if(arlt_clr)
			arlt <= 0;
		if(arrt_clr)
			arrt <= 0;
		for(i = 0; i < 18; i = i+1) begin
			if(arlt_fm_mb0 & ~mb[i])
				arlt[i] <= 0;
			if(arlt_fm_mb1 & mb[i])
				arlt[i] <= 1;
			if(arrt_fm_mb0 & ~mb[i+18])
				arrt[i+18] <= 0;
			if(arrt_fm_mb1 & mb[i+18])
				arrt[i+18] <= 1;
		end
		if(arlt_fm_datasw1)
			arlt <= arlt | datasw[0:17];
		if(arlt_fm_datasw1)
			arrt <= arrt | datasw[18:35];
	end

	assign ar = {arlt, arrt};
endmodule

module mb_reg(
	input clk,
	input mblt_clr,
	input mbrt_clr,
	input mblt_fm_ar0,
	input mbrt_fm_ar0,
	input mblt_fm_ar1,
	input mbrt_fm_ar1,
	input mblt_fm_mbrtJ,
	input mbrt_fm_mbltJ,
	input [0:35] mbN_clr,
	input [0:35] mbN_set,
	input [0:35] ar,
	output [0:35] mb
);
	reg [0:17] mblt;
	reg [18:35] mbrt;

	always @(posedge clk) begin: mbctl
		integer i;
		if(mblt_clr)
			mblt <= 0;
		if(mbrt_clr)
			mbrt <= 0;
		for(i = 0; i < 18; i = i+1) begin
			if(mblt_fm_ar0 & ~ar[i])
				mblt[i] <= 0;
			if(mblt_fm_ar1 & ar[i])
				mblt[i] <= 1;
			if(mbrt_fm_ar0 & ~ar[i+18])
				mbrt[i+18] <= 0;
			if(mbrt_fm_ar1 & ar[i+18])
				mbrt[i+18] <= 1;
		end
		for(i = 0; i < 18; i = i+1) begin
			if(mbN_clr[i])
				mblt[i] <= 0;
			if(mbN_clr[i+18])
				mbrt[i+18] <= 0;
			if(mbN_set[i])
				mblt[i] <= 1;
			if(mbN_set[i+18])
				mbrt[i+18] <= 1;
		end
		if(mblt_fm_mbrtJ)
			mblt <= mbrt;
		if(mbrt_fm_mbltJ)
			mbrt <= mblt;
	end

	assign mb = {mblt, mbrt};
endmodule

module pc_reg(
	input clk,
	input pc_clr,
	input pc_inc,
	input pc_fm_ma1,
	input [18:35] ma,
	output [18:35] pc
);
	reg [18:35] pc;

	always @(posedge clk) begin
		if(pc_clr)
			pc <= 0;
		if(pc_inc)
			pc <= pc+1;
		if(pc_fm_ma1)
			pc <= pc | ma;
	end
endmodule

module ma_reg(
	input clk,
	input ma_clr,
	input ma_inc,
	input key_ma_fm_masw1,
	input ma_fm_pc1,
	input [18:35] mas,
	input [18:35] pc,
	output [18:35] ma
);
	reg [18:35] ma;
	reg en_cry;
	reg ma32_cry_out;

	initial
		en_cry = 0;

	always @(posedge clk) begin
		if(ma_clr)
			ma <= 0;
		if(ma_inc) begin
			{ma32_cry_out, ma[32:35]} = ma[32:35]+1;
			ma[18:31] = ma[18:31] + (ma32_cry_out & en_cry);
		end
		if(key_ma_fm_masw1)
			ma <= ma | mas;
	end
endmodule

module apr(
	input clk,
	input key_start,
	input key_read_in,
	input key_inst_cont,
	input key_mem_cont,
	input key_inst_stop,
	input key_mem_stop,
	input key_exec,
	input key_ioreset,
	input key_dep,
	input key_dep_nxt,
	input key_ex,
	input key_ex_nxt,
	input sw_addr_stop,
	input sw_mem_disable,
	input sw_repeat,
	input sw_power,
	input sw_rim_maint,
	output [0:35]  mi,
	input [18:35] mas,
	input [0:35]  datasw,

	output membus_mc_wr_rs,
	output membus_mc_rq_cyc,
	output membus_mc_rd_rq,
	output membus_mc_wr_rq,
	output [21:35] membus_ma,
	output [18:21] membus_sel,
	output membus_fmc_select,
	output [0:35] membus_mb_out,

	input  membus_mai_cmc_addr_ack,
	input  membus_mai_cmc_rd_rs,
	input  [0:35] membus_mb_in
);

	reg run;
	reg  [0:35]  mi;
	wire [18:35] ma;
	wire [18:35] pc;
	wire [0:35]  ar;
	wire [0:35]  mb;
	wire [0:35]  mbN_clr;
	wire [0:35]  mbN_set;

	// TMP
	assign st7 = 0;

	/******
	 * MA *
	 ******/

	ma_reg ma_reg(
		.clk(clk),
		.ma_clr(ma_clr),
		.ma_inc(ma_inc),
		.key_ma_fm_masw1(key_ma_fm_masw1),
		.mas(mas),
		.pc(pc),
		.ma(ma));
	assign ma_inc = key_ma_inc;
	assign ma_clr = key_ma_clr;

	/******
	 * PC *
	 ******/

	pc_reg pc_reg(
		.clk(clk),
		.pc_clr(pc_clr),
		.pc_inc(pc_inc),
		.pc_fm_ma1(pc_fm_ma1),
		.ma(ma),
		.pc(pc));
	assign pc_clr = kt1 & key_start_OR_read_in;
	assign pc_fm_ma1 = kt3 & key_start_OR_read_in;

	/******
	 * AR *
	 ******/

	ar_reg ar_reg(
		.clk(clk),
		.arlt_clr(arlt_clr),
		.arrt_clr(arrt_clr),
		.arlt_fm_mb0(arlt_fm_mb0),
		.arrt_fm_mb0(arrt_fm_mb0),
		.arlt_fm_mb1(arlt_fm_mb1),
		.arrt_fm_mb1(arrt_fm_mb1),
		.arlt_fm_datasw1(arlt_fm_datasw1),
		.arrt_fm_datasw1(arrt_fm_datasw1),
		.mb(mb),
		.datasw(datasw),
		.ar(ar));
	assign arlt_clr = key_ar_clr;
	assign arrt_clr = key_ar_clr;
	assign arlt_fm_mb0 = ar_fm_mbJ;
	assign arrt_fm_mb0 = ar_fm_mbJ;
	assign arlt_fm_mb1 = ar_fm_mbJ;
	assign arrt_fm_mb1 = ar_fm_mbJ;
	assign arlt_fm_datasw1 = key_ar_fm_datasw1;
	assign arrt_fm_datasw1 = key_ar_fm_datasw1;
	assign ar_fm_mbJ = mbJ_swap_arJ;

	/******
	 * MB *
	 ******/

	mb_reg mb_reg(
		.clk(clk),
		.mblt_clr(mblt_clr),
		.mbrt_clr(mbrt_clr),
		.mblt_fm_ar0(mblt_fm_ar0),
		.mbrt_fm_ar0(mbrt_fm_ar0),
		.mblt_fm_ar1(mblt_fm_ar1),
		.mbrt_fm_ar1(mbrt_fm_ar1),
		.mblt_fm_mbrtJ(mblt_fm_mbrtJ),
		.mbrt_fm_mbltJ(mbrt_fm_mbltJ),
		.mbN_clr(mbN_clr),
		.mbN_set(mbN_set),
		.ar(ar),
		.mb(mb));
	dly100ns mbdly1(.clk(clk), .in(mc_mb_clr), .out(mc_mb_clr_dly));
	assign mb_clr = mc_mb_clr_dly;
	assign mblt_clr = mb_clr;
	assign mbrt_clr = mb_clr;
	assign mblt_fm_ar0 = mb_fm_arJ;
	assign mbrt_fm_ar0 = mb_fm_arJ;
	assign mblt_fm_ar1 = mb_fm_arJ;
	assign mbrt_fm_ar1 = mb_fm_arJ;
	assign mbN_set = membus_mb_in & {36{mc_mb_membus_enable}};

	assign mblt_fm_mbrtJ = 0;
	assign mbrt_fm_mbltJ = 0;
	assign mb_fm_arJ = key_wr;
	assign mbJ_swap_arJ = 0;

	/******
	 * MI *
	 ******/

	assign mi_clr = (mai_rd_rs | mc_wr_rs) & ma == mas |
		mc_rs_t1 & key_ex_OR_dep_nxt;
	dly100ns midly0(.clk(clk), .in(mi_clr), .out(mi_fm_mb1));
	always @(posedge clk) begin
		if(mi_clr)
			mi <= 0;
		if(mi_fm_mb1)
			mi <= mi | mb;
	end

	/*******
	 * KEY *
	 *******/

	reg key_ex_st, key_dep_st, key_ex_sync, key_dep_sync;
	reg key_rim_sbr;

	// KEY-1
	assign key_clr_rim = ~(key_read_in | key_mem_cont | key_inst_cont); 
	assign key_ma_fm_mas = key_ex_sync | key_dep_sync |
		key_start_OR_read_in;
	assign key_execute = key_exec & ~run;
	assign key_start_OR_read_in = key_start | key_read_in;
	assign key_start_OR_cont_OR_read_in = key_start_OR_read_in |
		key_inst_cont;
	assign key_ex_OR_dep_nxt = key_ex_nxt | key_dep_nxt;
	assign key_dp_OR_dp_nxt = key_dep_sync | key_dep_nxt;
	assign key_ex_OR_ex_nxt = key_ex_sync | key_ex_nxt;
	assign key_ex_OR_dep_st = key_ex_st | key_dep_st;
	assign key_run1_AND_NOT_ex_OR_dep = run & ~key_ex_OR_dep_st;
	assign key_manual = key_ex | key_ex_nxt | key_dep | key_dep_nxt |
		key_start | key_inst_cont | key_mem_cont | key_ioreset |
		key_execute | key_read_in;
	assign key_run_AND_ex_OR_dep = run & key_ex_OR_dep_st;
	assign key_execute_OR_dp_OR_dp_nxt = key_execute | key_dp_OR_dp_nxt;

	syncpulse keysync1(clk, sw_power, mr_pwr_clr);
	syncpulse keysync2(clk, key_inst_stop, run_clr);

	always @(posedge clk) begin
		if(mr_pwr_clr | run_clr)
			run <= 0;
		if(kt0a | key_go) begin
			key_ex_st <= 0;
			key_dep_st <= 0;
		end
		if(kt0a) begin
			key_ex_sync <= key_ex;
			key_dep_sync <= key_dep;
		end
		if(key_go) begin
			key_ex_sync <= 0;
			key_dep_sync <= 0;
			run <= 1;
		end
	end


	// KEY-2
	assign mr_start = mr_pwr_clr | kt1 & key_ioreset;
	assign mr_clr = kt1 & key_manual & ~key_mem_cont |
		mr_pwr_clr;	// TODO: more
	assign key_ma_clr = kt1 & key_ma_fm_mas;
	assign key_ma_fm_masw1 = kt2 & key_ma_fm_mas;
	assign key_ma_inc = kt1 & key_ex_OR_dep_nxt;
	assign key_ar_clr = kt1 & key_execute_OR_dp_OR_dp_nxt;
	assign key_ar_fm_datasw1 = kt2 & key_execute_OR_dp_OR_dp_nxt |
		0;	// TODO: more
	assign key_rd = kt3 & key_ex_OR_ex_nxt;
	assign key_wr = kt3 & key_dp_OR_dp_nxt;

	syncpulse keysync3(clk, key_manual, kt0);

	assign kt0a = kt0;
	dly100ns keydly1(.clk(clk), .in(kt0a), .out(key_tmp1));
	assign kt1 = kt0a & key_mem_cont |
		key_tmp1 & ~run & ~key_mem_cont; // no key_mem_cont in the real hardware, why?
	dly200ns keydly2(.clk(clk), .in(kt1 & ~run), .out(kt2));
	dly200ns keydly3(.clk(clk), .in(kt2), .out(kt3));
	assign kt4 = kt3 & key_execute |
		key_rdwr_ret |
		mc_stop_set & key_mem_cont |
		st7 & key_start_OR_cont_OR_read_in;
	assign key_go = kt3 & key_start_OR_cont_OR_read_in |
		kt4 & key_run1_AND_NOT_ex_OR_dep;

	always @(posedge clk) begin
		if(kt1 & key_clr_rim)
			key_rim_sbr <= 0;
		if(kt1 & key_read_in | sw_rim_maint)
			key_rim_sbr <= 1;
	end

	sbr key_rdwr(
		.clk(clk),
		.clr(mr_clr),
		.set(key_rd | key_wr),
		.from(mc_rs_t1),
		.ret(key_rdwr_ret));


	/******
	 * MC *
	 ******/

	reg mc_rd, mc_wr, mc_rq, mc_stop, mc_stop_sync, mc_split_cyc_sync;

	assign mc_mb_membus_enable = mc_rd;
	assign membus_mc_rq_cyc = mc_rq & (mc_rd | mc_wr);
	assign membus_mc_rd_rq = mc_rd;
	assign membus_mc_wr_rq = mc_wr;
	assign membus_ma = {rla[21:25], ma[26:35]};
	assign membus_sel = rla[18:21]; 
	assign membus_fmc_select = ~key_rim_sbr & ma[18:31] == 0;
	assign membus_mc_wr_rs = mc_wr_rs;
	assign membus_mb_out = mc_wr_rs_b ? mb : 0;

	// pulses
	assign mc_mb_clr = mc_rd_rq_pulse |
		mc_rdwr_rq_pulse;
	assign mc_rd_rq_pulse = key_rd; // TODO
	assign mc_wr_rq_pulse = key_wr |
		mc_rdwr_rs_pulse & mc_split_cyc_sync; // TODO
	assign mc_split_rd_rq = 0;
	assign mc_rdwr_rq_pulse = 0;//key_rd; // TODO
	assign mc_rdwr_rs_pulse = 0;//key_wr;
	assign mc_split_wr_rq = 0;
	assign mc_rq_pulse = mc_rd_rq_pulse |
		mc_wr_rq_pulse |
		mc_rdwr_rq_pulse;

	dly100us mcdly1(.clk(clk), .in(mc_rq_pulse), .out(mc_tmp3));
	assign mc_non_exist_mem = mc_tmp3 & mc_rq & ~mc_stop;
	assign mc_non_exist_mem_rst = mc_non_exist_mem & ~sw_mem_disable;
	assign mc_non_exist_rd = mc_non_exist_mem_rst & mc_rd;
	syncpulse syncmc1(.clk(clk), .in(membus_mai_cmc_addr_ack), .out(mai_addr_ack));
	assign mc_addr_ack = mai_addr_ack | mc_non_exist_mem_rst;
	syncpulse syncmc2(.clk(clk), .in(membus_mai_cmc_rd_rs), .out(mai_rd_rs));
	assign mc_rs_t0 = kt1 & key_mem_cont & mc_stop |
		(mc_wr_rs | mc_non_exist_rd | mai_rd_rs) & ~mc_stop;
	dly50ns mcdly2(.clk(clk), .in(mc_rs_t0), .out(mc_rs_t1));
	syncpulse syncmc3(.clk(clk), .in(| membus_mb_in), .out(mb_pulse));
	assign mc_wr_rs =
		kt1 & key_manual & mc_stop & mc_stop_sync & ~key_mem_cont |
		mc_addr_ack & mc_wr & ~mc_rd |
		mc_tmp1 & ~mc_split_cyc_sync;
	pa100ns mcpa0(.clk(clk), .in(mc_wr_rs), .out(mc_wr_rs_b));
	dly100ns mcdly3(.clk(clk), .in(mc_rdwr_rs_pulse), .out(mc_tmp1));
	dly200ns mcdly4(.clk(clk), .in(mc_rq_pulse), .out(mc_tmp2));
	assign mc_stop_set = mc_tmp2 &
		(key_mem_stop | ma == mas & sw_addr_stop);
	dly50ns mcdly5(.clk(clk), .in(mc_rq_pulse), .out(mc_tmp4));
	dly150ns mcdly6(.clk(clk), .in(mc_rq_pulse), .out(mc_tmp5));
	assign mc_rq_set = mc_tmp4 & ex_inh_rel |
		mc_tmp5 & pr_rel_AND_ma_ok;
	assign mc_illeg_address = mc_tmp5 & pr_rel_AND_NOT_ma_ok;
	// TODO: 100ns -> ST7

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
		// TODO: set mc_split_cyc_sync
	end

	/*
	 * Memory relocation
	 */

	reg [18:25] pr;
	reg [18:25] rlr;
	wire [18:25] rla;

	assign rla = ma[18:25] + (ex_inh_rel ? 0 : rlr);
	assign pr18ok  = ma[18:25] <= pr;
	assign pr_rel_AND_ma_ok = pr18ok & ~ex_inh_rel;
	assign pr_rel_AND_NOT_ma_ok = ~pr18ok & ~ex_inh_rel;

	/******
	 * EX *
	 ******/

	reg ex_user, ex_mode_sync, ex_uuo_sync, ex_pi_sync, ex_ill_op;

	assign ex_inh_rel = ~ex_user | ex_pi_sync | (ma[18:31] == 0) | ex_ill_op;
	assign ex_clr = mr_start |
		0; // TODO

	always @(posedge clk) begin
		if(mr_clr) begin
			if(ex_mode_sync)
				ex_user <= 1;
			ex_mode_sync <= 0;
			ex_uuo_sync <= 0;
			if(~pi_cyc)
				ex_pi_sync <= 0;
		end
		if(mr_start) begin
			ex_user <= 0;
			ex_ill_op <= 0;
		end
		if(ex_clr) begin
			pr <= 0;
			rlr <= 0;
		end
	end

	/******
	 * PI *
	 ******/

	reg pi_ov, pi_cyc, pi_active;

	assign pi_reset = mr_start |
		0;	// TODO

	always @(posedge clk) begin
		if(mr_start) begin
			pi_ov <= 0;
			pi_cyc <= 0;
		end
		if(pi_reset)
			pi_active <= 0;
	end

	/*******
	 * CPA *
	 *******/

	reg cpa_iot_user, cpa_illeg_op, cpa_non_exist_mem;
	reg cpa_clock_enable, cpa_clock_flag;
	reg cpa_pc_chg_enable, cpa_pdl_ov;
	reg cpa_arov_enable;
	reg [33:35] cpa_pia;

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
		if(mc_illeg_address)
			cpa_illeg_op <= 1;
		if(mc_non_exist_mem)
			cpa_non_exist_mem <= 1;
	end

endmodule
