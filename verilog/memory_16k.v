module memory_16k(
	input wire i_clk,
	input wire i_reset_n,
	input wire [17:0] i_address,
	input wire i_write,
	input wire i_read,
	input wire [35:0] i_writedata,
	output wire [35:0] o_readdata,
	output reg o_waitrequest
);
	
	wire addrok = i_address[17:14] == 0;
	wire [13:0] addr = i_address[13:0];
	reg we;

	onchip_ram #(
		.ADDR_WIDTH(14)
	) ram (
		.clk(i_clk),
		.data(i_writedata),
		.addr(addr),
		.we(we),
		.q(o_readdata));

	/* have to wait one clock for ram address */
	always @(posedge i_clk) begin
		if(~i_reset_n) begin
			we <= 0;
			o_waitrequest <= 0;
		end else begin
			if(i_read | i_write)
				o_waitrequest <= 0;
			else
				o_waitrequest <= 1;

			if(we)
				we <= 0;
			else
				we <= i_write & addrok;
		end
	end
endmodule
