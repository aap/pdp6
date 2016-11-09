module fastmem(
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
	wire [0:35] mb_out;
	wire [0:35] mb_in;

	reg power;
	reg single_step_sw;
	reg restart_sw;
	reg [0:3] memsel_p0;
	reg [0:3] memsel_p1;
	reg [0:3] memsel_p2;
	reg [0:3] memsel_p3;
	reg fmc_p0_sel;
	reg fmc_p1_sel;
	reg fmc_p2_sel;
	reg fmc_p3_sel;

	reg fmc_act, fmc_rd0, fmc_rs, fmc_stop, fmc_wr;
	reg [0:35] ff[0:16];
	wire [32:35] fma;
	wire fma_rd_rq, fma_wr_rq;
	wire [0:35] fm_out;

	// simulate power on
	initial begin
		power = 0;
		single_step_sw = 0;
		restart_sw = 0;
		#500 power = 1;
	end

	assign fma = fmc_p0_sel ? ma_p0[32:35] :
	             fmc_p1_sel ? ma_p1[32:35] :
	             fmc_p2_sel ? ma_p2[32:35] :
	             fmc_p3_sel ? ma_p3[32:35] : 0;
	assign mb_in = fmc_p0_sel ? mb_in_p0 :
	               fmc_p1_sel ? mb_in_p1 :
	               fmc_p2_sel ? mb_in_p2 :
	               fmc_p3_sel ? mb_in_p3 : 0;
	assign fmc_wr_rs = fmc_p0_sel ? mc_wr_rs_p0 :
	                   fmc_p1_sel ? mc_wr_rs_p1 :
	                   fmc_p2_sel ? mc_wr_rs_p2 :
	                   fmc_p3_sel ? mc_wr_rs_p3 : 0;
	assign fma_rd_rq = fmc_p0_sel ? mc_rd_rq_p0 :
			   fmc_p1_sel ? mc_rd_rq_p1 :
	                   fmc_p2_sel ? mc_rd_rq_p2 :
	                   fmc_p3_sel ? mc_rd_rq_p3 : 0;
	assign fma_wr_rq = fmc_p0_sel ? mc_wr_rq_p0 :
			   fmc_p1_sel ? mc_wr_rq_p1 :
	                   fmc_p2_sel ? mc_wr_rq_p2 :
	                   fmc_p3_sel ? mc_wr_rq_p3 : 0;
	assign cmc_addr_ack_p0 = fmc_addr_ack & fmc_p0_sel;
	assign cmc_rd_rs_p0 = fmc_rd_rs & fmc_p0_sel;
	assign mb_out_p0 = fmc_p0_sel ? mb_out : 0;
	assign cmc_addr_ack_p1 = fmc_addr_ack & fmc_p1_sel;
	assign cmc_rd_rs_p1 = fmc_rd_rs & fmc_p1_sel;
	assign mb_out_p1 = fmc_p1_sel ? mb_out : 0;
	assign cmc_addr_ack_p2 = fmc_addr_ack & fmc_p2_sel;
	assign cmc_rd_rs_p2 = fmc_rd_rs & fmc_p2_sel;
	assign mb_out_p2 = fmc_p2_sel ? mb_out : 0;
	assign cmc_addr_ack_p3 = fmc_addr_ack & fmc_p3_sel;
	assign cmc_rd_rs_p3 = fmc_rd_rs & fmc_p3_sel;
	assign mb_out_p3 = fmc_p3_sel ? mb_out : 0;

	assign fmc_addr_ack = fmc_t0;
	assign fmc_rd_rs = fmc_t1;
	assign mb_out = fmc_rd_strb ? fm_out : 0;
	assign fm_out = fma > 0 | fmc_rd0 ? ff[fma] : 0;

	assign fmc_p0_sel1 = fmc_p0_sel & ~fmc_stop;
	assign fmc_p1_sel1 = fmc_p1_sel & ~fmc_stop;
	assign fmc_p2_sel1 = fmc_p2_sel & ~fmc_stop;
	assign fmc_p3_sel1 = fmc_p3_sel & ~fmc_stop;
	assign fmc_p0_wr_sel = fmc_p0_sel & fmc_act & ~fma_rd_rq;
	assign fmc_p1_wr_sel = fmc_p1_sel & fmc_act & ~fma_rd_rq;
	assign fmc_p2_wr_sel = fmc_p2_sel & fmc_act & ~fma_rd_rq;
	assign fmc_p3_wr_sel = fmc_p3_sel & fmc_act & ~fma_rd_rq;
	assign fmpc_p0_rq = fmc_p0_sel1 & memsel_p0 == sel_p0 &
		fmc_select_p0 & mc_rq_cyc_p0;
	assign fmpc_p1_rq = fmc_p1_sel1 & memsel_p1 == sel_p1 &
		fmc_select_p1 & mc_rq_cyc_p1;
	assign fmpc_p2_rq = fmc_p2_sel1 & memsel_p2 == sel_p2 &
		fmc_select_p2 & mc_rq_cyc_p2;
	assign fmpc_p3_rq = fmc_p3_sel1 & memsel_p3 == sel_p3 &
		fmc_select_p3 & mc_rq_cyc_p3;

	// Pulses
	// the delays here aren't accurate, but gate delays accumulate
	syncpulse syncfmc0(.clk(clk), .in(power), .out(fmc_pwr_start));
	syncpulse syncfmc1(.clk(clk), .in(fma_rd_rq), .out(fma_rd_rqD));
	dly50ns fmcdly0(.clk(clk), .in(fma_rd_rqD), .out(fmc_rd0_set));
	syncpulse syncfmc2(.clk(clk), .in(fmc_act), .out(fmc_t0));
	dly50ns fmcdly1(.clk(clk), .in(fmc_t0), .out(fmc_t0D));
	assign fmc_t1 = fmc_t0D & fma_rd_rq;
	// generate a longer pulse so processor has time to read
	pa100ns fmcpa0(.clk(clk), .in(fmc_t1), .out(fmc_rd_strb));
	dly100ns fmcdly2(.clk(clk), .in(fmc_t1), .out(fmc_t1D));
	assign fmc_t3 = fmc_t0D & ~fma_rd_rq & fma_wr_rq |
		fmc_t1D & fma_wr_rq;
	assign fmc_restart = fmc_pwr_start;
	dly200ns fmcdly3(.clk(clk), .in(fmc_restart), .out(fmc_start), .level(fmc_clr));
	assign fmc_t4 = fmc_wr_rsD | fmc_t1D & ~fma_wr_rq;
	dly50ns fmcdly4(.clk(clk), .in(fmc_t4), .out(fmc_t4D));
	assign fmc_t5 = fmc_start | fmc_t4D & ~fmc_stop;

	syncpulse syncfmc3(.clk(clk), .in(| mb_in), .out(fmb_in));
	syncpulse syncfmc4(.clk(clk), .in(fmc_wr_rs), .out(fmc_wr_rsS));
	dly50ns   fmcdly5(.clk(clk), .in(fmc_wr_rsS), .out(fmc_wr_rsD));

	wire [0:35] wordn;
	assign wordn = ff[fma];

	always @(posedge clk) begin
		if(fmc_clr) begin
			fmc_act <= 0;
			fmc_stop <= 1;
		end
		if(fmpc_p0_rq | fmpc_p1_rq | fmpc_p2_rq | fmpc_p3_rq)
			fmc_act <= 1;
		if(fmc_rd0_set)
			fmc_rd0 <= 1;
		if(~fma_rd_rq)
			fmc_rd0 <= 0;
		if(fmc_t0) begin
			fmc_rs <= 0;
			fmc_stop <= single_step_sw;
		end
		if(fmc_t3) begin
			fmc_wr <= 1;
			ff[fma] <= 0;
		end
		if(fmc_t4) begin
			fmc_rd0 <= 0;
			fmc_act <= 0;
		end
		if(fmc_t5) begin
			fmc_stop <= 0;
			fmc_wr <= 0;
		end
		if(fmb_in & fmc_wr)
			ff[fma] <= ff[fma] | mb_in;
		if(fmc_wr_rsS)
			fmc_rs <= 1;
	end

endmodule
