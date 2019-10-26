module arbiter(
	input wire clk,
	input wire reset,

	// Slave 0
	input wire [17:0] s0_address,
	input wire s0_write,
	input wire s0_read,
	input wire [35:0] s0_writedata,
	output reg [35:0] s0_readdata,
	output reg s0_waitrequest,

	// Slave 1
	input wire [17:0] s1_address,
	input wire s1_write,
	input wire s1_read,
	input wire [35:0] s1_writedata,
	output reg [35:0] s1_readdata,
	output reg s1_waitrequest,

	// Master
	output reg [17:0] m_address,
	output reg m_write,
	output reg m_read,
	output reg [35:0] m_writedata,
	input wire [35:0] m_readdata,
	input wire m_waitrequest
);

	wire cyc0 = s0_read | s0_write;
	wire cyc1 = s1_read | s1_write;
	reg sel0, sel1;
	wire connected = sel0 | sel1;

	always @(posedge clk or negedge reset) begin
		if(~reset) begin
			sel0 <= 0;
			sel1 <= 0;
		end else begin
			if(sel0 & ~cyc0 | sel1 & ~cyc1) begin
				// disconnect if cycle is done
				sel0 <= 0;
				sel1 <= 0;
			end else if(~connected) begin
				// connect to master 0 or 1
				if(cyc0)
					sel0 <= 1;
				else if(cyc1)
					sel1 <= 1;
			end
		end
	end

	// Do the connection
	always @(*) begin
		if(sel0) begin
			m_address <= s0_address;
			m_write <= s0_write;
			m_read <= s0_read;
			m_writedata <= s0_writedata;
			s0_readdata <= m_readdata;
			s0_waitrequest <= m_waitrequest;
			s1_readdata <= 0;
			s1_waitrequest <= 1;
		end else if(sel1) begin
			m_address <= s1_address;
			m_write <= s1_write;
			m_read <= s1_read;
			m_writedata <= s1_writedata;
			s1_readdata <= m_readdata;
			s1_waitrequest <= m_waitrequest;
			s0_readdata <= 0;
			s0_waitrequest <= 1;
		end else begin
			m_address <= 0;
			m_write <= 0;
			m_read <= 0;
			m_writedata <= 0;
			s0_readdata <= 0;
			s0_waitrequest <= 1;
			s1_readdata <= 0;
			s1_waitrequest <= 1;
		end
	end
endmodule
