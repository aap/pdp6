module fe_req(
	// unused
	input wire clk,
	input wire reset,

	// requests
	input wire [31:0] req,

	// Avalon slave
	output wire [31:0] readdata
);
	assign readdata = req;
endmodule
