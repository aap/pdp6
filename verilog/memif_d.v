module memif_d(
	input wire clk,
	input wire reset,

	// Avalon Slave
	input wire [1:0] s_address,
	input wire s_write,
	input wire s_read,
	input wire [31:0] s_writedata,
	output reg [31:0] s_readdata,
	output wire s_waitrequest,

	// 64 bit Avalon Master
	output wire [31:0] m_address,
	output reg m_write,
	output reg m_read,
	output wire [63:0] m_writedata,
	input wire [63:0] m_readdata,
	input wire m_waitrequest
);

	reg [31:0] addr;
	reg [63:0] word;

	assign m_address = addr;
	assign m_writedata = word;

	wire write_edge, read_edge;
	edgedet e0(clk, reset, s_write, write_edge);
	edgedet e1(clk, reset, s_read, read_edge);

	reg waiting;
	wire req = (write_edge|read_edge) & s_address == 2'h2;
	assign s_waitrequest = req | waiting;

	always @(posedge clk or negedge reset) begin
		if(~reset) begin
			m_write <= 0;
			m_read <= 0;
			waiting <= 0;

			addr <= 0;
			word <= 0;
		end else begin
			if(write_edge) begin
				case(s_address)
				2'h0: addr <= s_writedata[31:0];
				2'h1: word[31:0] <= s_writedata[31:0];
				2'h2: word[63:32] <= s_writedata[31:0];
				endcase
			end

			if(req) begin
				waiting <= 1;
				if(s_write)
					m_write <= 1;
				else if(s_read)
					m_read <= 1;
			end

			if(m_write & ~m_waitrequest) begin
				m_write <= 0;
				waiting <= 0;
			end
			if(m_read & ~m_waitrequest) begin
				m_read <= 0;
				waiting <= 0;
				word <= m_readdata;
			end
		end
	end

	always @(*) begin
		case(s_address)
		2'h1: s_readdata <= word[31:0];
		2'h2: s_readdata <= word[63:32];
		default: s_readdata <= 32'b0;
		endcase
	end
endmodule
