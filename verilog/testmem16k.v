module testmem16k(
	// input
	i_clk, i_reset_n,
	i_address, i_write, i_read, i_writedata,
	// output
	o_readdata, o_waitrequest
);
	input wire i_clk;
	input wire i_reset_n;
	input wire [17:0] i_address;
	input wire i_write;
	input wire i_read;
	input wire [35:0] i_writedata;
	output wire [35:0] o_readdata;
	output wire o_waitrequest;

	reg [35:0] mem[0:'o40000-1];
	wire addrok = i_address[17:14] == 0;
	wire [13:0] addr = i_address[13:0];
	wire [35:0] memword = addrok ? mem[addr] : 0;

	wire write_edge, read_edge;
	reg [3:0] dly;
	wire ready = dly == 0;

	edgedet e0(i_clk, i_reset_n, i_write, write_edge);
	edgedet e1(i_clk, i_reset_n, i_read, read_edge);

	always @(posedge i_clk or negedge i_reset_n) begin
		if(~i_reset_n) begin
			dly <= 4;
		end else begin
			if(i_write & ready & addrok) begin
				mem[addr] <= i_writedata;
			end
			if(~(i_write | i_read))
				dly <= 4;
			else if(dly)
				dly <= dly - 1;
		end
	end

	assign o_readdata = i_read ? memword : 0;
	assign o_waitrequest = ~ready;
endmodule
