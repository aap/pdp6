`default_nettype none
`timescale 1ns/1ns

module tb_mem();

	wire clk, reset;
	clock clock(clk, reset);

	reg m0_write = 0;
	reg m0_read = 0;
	reg [35:0] m0_writedata = 0;
	reg [17:0] m0_address = 0;
	wire [35:0] m0_readdata;
	wire m0_waitrequest;
	reg m1_write = 0;
	reg m1_read = 0;
	reg [35:0] m1_writedata = 0;
	reg [17:0] m1_address = 0;
	wire [35:0] m1_readdata;
	wire m1_waitrequest;
	wire s_write, s_read, s_waitrequest;
	wire [17:0] s_address;
	wire [35:0] s_writedata, s_readdata;

	arbiter arb0(.clk(clk), .reset(reset),
		.s0_address(m0_address),
		.s0_write(m0_write),
		.s0_read(m0_read),
		.s0_writedata(m0_writedata),
		.s0_readdata(m0_readdata),
		.s0_waitrequest(m0_waitrequest),
		.s1_address(m1_address),
		.s1_write(m1_write),
		.s1_read(m1_read),
		.s1_writedata(m1_writedata),
		.s1_readdata(m1_readdata),
		.s1_waitrequest(m1_waitrequest),
		.m_address(s_address),
		.m_write(s_write),
		.m_read(s_read),
		.m_writedata(s_writedata),
		.m_readdata(s_readdata),
		.m_waitrequest(s_waitrequest));

	testmem16k memory(.i_clk(clk), .i_reset_n(reset),
		.i_address(s_address), .i_write(s_write), .i_read(s_read),
		.i_writedata(s_writedata),
		.o_readdata(s_readdata),
		.o_waitrequest(s_waitrequest));

	initial begin
		$dumpfile("dump.vcd");
		$dumpvars();

//		memory.mem[4] = 'o123;
//		memory.mem[5] = 'o321;
//		memory.mem[8] = 'o11111;
		memory.ram.ram[4] = 'o123;
		memory.ram.ram[5] = 'o321;
		memory.ram.ram[6] = 'o444444;
		memory.ram.ram[8] = 'o11111;

		#5;

		#200;


		m0_address <= 'o4;
//		m1_address <= 'o10;
		m0_write <= 1;
//		m1_write <= 1;
		m0_writedata <= 'o1234;
//		m1_writedata <= 'o4321;


		@(negedge m0_write);
		@(posedge clk);
		m0_address <= 5;
		m0_read <= 1;

		@(negedge m0_read);
		@(posedge clk);
		m0_address <= 6;
		m0_read <= 1;

		@(negedge m0_read);
		@(posedge clk);
		m0_address <= 0;
		m0_read <= 1;

		@(negedge m0_read);
		@(posedge clk);
		m0_address <= 4;
		m0_read <= 1;
	end

	initial begin
		#40000;
		$finish;
	end

	reg [35:0] data0;
	reg [35:0] data1;
	always @(posedge clk) begin

		if(~m0_waitrequest & m0_write)
			m0_write <= 0;
		if(~m0_waitrequest & m0_read) begin
			m0_read <= 0;
			data0 <= m0_readdata;
		end
		if(~m1_waitrequest & m1_write)
			m1_write <= 0;
		if(~m1_waitrequest & m1_read) begin
			m1_read <= 0;
			data1 <= m1_readdata;
		end
	end

endmodule
