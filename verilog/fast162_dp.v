module fast162_dp(
	input wire clk,
	input wire reset,
	input wire power,
	input wire sw_single_step,
	input wire sw_restart,

	// 4 Membus slaves
	input  wire membus_wr_rs_p0,
	input  wire membus_rq_cyc_p0,
	input  wire membus_rd_rq_p0,
	input  wire membus_wr_rq_p0,
	input  wire [21:35] membus_ma_p0,
	input  wire [18:21] membus_sel_p0,
	input  wire membus_fmc_select_p0,
	input  wire [0:35] membus_mb_in_p0,
	output wire membus_addr_ack_p0,
	output wire membus_rd_rs_p0,
	output wire [0:35] membus_mb_out_p0,

	input  wire membus_wr_rs_p1,
	input  wire membus_rq_cyc_p1,
	input  wire membus_rd_rq_p1,
	input  wire membus_wr_rq_p1,
	input  wire [21:35] membus_ma_p1,
	input  wire [18:21] membus_sel_p1,
	input  wire membus_fmc_select_p1,
	input  wire [0:35] membus_mb_in_p1,
	output wire membus_addr_ack_p1,
	output wire membus_rd_rs_p1,
	output wire [0:35] membus_mb_out_p1,

	input  wire membus_wr_rs_p2,
	input  wire membus_rq_cyc_p2,
	input  wire membus_rd_rq_p2,
	input  wire membus_wr_rq_p2,
	input  wire [21:35] membus_ma_p2,
	input  wire [18:21] membus_sel_p2,
	input  wire membus_fmc_select_p2,
	input  wire [0:35] membus_mb_in_p2,
	output wire membus_addr_ack_p2,
	output wire membus_rd_rs_p2,
	output wire [0:35] membus_mb_out_p2,

	input  wire membus_wr_rs_p3,
	input  wire membus_rq_cyc_p3,
	input  wire membus_rd_rq_p3,
	input  wire membus_wr_rq_p3,
	input  wire [21:35] membus_ma_p3,
	input  wire [18:21] membus_sel_p3,
	input  wire membus_fmc_select_p3,
	input  wire [0:35] membus_mb_in_p3,
	output wire membus_addr_ack_p3,
	output wire membus_rd_rs_p3,
	output wire [0:35] membus_mb_out_p3,

	// 36 bit Avalon Slave
	input wire [17:0] s_address,
	input wire s_write,
	input wire s_read,
	input wire [35:0] s_writedata,
	output wire [35:0] s_readdata,
	output wire s_waitrequest
);

	/* Jumpers */
	parameter memsel_p0 = 4'b0;
	parameter memsel_p1 = 4'b0;
	parameter memsel_p2 = 4'b0;
	parameter memsel_p3 = 4'b0;
	parameter fmc_p0_sel = 1'b1;
	parameter fmc_p1_sel = 1'b0;
	parameter fmc_p2_sel = 1'b0;
	parameter fmc_p3_sel = 1'b0;


	reg fmc_act;
	reg fmc_rd0;
	reg fmc_rs;		// not used, what is this?
	reg fmc_stop;
	reg fmc_wr;
	wire [0:35] fm_out = (fma != 0 | fmc_rd0) ? ff[fma] : 0;
	reg [0:35] ff[0:15];

	wire wr_rs = fmc_p0_sel ? membus_wr_rs_p0 :
	             fmc_p1_sel ? membus_wr_rs_p1 :
	             fmc_p2_sel ? membus_wr_rs_p2 :
	             fmc_p3_sel ? membus_wr_rs_p3 : 1'b0;
	wire fma_rd_rq = fmc_p0_sel ? membus_rd_rq_p0 :
	                 fmc_p1_sel ? membus_rd_rq_p1 :
	                 fmc_p2_sel ? membus_rd_rq_p2 :
	                 fmc_p3_sel ? membus_rd_rq_p3 : 1'b0;
	wire fma_wr_rq = fmc_p0_sel ? membus_wr_rq_p0 :
	                 fmc_p1_sel ? membus_wr_rq_p1 :
	                 fmc_p2_sel ? membus_wr_rq_p2 :
	                 fmc_p3_sel ? membus_wr_rq_p3 : 1'b0;
	wire [21:35] fma = fmc_p0_sel ? membus_ma_p0[32:35] :
	                   fmc_p1_sel ? membus_ma_p1[32:35] :
	                   fmc_p2_sel ? membus_ma_p2[32:35] :
	                   fmc_p3_sel ? membus_ma_p3[32:35] : 1'b0;
	wire [0:35] mb_in = fmc_p0_wr_sel ? membus_mb_in_p0 :
	                    fmc_p1_wr_sel ? membus_mb_in_p1 :
	                    fmc_p2_wr_sel ? membus_mb_in_p2 :
	                    fmc_p3_wr_sel ? membus_mb_in_p3 : 1'b0;
	assign membus_addr_ack_p0 = fmc_addr_ack & fmc_p0_sel;
	assign membus_rd_rs_p0 = fmc_rd_rs & fmc_p0_sel;
	assign membus_mb_out_p0 = fmc_p0_sel ? mb_out : 1'b0;
	assign membus_addr_ack_p1 = fmc_addr_ack & fmc_p1_sel;
	assign membus_rd_rs_p1 = fmc_rd_rs & fmc_p1_sel;
	assign membus_mb_out_p1 = fmc_p1_sel ? mb_out : 1'b0;
	assign membus_addr_ack_p2 = fmc_addr_ack & fmc_p2_sel;
	assign membus_rd_rs_p2 = fmc_rd_rs & fmc_p2_sel;
	assign membus_mb_out_p2 = fmc_p2_sel ? mb_out : 1'b0;
	assign membus_addr_ack_p3 = fmc_addr_ack & fmc_p3_sel;
	assign membus_rd_rs_p3 = fmc_rd_rs & fmc_p3_sel;
	assign membus_mb_out_p3 = fmc_p3_sel ? mb_out : 1'b0;

	wire fmc_addr_ack;
	wire fmc_rd_rs;
	wire [0:35] mb_out = fmc_rd_strb ? fm_out : 36'b0;

	wire fmc_p0_sel1 = fmc_p0_sel & ~fmc_stop;
	wire fmc_p1_sel1 = fmc_p1_sel & ~fmc_stop;
	wire fmc_p2_sel1 = fmc_p2_sel & ~fmc_stop;
	wire fmc_p3_sel1 = fmc_p3_sel & ~fmc_stop;
	wire fmc_p0_wr_sel = fmc_p0_sel & fmc_act & ~fma_rd_rq;
	wire fmc_p1_wr_sel = fmc_p1_sel & fmc_act & ~fma_rd_rq;
	wire fmc_p2_wr_sel = fmc_p2_sel & fmc_act & ~fma_rd_rq;
	wire fmc_p3_wr_sel = fmc_p3_sel & fmc_act & ~fma_rd_rq;
	wire fmpc_p0_rq = fmc_p0_sel1 & memsel_p0 == membus_sel_p0 &
		membus_fmc_select_p0 & membus_rq_cyc_p0;
	wire fmpc_p1_rq = fmc_p1_sel1 & memsel_p1 == membus_sel_p1 &
		membus_fmc_select_p1 & membus_rq_cyc_p1;
	wire fmpc_p2_rq = fmc_p2_sel1 & memsel_p2 == membus_sel_p2 &
		membus_fmc_select_p2 & membus_rq_cyc_p2;
	wire fmpc_p3_rq = fmc_p3_sel1 & memsel_p3 == membus_sel_p3 &
		membus_fmc_select_p3 & membus_rq_cyc_p3;

	wire fmc_pwr_on;
	wire fmc_restart;
	wire fmc_start;
	wire fmc_rd_strb;
	wire fmct0;
	wire fmct1;
	wire fmct3;
	wire fmct4;
	wire fmct5;

	wire fm_clr;
	wire fmc_wr_set;
	wire fmc_wr_rs;
	wire fma_rd_rq_P, fma_rd_rq_D, fmc_rd0_set;
	wire fmct1_D, fmct3_D;
	wire mb_pulse_in;

	pg fmc_pg0(.clk(clk), .reset(reset), .in(power), .p(fmc_pwr_on));
	pg fmc_pg1(.clk(clk), .reset(reset), .in(sw_restart & fmc_stop),
		.p(fmc_restart));
	pg fmc_pg2(.clk(clk), .reset(reset), .in(fmc_act), .p(fmct0));
	pg fmc_pg3(.clk(clk), .reset(reset), .in(fma_rd_rq), .p(fma_rd_rq_P));
	pg cmc_pg5(.clk(clk), .reset(reset), .in(wr_rs), .p(fmc_wr_rs));

	pa fmc_pa0(.clk(clk), .reset(reset),
		.in(fmc_start |
		    fmct4 & ~fmc_stop),
		.p(fmct5));
	pa fmc_pa1(.clk(clk), .reset(reset),
		.in(fmct0 & fma_rd_rq),
		.p(fmct1));
	pa fmc_pa2(.clk(clk), .reset(reset),
		.in(fma_rd_rq_D),
		.p(fmc_rd0_set));
	pa fmc_pa3(.clk(clk), .reset(reset),
		.in(fmct3),
		.p(fm_clr));
	pa fmc_pa4(.clk(clk), .reset(reset),
		.in(fmct3_D),
		.p(fmc_wr_set));
	pg fmc_pg5(.clk(clk), .reset(reset),
		.in(fmct0 & ~fma_rd_rq & fma_wr_rq |
		    fmct1_D & fma_wr_rq),
		.p(fmct3));
	pa fmc_pa6(.clk(clk), .reset(reset),
		.in(fmct1_D & ~fma_wr_rq |
		    fmc_wr_rs),
		.p(fmct4));

	dly200ns fmc_dly0(.clk(clk), .reset(reset),
		.in(fmc_restart | fmc_pwr_on),
		.p(fmc_start));
	dly50ns  fmc_dly1(.clk(clk), .reset(reset),
		.in(fma_rd_rq_P),
		.p(fma_rd_rq_D));
	dly100ns fmc_dly3(.clk(clk), .reset(reset),
		.in(fmct1),
		.p(fmct1_D));
	dly50ns  fmc_dly4(.clk(clk), .reset(reset),
		.in(fmct3),
		.p(fmct3_D));

	bd fmc_bd0(.clk(clk), .reset(reset), .in(fmct0), .p(fmc_addr_ack));
	bd fmc_bd1(.clk(clk), .reset(reset), .in(fmct1), .p(fmc_rd_rs));
	bd fmc_bd2(.clk(clk), .reset(reset), .in(fmct1), .p(fmc_rd_strb));

