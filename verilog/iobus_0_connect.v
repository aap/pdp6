module iobus_0_connect(
	// unused
	input wire clk,
	input wire reset,

	// Master
	input wire m_iob_poweron,
	input wire m_iob_reset,
	input wire m_datao_clear,
	input wire m_datao_set,
	input wire m_cono_clear,
	input wire m_cono_set,
	input wire m_iob_fm_datai,
	input wire m_iob_fm_status,
	input wire [3:9]  m_ios,
	input wire [0:35] m_iob_out,
	output wire [1:7]  m_pi_req,
	output wire [0:35] m_iob_in
);
	assign m_pi_req = 0;
	assign m_iob_in = m_iob_out;
endmodule
