`default_nettype none
`timescale 1ns/1ns

module tb_mem();

	wire clk, reset;
	clock clock(clk, reset);

	reg write = 0;
	reg read = 0;
	reg [35:0] writedata = 0;
	reg [17:0] address;
	wire [35:0] readdata;
	wire waitrequest;
	dlymemory memory(.i_clk(clk), .i_reset_n(reset),
		.i_address(address), .i_write(write), .i_read(read),
		.i_writedata(writedata),
		.o_readdata(readdata),
		.o_waitrequest(waitrequest));

	initial begin
		$dumpfile("dump.vcd");
		$dumpvars();

		memory.mem[4] = 123;
		memory.mem[5] = 321;

		#5;

		address <= 4;

		#200;
		write <= 1;
		writedata <= 36'o44556677;

		@(negedge write);
		@(posedge clk);
		address <= 5;
		read <= 1;
	end

	initial begin
		#40000;
		$finish;
	end

	always @(posedge clk) begin
		if(~waitrequest & write)
			write <= 0;
		if(~waitrequest & read)
			read <= 0;
	end

endmodule