`ifdef simulation
	always @(posedge reset) begin
		fmc_act <= 0;
	end
`endif

	always @(posedge clk) begin
		if(fmc_restart | fmc_pwr_on) begin
			fmc_act <= 0;
			fmc_stop <= 1;
		end
		if(fmpc_p0_rq | fmpc_p1_rq | fmpc_p2_rq | fmpc_p3_rq)
			fmc_act <= 1;
		if(fmc_wr_rs)
			fmc_rs <= 1;
		if(~fma_rd_rq)
			fmc_rd0 <= 0;
		if(fmc_rd0_set)
			fmc_rd0 <= 1;
		if(fmc_wr_set)
			fmc_wr <= 1;
		if(fm_clr)
			ff[fma] <= 0;
		if(fmc_wr)
			ff[fma] <= ff[fma] | mb_in;
		if(fmct0) begin
			fmc_rs <= 0;
			fmc_stop <= sw_single_step;
		end
		if(fmct4) begin
			fmc_act <= 0;
			fmc_rd0 <= 0;
		end
		if(fmct5) begin
			fmc_stop <= 0;
			fmc_wr <= 0;
		end

		if(s_write)
			ff[s_address[3:0]] <= s_writedata;
	end


	assign s_readdata = ff[s_address[3:0]];
	assign s_waitrequest = 0;

endmodule
