`default_nettype none
`timescale 1ns/1ns

module tb_membusif();

	wire clk, reset;
	clock clock(clk, reset);

	// avalon
	reg a_write = 0;
	reg a_read = 0;
	reg [31:0] a_writedata = 0;
	reg [1:0] a_address;
	wire [31:0] a_readdata;
	wire a_waitrequest;

	// membus
	wire b_rq_cyc;
	wire b_rd_rq;
	wire b_wr_rq;
	wire [21:35] b_ma;
	wire [18:21] b_sel;
	wire b_fmc_select;
	wire [0:35] b_mb_write;
	wire b_wr_rs;

	wire [0:35] b_mb_read = b_mb_read_0 | b_mb_read_1;
	wire b_addr_ack = b_addr_ack_0 | b_addr_ack_1;
	wire b_rd_rs = b_rd_rs_0 | b_rd_rs_1;


	membusif membusif0(
		.clk(clk),
		.reset(reset),

		.s_address(a_address),
		.s_write(a_write),
		.s_read(a_read),
		.s_writedata(a_writedata),
		.s_readdata(a_readdata),
		.s_waitrequest(a_waitrequest),

		.m_rq_cyc(b_rq_cyc),
		.m_rd_rq(b_rd_rq),
		.m_wr_rq(b_wr_rq),
		.m_ma(b_ma),
		.m_sel(b_sel),
		.m_fmc_select(b_fmc_select),
		.m_mb_write(b_mb_write),
		.m_wr_rs(b_wr_rs),
		.m_mb_read(b_mb_read),
		.m_addr_ack(b_addr_ack),
		.m_rd_rs(b_rd_rs));

	wire [0:35] b_mb_read_0;
	wire b_addr_ack_0;
	wire b_rd_rs_0;
	core161c cmem(
		.clk(clk),
		.reset(~reset),
		.power(1'b1),
		.sw_single_step(1'b0),
		.sw_restart(1'b0),

		.membus_rq_cyc_p0(b_rq_cyc),
		.membus_rd_rq_p0(b_rd_rq),
		.membus_wr_rq_p0(b_wr_rq),
		.membus_ma_p0(b_ma),
		.membus_sel_p0(b_sel),
		.membus_fmc_select_p0(b_fmc_select),
		.membus_mb_in_p0(b_mb_write),
		.membus_wr_rs_p0(b_wr_rs),
		.membus_mb_out_p0(b_mb_read_0),
		.membus_addr_ack_p0(b_addr_ack_0),
		.membus_rd_rs_p0(b_rd_rs_0),

		.membus_wr_rs_p1(1'b0),
		.membus_rq_cyc_p1(1'b0),
		.membus_rd_rq_p1(1'b0),
		.membus_wr_rq_p1(1'b0),
		.membus_ma_p1(15'b0),
		.membus_sel_p1(4'b0),
		.membus_fmc_select_p1(1'b0),
		.membus_mb_in_p1(36'b0),

		.membus_wr_rs_p2(1'b0),
		.membus_rq_cyc_p2(1'b0),
		.membus_rd_rq_p2(1'b0),
		.membus_wr_rq_p2(1'b0),
		.membus_ma_p2(15'b0),
		.membus_sel_p2(4'b0),
		.membus_fmc_select_p2(1'b0),
		.membus_mb_in_p2(36'b0),

		.membus_wr_rs_p3(1'b0),
		.membus_rq_cyc_p3(1'b0),
		.membus_rd_rq_p3(1'b0),
		.membus_wr_rq_p3(1'b0),
		.membus_ma_p3(15'b0),
		.membus_sel_p3(4'b0),
		.membus_fmc_select_p3(1'b0),
		.membus_mb_in_p3(36'b0)
	);

	wire [0:35] b_mb_read_1;
	wire b_addr_ack_1;
	wire b_rd_rs_1;
	fast162 fmem(
		.clk(clk),
		.reset(~reset),
		.power(1'b1),
		.sw_single_step(1'b0),
		.sw_restart(1'b0),

		.membus_rq_cyc_p0(b_rq_cyc),
		.membus_rd_rq_p0(b_rd_rq),
		.membus_wr_rq_p0(b_wr_rq),
		.membus_ma_p0(b_ma),
		.membus_sel_p0(b_sel),
		.membus_fmc_select_p0(b_fmc_select),
		.membus_mb_in_p0(b_mb_write),
		.membus_wr_rs_p0(b_wr_rs),
		.membus_mb_out_p0(b_mb_read_1),
		.membus_addr_ack_p0(b_addr_ack_1),
		.membus_rd_rs_p0(b_rd_rs_1),

		.membus_wr_rs_p1(1'b0),
		.membus_rq_cyc_p1(1'b0),
		.membus_rd_rq_p1(1'b0),
		.membus_wr_rq_p1(1'b0),
		.membus_ma_p1(15'b0),
		.membus_sel_p1(4'b0),
		.membus_fmc_select_p1(1'b0),
		.membus_mb_in_p1(36'b0),

		.membus_wr_rs_p2(1'b0),
		.membus_rq_cyc_p2(1'b0),
		.membus_rd_rq_p2(1'b0),
		.membus_wr_rq_p2(1'b0),
		.membus_ma_p2(15'b0),
		.membus_sel_p2(4'b0),
		.membus_fmc_select_p2(1'b0),
		.membus_mb_in_p2(36'b0),

		.membus_wr_rs_p3(1'b0),
		.membus_rq_cyc_p3(1'b0),
		.membus_rd_rq_p3(1'b0),
		.membus_wr_rq_p3(1'b0),
		.membus_ma_p3(15'b0),
		.membus_sel_p3(4'b0),
		.membus_fmc_select_p3(1'b0),
		.membus_mb_in_p3(36'b0)
	);

	initial begin
		$dumpfile("dump.vcd");
		$dumpvars();

		cmem.core[4] = 123;
		cmem.core[5] = 321;
		cmem.core['o123] = 36'o112233445566;
		fmem.ff[3] = 36'o777777666666;

		#5;

		#200;

		// write address
		@(posedge clk);
		a_address <= 0;
		a_write <= 1;
		a_writedata <= 32'o0000123;
		@(negedge a_write);

		@(posedge clk);
		a_address <= 2;
		a_read <= 1;
		@(negedge a_read);

		@(posedge clk);
		a_address <= 1;
		a_read <= 1;
		@(negedge a_read);


		// write address
		@(posedge clk);
		a_address <= 0;
		a_write <= 1;
		a_writedata <= 32'o0000124;
		@(negedge a_write);

		@(posedge clk);
		a_address <= 2;
		a_read <= 1;
		@(negedge a_read);

		@(posedge clk);
		a_address <= 1;
		a_read <= 1;
		@(negedge a_read);

/*
		// write low word
		@(posedge clk);
		a_address <= 1;
		a_write <= 1;
		a_writedata <= 32'o111222;
		@(negedge a_write);

		// write high word
		@(posedge clk);
		a_address <= 2;
		a_write <= 1;
		a_writedata <= 32'o333444;
		@(negedge a_write);
*/
	end

	initial begin
		#40000;
		$finish;
	end

	always @(posedge clk) begin
		if(~a_waitrequest & a_write)
			a_write <= 0;
		if(~a_waitrequest & a_read)
			a_read <= 0;
	end

endmodule
