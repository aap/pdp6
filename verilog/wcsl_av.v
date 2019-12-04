module wcsl_av(
	input wire clk,
	input wire reset,

	/* Avalon slave */
	input wire s_write,
	input wire [1:0] s_address,
	input wire [31:0] s_writedata,

	/* Externals */
	output reg [0:17] ctl1,
	output reg [0:17] ctl2,
	output reg [0:17] ctl3,
	output reg [0:17] ctl4
);

	always @(posedge clk) begin
		if(reset) begin
			ctl1 <= 0;
			ctl2 <= 0;
			ctl3 <= 0;
			ctl4 <= 0;
		end else begin
			if(s_write) case(s_address)
			2'b00: ctl1 <= s_writedata;
			2'b01: ctl2 <= s_writedata;
			2'b10: ctl3 <= s_writedata;
			2'b11: ctl4 <= s_writedata;
			endcase
		end
	end
endmodule
