module clock(clk, reset);
	output reg clk;
	output reg reset;

	initial begin
		clk = 0;
		reset = 0;
		#50 reset = 1;
	end

	always
//		#5 clk = ~clk;
		#10 clk = ~clk;
endmodule

module edgedet(clk, reset, signal, p);
	input wire clk;
	input wire reset;
	input wire signal;
	output wire p;

	reg last;
	always @(posedge clk or negedge reset) begin
		if(~reset)
			last <= 0;
		else
			last <= signal;
	end

	assign p = signal & ~last;
endmodule
