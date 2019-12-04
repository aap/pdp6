module memory_256k(
	input wire clk,
	input wire reset,

	// 36 bit Slave
	input wire [17:0] s_address,
	input wire s_write,
	input wire s_read,
	input wire [35:0] s_writedata,
	output wire [35:0] s_readdata,
	output wire s_waitrequest,

	// 64 bit Avalon Master
	output wire [31:0] m_address,
	output wire m_write,
	output wire m_read,
	output wire [63:0] m_writedata,
	input wire [63:0] m_readdata,
	input wire m_waitrequest
);
	parameter [31:0] base_addr = 32'h30000000;

	assign m_address = base_addr | { s_address, 3'b0 };
	assign m_write = s_write;
	assign m_read = s_read;
	assign m_writedata = s_writedata;
	assign s_readdata = m_readdata;
	assign s_waitrequest = m_waitrequest;
endmodule
