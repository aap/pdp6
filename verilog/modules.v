/*module pireq(
	input wire piok_in,
	input wire pih,
	input wire pir,
	output wire pireq,
	output wire piok_out
);
	wire a = piok_in & ~pih;
	assign pireq = a & pir;
	assign piok_out = a & ~pir;
endmodule*/

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

module dly70ns(input clk, input reset, input in, output p);
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
	assign p = r == 9;
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

module ldly1us(input clk, input reset, input in, output p, output l);
	reg [6:0] r;
	reg l;
	always @(posedge clk or posedge reset) begin
		if(reset) begin
			l <= 0;
			r <= 0;
		end else begin
			if(r)
				r <= r + 1;
			if(in) begin
				l <= 1;
				r <= 1;
			end
			if(r == 101) begin
				l <= 0;
				//r <= 0;
			end
		end
	end
	assign p = r == 102;
endmodule

module ldly1_5us(input clk, input reset, input in, output p, output l);
	reg [7:0] r;
	reg l;
	always @(posedge clk or posedge reset) begin
		if(reset) begin
			l <= 0;
			r <= 0;
		end else begin
			if(r)
				r <= r + 1;
			if(in) begin
				l <= 1;
				r <= 1;
			end
			if(r == 151) begin
				l <= 0;
				//r <= 0;
			end
		end
	end
	assign p = r == 152;
endmodule

module ldly2us(input clk, input reset, input in, output p, output l);
	reg [7:0] r;
	reg l;
	always @(posedge clk or posedge reset) begin
		if(reset) begin
			l <= 0;
			r <= 0;
		end else begin
			if(r)
				r <= r + 1;
			if(in) begin
				l <= 1;
				r <= 1;
			end
			if(r == 201) begin
				l <= 0;
				//r <= 0;
			end
		end
	end
	assign p = r == 202;
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

module ldly100us(input clk, input reset, input in, output p, output l);
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
	assign l = r != 0 && r < 10002;
endmodule

