`default_nettype none
`timescale 1ns/1ns

module tb_memif();

	wire clk, reset;
	clock clock(clk, reset);

	reg a_write = 0;
	reg a_read = 0;
	reg [31:0] a_writedata = 0;
	reg [1:0] a_address;
	wire [31:0] a_readdata;
	wire a_waitrequest;

	wire [17:0] b_address;
	wire b_write;
	wire b_read;
	wire [35:0] b_writedata;
	wire [35:0] b_readdata;
	wire b_waitrequest;

	memif memif0(
		.clk(clk),
		.reset(reset),

		.s_address(a_address),
		.s_write(a_write),
		.s_read(a_read),
		.s_writedata(a_writedata),
		.s_readdata(a_readdata),
		.s_waitrequest(a_waitrequest),

		.m_address(b_address),
		.m_write(b_write),
		.m_read(b_read),
		.m_writedata(b_writedata),
		.m_readdata(b_readdata),
		.m_waitrequest(b_waitrequest));

	dlymemory memory(
		.i_clk(clk),
		.i_reset_n(reset),

		.i_address(b_address),
		.i_write(b_write),
		.i_read(b_read),
		.i_writedata(b_writedata),
		.o_readdata(b_readdata),
		.o_waitrequest(b_waitrequest));

	initial begin
		$dumpfile("dump.vcd");
		$dumpvars();

		memory.mem[4] = 123;
		memory.mem[5] = 321;
		memory.mem['o123] = 36'o112233445566;

		#5;

		#200;

		// write address
		@(posedge clk);
		a_address <= 0;
		a_write <= 1;
		a_writedata <= 32'o123;
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
