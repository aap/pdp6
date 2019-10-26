module membusif(
	input wire clk,
	input wire reset,

	// Avalon Slave
	input wire [1:0] s_address,
	input wire s_write,
	input wire s_read,
	input wire [31:0] s_writedata,
	output reg [31:0] s_readdata,
	output wire s_waitrequest,

	// Membus Master
	output reg m_rq_cyc,
	output reg m_rd_rq,
	output reg m_wr_rq,
	output wire [21:35] m_ma,
	output wire [18:21] m_sel,
	output reg m_fmc_select,
	output wire [0:35] m_mb_write,
	output wire m_wr_rs,
	input wire [0:35] m_mb_read,
	input wire m_addr_ack,
	input wire m_rd_rs
);
	reg [0:17] addr;
	reg [0:35] word;

	assign m_ma = addr[3:17];
	assign m_sel = addr[0:3];

	wire write_edge, read_edge;
	edgedet e0(clk, reset, s_write, write_edge);
	edgedet e1(clk, reset, s_read, read_edge);

	reg waiting;
	wire req = (write_edge|read_edge) & s_address == 2'h2;
	assign s_waitrequest = req | waiting | (|waitcyc);

	wire mb_write_pulse;
	wire wr_rs = m_addr_ack & m_wr_rq;
	bd  mc_bd0(clk, ~reset, wr_rs, m_wr_rs);
	bd2 mb_bd1(clk, ~reset, wr_rs, mb_write_pulse);
	assign m_mb_write = mb_write_pulse ? word : 0;

	reg [7:0] waitcyc;

	always @(posedge clk or negedge reset) begin
		if(~reset) begin
			m_rq_cyc <= 0;
			m_rd_rq <= 0;
			m_wr_rq <= 0;
			waiting <= 0;

			addr <= 0;
			m_fmc_select <= 0;
			word <= 0;

			waitcyc <= 0;
		end else begin
			if(write_edge) begin
				case(s_address)
				2'h0: begin
					addr <= s_writedata[17:0];
					m_fmc_select <= s_writedata[18];
				end
				2'h1: word[18:35] <= s_writedata[17:0];
				2'h2: word[0:17] <= s_writedata[17:0];
				endcase
			end

			if(req) begin
				waiting <= 1;
				m_rq_cyc <= 1;
				if(s_write)
					m_wr_rq <= 1;
				else if(s_read) begin
					m_rd_rq <= 1;
					word <= 0;
				end
			end
			// have to wait between cycles
			// because fastmem can get stuck
			if(waitcyc) begin
				if(waitcyc == 'o14)
					waitcyc <= 0;
				else
					waitcyc <= waitcyc + 1;
			end

			if(waiting & m_rd_rq)
				word <= m_mb_read;

			if(m_addr_ack) begin
				m_rq_cyc <= 0;
				waitcyc <= 1;
			end

			if(m_rd_rs) begin	
				m_rd_rq <= 0;
				waiting <= 0;
			end

			if(m_wr_rs) begin
				m_wr_rq <= 0;
				waiting <= 0;
			end
		end
	end

	always @(*) begin
		case(s_address)
		2'h1: s_readdata <= { 14'b0, word[18:35] };
		2'h2: s_readdata <= { 14'b0, word[0:17] };
		default: s_readdata <= 32'b0;
		endcase
	end
endmodule
