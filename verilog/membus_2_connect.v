module membus_2_connect(
	// unused
	input wire clk,
	input wire reset,

	// Master
	input wire m_wr_rs,
	input wire m_rq_cyc,
	input wire m_rd_rq,
	input wire m_wr_rq,
	input wire [21:35] m_ma,
	input wire [18:21] m_sel,
	input wire m_fmc_select,
	input wire [0:35] m_mb_write,
	output wire m_addr_ack,
	output wire m_rd_rs,
	output wire [0:35] m_mb_read,

	// Slave 0
	output wire s0_wr_rs,
	output wire s0_rq_cyc,
	output wire s0_rd_rq,
	output wire s0_wr_rq,
	output wire [21:35] s0_ma,
	output wire [18:21] s0_sel,
	output wire s0_fmc_select,
	output wire [0:35] s0_mb_write,
	input wire s0_addr_ack,
	input wire s0_rd_rs,
	input wire [0:35] s0_mb_read,

	// Slave 1
	output wire s1_wr_rs,
	output wire s1_rq_cyc,
	output wire s1_rd_rq,
	output wire s1_wr_rq,
	output wire [21:35] s1_ma,
	output wire [18:21] s1_sel,
	output wire s1_fmc_select,
	output wire [0:35] s1_mb_write,
	input wire s1_addr_ack,
	input wire s1_rd_rs,
	input wire [0:35] s1_mb_read
);
	wire [0:35] mb_out = m_mb_write | s0_mb_read | s1_mb_read;

	assign m_addr_ack = s0_addr_ack | s1_addr_ack;
	assign m_rd_rs = s0_rd_rs | s1_rd_rs;
	assign m_mb_read = mb_out;

	assign s0_wr_rs = m_wr_rs;
	assign s0_rq_cyc = m_rq_cyc;
	assign s0_rd_rq = m_rd_rq;
	assign s0_wr_rq = m_wr_rq;
	assign s0_ma = m_ma;
	assign s0_sel = m_sel;
	assign s0_fmc_select = m_fmc_select;
	assign s0_mb_write = mb_out;

	assign s1_wr_rs = m_wr_rs;
	assign s1_rq_cyc = m_rq_cyc;
	assign s1_rd_rq = m_rd_rq;
	assign s1_wr_rq = m_wr_rq;
	assign s1_ma = m_ma;
	assign s1_sel = m_sel;
	assign s1_fmc_select = m_fmc_select;
	assign s1_mb_write = mb_out;
endmodule
