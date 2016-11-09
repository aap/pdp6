module coremem16k(
	input clk,

	input  mc_wr_rs_p0,
	input  mc_rq_cyc_p0,
	input  mc_rd_rq_p0,
	input  mc_wr_rq_p0,
	input  [21:35] ma_p0,
	input  [18:21] sel_p0,
	input  fmc_select_p0,
	input  [0:35] mb_in_p0,
	output cmc_addr_ack_p0,
	output cmc_rd_rs_p0,
	output [0:35] mb_out_p0,

	input  mc_wr_rs_p1,
	input  mc_rq_cyc_p1,
	input  mc_rd_rq_p1,
	input  mc_wr_rq_p1,
	input  [21:35] ma_p1,
	input  [18:21] sel_p1,
	input  fmc_select_p1,
	input  [0:35] mb_in_p1,
	output cmc_addr_ack_p1,
	output cmc_rd_rs_p1,
	output [0:35] mb_out_p1,

	input  mc_wr_rs_p2,
	input  mc_rq_cyc_p2,
	input  mc_rd_rq_p2,
	input  mc_wr_rq_p2,
	input  [21:35] ma_p2,
	input  [18:21] sel_p2,
	input  fmc_select_p2,
	input  [0:35] mb_in_p2,
	output cmc_addr_ack_p2,
	output cmc_rd_rs_p2,
	output [0:35] mb_out_p2,

	input  mc_wr_rs_p3,
	input  mc_rq_cyc_p3,
	input  mc_rd_rq_p3,
	input  mc_wr_rq_p3,
	input  [21:35] ma_p3,
	input  [18:21] sel_p3,
	input  fmc_select_p3,
	input  [0:35] mb_in_p3,
	output cmc_addr_ack_p3,
	output cmc_rd_rs_p3,
	output [0:35] mb_out_p3
);

	wire [21:35] ma;
	wire [0:35] mb_out;
	wire [0:35] mb_in;
	wire cmc_power_start;

	reg power;
	reg single_step_sw;
	reg restart_sw;
	reg [0:3] memsel_p0;
	reg [0:3] memsel_p1;
	reg [0:3] memsel_p2;
	reg [0:3] memsel_p3;

	reg cmc_p0_act, cmc_p1_act, cmc_p2_act, cmc_p3_act;
	reg cmc_last_proc;
	reg cmc_aw_rq, cmc_proc_rs, cmc_pse_sync, cmc_stop;
	reg cmc_rd, cmc_wr, cmc_inhibit;
	reg [0:35] cmb;
	reg [22:35] cma;
	reg cma_rd_rq, cma_wr_rq;
	reg [0:36] core[0:040000];

	assign cyc_rq_p0 = memsel_p0 == sel_p0 & ~fmc_select_p0 & mc_rq_cyc_p0;
	assign cyc_rq_p1 = memsel_p1 == sel_p1 & ~fmc_select_p1 & mc_rq_cyc_p1;
	assign cyc_rq_p2 = memsel_p2 == sel_p2 & ~fmc_select_p2 & mc_rq_cyc_p2;
	assign cyc_rq_p3 = memsel_p3 == sel_p3 & ~fmc_select_p3 & mc_rq_cyc_p3;
	assign cmpc_p0_rq = cyc_rq_p0 & cmc_aw_rq;
	assign cmpc_p1_rq = cyc_rq_p1 & cmc_aw_rq;
	assign cmpc_p2_rq = cyc_rq_p2 & cmc_aw_rq;
	assign cmpc_p3_rq = cyc_rq_p3 & cmc_aw_rq;

	// simulate power on
	initial begin
		power = 0;
		cmc_aw_rq = 0;
		cmc_rd = 0;
		cmc_wr = 0;
		single_step_sw = 0;
		restart_sw = 0;

		cmc_proc_rs = 0;
		cmc_pse_sync = 0;
		cmc_last_proc = 0;
		#500;
		power = 1;
	end

	// there has to be a better way....
	// from proc to mem
	assign mc_wr_rs = cmc_p0_act ? mc_wr_rs_p0 :
	                  cmc_p1_act ? mc_wr_rs_p1 :
	                  cmc_p2_act ? mc_wr_rs_p2 :
	                  cmc_p3_act ? mc_wr_rs_p3 : 0;
	assign mc_rq_cyc = cmc_p0_act ? mc_rq_cyc_p0 :
	                   cmc_p1_act ? mc_rq_cyc_p1 :
	                   cmc_p2_act ? mc_rq_cyc_p2 :
	                   cmc_p3_act ? mc_rq_cyc_p3 : 0;
	assign mc_rd_rq = cmc_p0_act ? mc_rd_rq_p0 :
	                  cmc_p1_act ? mc_rd_rq_p1 :
	                  cmc_p2_act ? mc_rd_rq_p2 :
	                  cmc_p3_act ? mc_rd_rq_p3 : 0;
	assign mc_wr_rq = cmc_p0_act ? mc_wr_rq_p0 :
	                  cmc_p1_act ? mc_wr_rq_p1 :
	                  cmc_p2_act ? mc_wr_rq_p2 :
	                  cmc_p3_act ? mc_wr_rq_p3 : 0;
	assign ma = cmc_p0_act ? ma_p0 :
	            cmc_p1_act ? ma_p1 :
	            cmc_p2_act ? ma_p2 :
	            cmc_p3_act ? ma_p3 : 0;
	assign mb_in = cmc_p0_act ? mb_in_p0 :
	               cmc_p1_act ? mb_in_p1 :
	               cmc_p2_act ? mb_in_p2 :
	               cmc_p3_act ? mb_in_p3 : 0;

	// from mem to proc
	assign cmc_addr_ack_p0 = cmc_addr_ack & cmc_p0_act;
	assign cmc_rd_rs_p0 = cmc_rd_rs & cmc_p0_act;
	assign mb_out_p0 = cmc_p0_act ? mb_out : 0;
	assign cmc_addr_ack_p1 = cmc_addr_ack & cmc_p1_act;
	assign cmc_rd_rs_p1 = cmc_rd_rs & cmc_p1_act;
	assign mb_out_p1 = cmc_p1_act ? mb_out : 0;
	assign cmc_addr_ack_p2 = cmc_addr_ack & cmc_p2_act;
	assign cmc_rd_rs_p2 = cmc_rd_rs & cmc_p2_act;
	assign mb_out_p2 = cmc_p2_act ? mb_out : 0;
	assign cmc_addr_ack_p3 = cmc_addr_ack & cmc_p3_act;
	assign cmc_rd_rs_p3 = cmc_rd_rs & cmc_p3_act;
	assign mb_out_p3 = cmc_p3_act ? mb_out : 0;

	syncpulse synccmc0(.clk(clk), .in(cmc_aw_rq & mc_rq_cyc), .out(cmc_t0));
	syncpulse synccmc2(.clk(clk), .in(mc_wr_rs), .out(mc_wr_rs_S));

	dly200ns  cmcdly1(.clk(clk), .in(cmc_t0), .out(cmc_t1));
	assign cmc_addr_ack = cmc_t1;
	dly1000ns cmcdly2(.clk(clk), .in(cmc_t1), .out(cmc_t2));
	dly1000ns cmcdly3(.clk(clk), .in(cmc_t2), .out(cmc_t4));
	dly200ns  cmcdly4(.clk(clk), .in(cmc_t4), .out(cmc_t5));
	assign cmc_t6 = cmc_t5 & ~cma_wr_rq | cmc_tmp4;
	dly200ns  cmcdly9(.clk(clk), .in(cmc_t6), .out(cmc_t7));
	dly200ns  cmcdly10(.clk(clk), .in(cmc_t7), .out(cmc_t8));
	dly1000ns cmcdly11(.clk(clk), .in(cmc_t8), .out(cmc_t9));
	dly400ns  cmcdly12(.clk(clk), .in(cmc_t9), .out(cmc_t9a));
	assign cmc_t10 = cmc_t9a & ~cmc_stop | cmc_pwr_start;
	dly100ns  cmcdly13(.clk(clk), .in(cmc_t10), .out(cmc_t11));
	dly200ns  cmcdly14(.clk(clk), .in(cmc_t9a), .out(cmc_t12));
	dly800ns  cmcdly5(.clk(clk), .in(cmc_t2), .out(cmc_tmp1));
	assign cmc_strb_sa = cmc_tmp1 & cma_rd_rq;
	dly100ns  cmcdly6(.clk(clk), .in(cmc_strb_sa), .out(cmc_rd_rs));
	dly250ns  cmcdly7(.clk(clk), .in(cmc_strb_sa), .out(cmc_tmp2));
	dly200ns  cmcdly8(.clk(clk), .in(cmc_strb_sa), .out(cmc_tmp3));
	assign cmc_cmb_clr = cmc_t0 | cmc_tmp2 & cma_wr_rq;
	assign cmc_state_clr = cmc_tmp3 & ~cma_wr_rq | cmc_t10 | cmc_proc_rs1;
	syncpulse synccmc1(.clk(clk), .in(| mb_in), .out(mb_pulse));
	syncpulse synccmc3(.clk(clk), .in(cmc_proc_rs & cmc_pse_sync), .out(cmc_tmp4));
	syncpulse synccmc4(.clk(clk), .in(cmc_proc_rs), .out(cmc_proc_rs1));
	syncpulse synccmc5(.clk(clk), .in(power), .out(cmc_pwr_start));

	// generate a longer pulse so processor has time to read
	pa100ns cmcpa0(.clk(clk), .in(cmc_strb_sa), .out(cmc_strb_sa_b));
	assign mb_out = cmc_strb_sa_b ? core[cma] : 0;

	wire [0:35] corescope;
	assign corescope = core[cma];

	always @(posedge clk) begin
		if(cmc_power_start)
			cmc_aw_rq <= 1;
		if(cmc_aw_rq) begin
			if(cmpc_p0_rq)
				cmc_p0_act <= 1;
			else if(cmpc_p1_rq)
				cmc_p1_act <= 1;
			else if(cmpc_p2_rq) begin
				if(~cmpc_p3_rq | cmc_last_proc)
					cmc_p2_act <= 1;
			end else if(cmpc_p3_rq) begin
				if(~cmpc_p2_rq | ~cmc_last_proc)
					cmc_p3_act <= 1;
			end
		end
		if(cmc_state_clr) begin
			cmc_p0_act <= 0;
			cmc_p1_act <= 0;
			cmc_p2_act <= 0;
			cmc_p3_act <= 0;
		end
		if(cmc_t0) begin
			cmc_aw_rq <= 0;
			cmc_proc_rs <= 0;
			cmc_pse_sync <= 0;
			cmc_stop <= 0;
			cma <= 0;
			cma_rd_rq <= 0;
			cma_wr_rq <= 0;
		end
		if(cmc_t1) begin
			cma <= cma | ma_p0[22:35];
			if(mc_rd_rq)
				cma_rd_rq <= 1;
			if(mc_wr_rq)
				cma_wr_rq <= 1;
		end
		if(cmc_t2) begin
			cmc_rd <= 1;
			if(cmc_p2_act)
				cmc_last_proc <= 0;
			if(cmc_p3_act)
				cmc_last_proc <= 1;
		end
		if(cmc_t5) begin
			// this is hack, normally core should be cleared
			// roughly at the time of cmc_strb_sa, which is
			// however does not happen on a write
			core[cma] <= 0;
			cmc_rd <= 0;
			cmc_pse_sync <= 1;
		end
		if(cmc_t7)
			cmc_inhibit <= 1;	// totally useless
		if(cmc_t8)
			cmc_wr <= 1;
		if(cmc_t9 & cmc_wr)
			// again a hack, core is written some time after T8
			// ...and we know cmc_wr is set then anway.
			core[cma] <= core[cma] | cmb;
		if(cmc_t11)
			cmc_aw_rq <= 1;
		if(cmc_t12) begin
			cmc_rd <= 0;
			cmc_wr <= 0;
			cmc_inhibit <= 0;
		end
		if(mc_wr_rs_S)
			cmc_proc_rs <= 1;
		if(cmc_strb_sa) begin
			cmb <= mb_out;
		end
		if(cmc_cmb_clr)
			cmb <= 0;
		if(mb_pulse)
			cmb <= cmb | mb_in;
	end

endmodule
