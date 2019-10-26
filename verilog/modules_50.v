// input 50mhz, output ~60hz
module clk60hz(
	input wire clk,
	output wire outclk
);
	reg [19:0] cnt = 0;
	assign outclk = cnt == 833333;
	always @(posedge clk)
		if(outclk)
			cnt <= 0;
		else
			cnt <= cnt + 20'b1;
endmodule

// input 50mhz, output 63.3hz
module clk63_3hz(
	input wire clk,
	output wire outclk
);
	reg [19:0] cnt = 0;
	assign outclk = cnt == 789900;
	always @(posedge clk)
		if(outclk)
			cnt <= 0;
		else
			cnt <= cnt + 20'b1;
endmodule

// input 50mhz, output 25khz
module clk25khz(
	input wire clk,
	input wire en,
	output wire outclk
);
	reg [10:0] cnt = 0;
	assign outclk = en & (cnt == 2000);
	always @(posedge clk)
		if(outclk)
			cnt <= 0;
		else
			cnt <= cnt + 11'b1;
endmodule

// input 50mhz, output 50khz
module clk50khz(
	input wire clk,
	output wire outclk
);
	reg [9:0] cnt = 0;
	assign outclk = cnt == 1000;
	always @(posedge clk)
		if(outclk)
			cnt <= 0;
		else
			cnt <= cnt + 10'b1;
endmodule


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

/*
// This breaks things because it doesn't detect power on
module pg(input wire clk, input wire reset, input wire in, output wire p);
	reg [1:0] x;
	reg [1:0] init = 0;
	always @(posedge clk or posedge reset)
		if(reset)
			init <= 0;
		else begin
			x <= { x[0], in };
			init <= { init[0], 1'b1 };
		end
	assign p = (&init) & x[0] & !x[1];
endmodule
*/

module pa(input wire clk, input wire reset, input wire in, output wire p);
	reg [1:0] x;
	reg [1:0] init = 0;
	always @(posedge clk or posedge reset)
		if(reset)
			init <= 0;
		else begin
			x <= { x[0], in };
			init <= { init[0], 1'b1 };
		end
	assign p = (&init) & x[0] & !x[1];
endmodule


// TODO: check the purpose of these

/* "bus driver", 40ns delayed pulse */
module bd(input clk, input reset, input in, output p);
	reg [2:0] r;
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
	assign p = r == 4;
endmodule

/* Same as above but with longer pulse. Used to pulse mb
 * because one more clock cycle is needed to get the data
 * after the pulse has been synchronized. */
// TODO? get rid of this and just latch
module bd2(input clk, input reset, input in, output p);
	reg [2:0] r;
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
	assign p = r == 4 || r == 5 || r == 6 || r == 7;
endmodule
