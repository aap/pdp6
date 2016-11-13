module pg(
	input clk,
	input reset,
	input in,
	output p
);
	reg [1:0] x;
	always @(posedge clk or posedge reset)
		if(reset)
			x <= 0;
		else
			x <= { x[0], in };
	assign p = x[0] & !x[1];
endmodule

module pa(input clk, input reset, input in, output p);
	reg p;
	always @(posedge clk or posedge reset)
		if(reset)
			p <= 0;
		else
			p <= in;
endmodule

/*
module pa100ns(input clk, input reset, input in, output p);
	reg [3:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 1;
			if(in)
				r <= 1;
		end
	end
	assign p = r && r <= 10;
endmodule
*/

/* "bus driver", 40ns delayed pulse */
module bd(input clk, input reset, input in, output p);
	reg [2:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 4;
endmodule

/* Same as above but with longer pulse. Used to pulse mb
 * because one more clock cycle is needed to get the data
 * after the pulse has been synchronizes. */
module bd2(input clk, input reset, input in, output p);
	reg [2:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 4 || r == 5;
endmodule

module dly50ns(input clk, input reset, input in, output p);
	reg [2:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 7;
endmodule

module dly100ns(input clk, input reset, input in, output p);
	reg [3:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 12;
endmodule

module dly150ns(input clk, input reset, input in, output p);
	reg [4:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 17;
endmodule

module dly200ns(input clk, input reset, input in, output p);
	reg [4:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 22;
endmodule

module dly250ns(input clk, input reset, input in, output p);
	reg [4:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 27;
endmodule

module dly400ns(input clk, input reset, input in, output p);
	reg [5:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 42;
endmodule

module dly800ns(input clk, input reset, input in, output p);
	reg [6:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 82;
endmodule

module dly1us(input clk, input reset, input in, output p);
	reg [6:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 102;
endmodule

module dly100us(input clk, input reset, input in, output p);
	reg [15:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 10002;
endmodule
