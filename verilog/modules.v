module syncpulse(
	input clk,
	input in,
	output out
);
	reg x0, x1;
	initial begin
		x0 <= 0;
		x1 <= 0;
	end
	always @(posedge clk) begin
		x0 <= in;
		x1 <= x0;
	end
	assign out = x0 && !x1;
endmodule

module sbr(
	input clk,
	input clr,
	input set,
	input from,
	output ret
);
	reg ff;
	always @(posedge clk) begin
		if(clr | from)
			ff <= 0;
		if(set)
			ff <= 1;
	end
	assign ret = ff & from;
endmodule

module pa100ns(
	input clk,
	input in,
	output out
);
	reg [1:0] r;
	initial
		r <= 0;
	always @(posedge clk) begin
		r = r << 1;
		if(in)
			r = 4'b11;
	end
	assign out = r[1];
endmodule

module dly50ns(
	input clk,
	input in,
	output out
);
	reg r;
	initial
		r <= 0;
	always @(posedge clk)
		r <= {r, in};
	assign out = r;
endmodule

module dly100ns(
	input clk,
	input in,
	output out
);
	reg [1:0] r;
	initial
		r <= 0;
	always @(posedge clk)
		r <= {r, in};
	assign out = r[1];
endmodule

module dly150ns(
	input clk,
	input in,
	output out
);
	reg [2:0] r;
	initial
		r <= 0;
	always @(posedge clk)
		r <= {r, in};
	assign out = r[2];
endmodule

module dly200ns(
	input clk,
	input in,
	output out,
	output level
);
	reg [3:0] r;
	initial
		r <= 0;
	always @(posedge clk)
		r <= {r, in};
	assign out = r[3];
	assign level = (|r[2:0]);
endmodule

module dly250ns(
	input clk,
	input in,
	output out
);
	reg [4:0] r;
	initial
		r <= 0;
	always @(posedge clk)
		r <= {r, in};
	assign out = r[4];
endmodule

module dly400ns(
	input clk,
	input in,
	output out
);
	reg [7:0] r;
	initial
		r <= 0;
	always @(posedge clk)
		r <= {r, in};
	assign out = r[7];
endmodule

module dly800ns(
	input clk,
	input in,
	output out
);
	reg [15:0] r;
	initial
		r <= 0;
	always @(posedge clk)
		r <= {r, in};
	assign out = r[15];
endmodule


module dly1000ns(
	input clk,
	input in,
	output out
);
	reg [19:0] r;
	initial
		r <= 0;
	always @(posedge clk)
		r <= {r, in};
	assign out = r[19];
endmodule

module dly100us(
	input clk,
	input in,
	output out
);
	reg [15:0] r;
	initial
		r <= 0;
	always @(posedge clk) begin
		if(r)
			r = r + 1;
		if(in)
			r = 1;
	end
	assign out = r == 2000;
endmodule
