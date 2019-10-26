module dly50ns(input clk, input reset, input in, output p);
	reg [2-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 2'b1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 2;
endmodule

module dly70ns(input clk, input reset, input in, output p);
	reg [2-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 2'b1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 3;
endmodule

module dly100ns(input clk, input reset, input in, output p);
	reg [3-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 3'b1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 5;
endmodule

module dly150ns(input clk, input reset, input in, output p);
	reg [3-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 3'b1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 7;
endmodule

module dly200ns(input clk, input reset, input in, output p);
	reg [4-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 4'b1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 10;
endmodule

module dly250ns(input clk, input reset, input in, output p);
	reg [4-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 4'b1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 12;
endmodule

module dly300ns(input clk, input reset, input in, output p);
	reg [4-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 4'b1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 15;
endmodule

module dly400ns(input clk, input reset, input in, output p);
	reg [5-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 5'b1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 20;
endmodule

module dly450ns(input clk, input reset, input in, output p);
	reg [5-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 5'b1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 22;
endmodule

module dly550ns(input clk, input reset, input in, output p);
	reg [5-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 5'b1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 27;
endmodule

module dly750ns(input clk, input reset, input in, output p);
	reg [6-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 6'b1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 37;
endmodule

module dly800ns(input clk, input reset, input in, output p);
	reg [6-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 6'b1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 40;
endmodule


module dly1us(input clk, input reset, input in, output p);
	reg [6-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 6'b1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 50;
endmodule


module ldly1us(input clk, input reset, input in, output p, output reg l);
	reg [6-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset) begin
			r <= 0;
			l <= 0;
		end else begin
			if(r)
				r <= r + 6'b1;
			if(in) begin
				r <= 1;
				l <= 1;
			end
			if(p) begin
				r <= 0;
				l <= 0;
			end
		end
	end
	assign p = r == 50;
endmodule


module ldly1_5us(input clk, input reset, input in, output p, output reg l);
	reg [7-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset) begin
			r <= 0;
			l <= 0;
		end else begin
			if(r)
				r <= r + 7'b1;
			if(in) begin
				r <= 1;
				l <= 1;
			end
			if(p) begin
				r <= 0;
				l <= 0;
			end
		end
	end
	assign p = r == 75;
endmodule


module ldly2us(input clk, input reset, input in, output p, output reg l);
	reg [7-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset) begin
			r <= 0;
			l <= 0;
		end else begin
			if(r)
				r <= r + 7'b1;
			if(in) begin
				r <= 1;
				l <= 1;
			end
			if(p) begin
				r <= 0;
				l <= 0;
			end
		end
	end
	assign p = r == 100;
endmodule


module dly100us(input clk, input reset, input in, output p);
	reg [13-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 13'b1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 5000;
endmodule


module ldly100us(input clk, input reset, input in, output p, output reg l);
	reg [13-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset) begin
			r <= 0;
			l <= 0;
		end else begin
			if(r)
				r <= r + 13'b1;
			if(in) begin
				r <= 1;
				l <= 1;
			end
			if(p) begin
				r <= 0;
				l <= 0;
			end
		end
	end
	assign p = r == 5000;
endmodule


module dly2_1ms(input clk, input reset, input in, output p);
	reg [17-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 17'b1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 105000;
endmodule


module dly2_5ms(input clk, input reset, input in, output p);
	reg [17-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 17'b1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 125000;
endmodule


module dly5ms(input clk, input reset, input in, output p);
	reg [18-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + 18'b1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == 250000;
endmodule


module ldly5ms(input clk, input reset, input in, output p, output reg l);
	reg [18-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset) begin
			r <= 0;
			l <= 0;
		end else begin
			if(r)
				r <= r + 18'b1;
			if(in) begin
				r <= 1;
				l <= 1;
			end
			if(p) begin
				r <= 0;
				l <= 0;
			end
		end
	end
	assign p = r == 250000;
endmodule


module ldly1s(input clk, input reset, input in, output p, output reg l);
	reg [26-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset) begin
			r <= 0;
			l <= 0;
		end else begin
			if(r)
				r <= r + 26'b1;
			if(in) begin
				r <= 1;
				l <= 1;
			end
			if(p) begin
				r <= 0;
				l <= 0;
			end
		end
	end
	assign p = r == 50000000;
endmodule


module ldly5s(input clk, input reset, input in, output p, output reg l);
	reg [28-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset) begin
			r <= 0;
			l <= 0;
		end else begin
			if(r)
				r <= r + 28'b1;
			if(in) begin
				r <= 1;
				l <= 1;
			end
			if(p) begin
				r <= 0;
				l <= 0;
			end
		end
	end
	assign p = r == 250000000;
endmodule

