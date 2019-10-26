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


	// Memory
	wire [17:0] cm_address;
	wire cm_write;
	wire cm_read;
	wire [35:0] cm_writedata;
	wire [35:0] cm_readdata;
	wire cm_waitrequest;

	wire [0:35] b_mb_read_0;
	wire b_addr_ack_0;
	wire b_rd_rs_0;
//	core161c_x cmem(
//	core164 cmem(
	core32k cmem(
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
		.membus_mb_in_p3(36'b0),

		.m_address(cm_address),
		.m_write(cm_write),
		.m_read(cm_read),
		.m_writedata(cm_writedata),
		.m_readdata(cm_readdata),
		.m_waitrequest(cm_waitrequest)
	);

//	memory_16k cmem_x(
	memory_32k cmem_x(
		.i_clk(clk),
		.i_reset_n(reset),
		.i_address(cm_address),
		.i_write(cm_write),
		.i_read(cm_read),
		.i_writedata(cm_writedata),
		.o_readdata(cm_readdata),
		.o_waitrequest(cm_waitrequest));

	reg [17:0] fm_address = 0;
	reg fm_write = 0;
	reg fm_read = 0;
	reg [35:0] fm_writedata = 0;
	wire [35:0] fm_readdata;
	wire fm_waitrequest;

	wire [0:35] b_mb_read_1;
	wire b_addr_ack_1;
	wire b_rd_rs_1;
	fast162_dp fmem(
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
		.membus_mb_in_p3(36'b0),


		.s_address(fm_address),
		.s_write(fm_write),
		.s_read(fm_read),
		.s_writedata(fm_writedata),
		.s_readdata(fm_readdata),
		.s_waitrequest(fm_waitrequest)
	);

	initial begin
		$dumpfile("dump.vcd");
		$dumpvars();

		cmem_x.ram.ram[3] = 36'o101010101010;
		cmem_x.ram.ram[4] = 123;
		cmem_x.ram.ram[5] = 321;
		cmem_x.ram.ram['o123] = 36'o112233445566;
		cmem_x.ram.ram['o124] = 36'o111111111111;
		cmem_x.ram.ram['o40123] = 36'o111111111111;
		fmem.ff[3] = 36'o777777666666;
		fmem.ff[4] = 36'o555555444444;
		fmem.ff[5] = 36'o333333222222;

		#5;

		#200;
		#5000;

/*
		@(posedge clk);
		fm_address <= 3;
		fm_write <= 1;
		fm_writedata <= 36'o1000123;
		@(negedge fm_write);

		@(posedge clk);
		fm_address <= 5;
		fm_write <= 1;
		fm_writedata <= 36'o101202303404;
		@(negedge fm_write);

		@(posedge clk);
		fm_address <= 3;
		fm_read <= 1;
		@(negedge fm_read);
*/

		// write address
		@(posedge clk);
		a_address <= 0;
		a_write <= 1;
		a_writedata <= 32'o0040123;
		@(negedge a_write);

		@(posedge clk);
		a_address <= 2;
		a_read <= 1;
		@(negedge a_read);

		@(posedge clk);
		a_address <= 1;
		a_read <= 1;
		@(negedge a_read);

		#2000;

		@(posedge clk);
		a_address <= 1;
		a_write <= 1;
		a_writedata <= 32'o0000555;
		@(negedge a_write);

		@(posedge clk);
		a_address <= 2;
		a_write <= 1;
		a_writedata <= 32'o101202;
		@(negedge a_write);



/*		// write address
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
*/

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

	reg [0:35] data;
	always @(posedge clk) begin
		if(~a_waitrequest & a_write)
			a_write <= 0;
		if(~a_waitrequest & a_read) begin
			a_read <= 0;
			data <= a_readdata;
		end

		if(~fm_waitrequest & fm_write)
			fm_write <= 0;
		if(~fm_waitrequest & fm_read) begin
			fm_read <= 0;
			data <= fm_readdata;
		end
	end

endmodule
