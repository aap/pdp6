// AUTOGEN
module iobus_6_connect(
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
	input wire m_rdi_pulse,
	input wire [3:9] m_ios,
	input wire [0:35] m_iob_write,
	output wire [1:7] m_pi_req,
	output wire [0:35] m_iob_read,
	output wire m_dr_split,
	output wire m_rdi_data,

	// Slave 0
	output wire s0_iob_poweron,
	output wire s0_iob_reset,
	output wire s0_datao_clear,
	output wire s0_datao_set,
	output wire s0_cono_clear,
	output wire s0_cono_set,
	output wire s0_iob_fm_datai,
	output wire s0_iob_fm_status,
	output wire s0_rdi_pulse,
	output wire [3:9] s0_ios,
	output wire [0:35] s0_iob_write,
	input wire [1:7] s0_pi_req,
	input wire [0:35] s0_iob_read,
	input wire s0_dr_split,
	input wire s0_rdi_data,

	// Slave 1
	output wire s1_iob_poweron,
	output wire s1_iob_reset,
	output wire s1_datao_clear,
	output wire s1_datao_set,
	output wire s1_cono_clear,
	output wire s1_cono_set,
	output wire s1_iob_fm_datai,
	output wire s1_iob_fm_status,
	output wire s1_rdi_pulse,
	output wire [3:9] s1_ios,
	output wire [0:35] s1_iob_write,
	input wire [1:7] s1_pi_req,
	input wire [0:35] s1_iob_read,
	input wire s1_dr_split,
	input wire s1_rdi_data,

	// Slave 2
	output wire s2_iob_poweron,
	output wire s2_iob_reset,
	output wire s2_datao_clear,
	output wire s2_datao_set,
	output wire s2_cono_clear,
	output wire s2_cono_set,
	output wire s2_iob_fm_datai,
	output wire s2_iob_fm_status,
	output wire s2_rdi_pulse,
	output wire [3:9] s2_ios,
	output wire [0:35] s2_iob_write,
	input wire [1:7] s2_pi_req,
	input wire [0:35] s2_iob_read,
	input wire s2_dr_split,
	input wire s2_rdi_data,

	// Slave 3
	output wire s3_iob_poweron,
	output wire s3_iob_reset,
	output wire s3_datao_clear,
	output wire s3_datao_set,
	output wire s3_cono_clear,
	output wire s3_cono_set,
	output wire s3_iob_fm_datai,
	output wire s3_iob_fm_status,
	output wire s3_rdi_pulse,
	output wire [3:9] s3_ios,
	output wire [0:35] s3_iob_write,
	input wire [1:7] s3_pi_req,
	input wire [0:35] s3_iob_read,
	input wire s3_dr_split,
	input wire s3_rdi_data,

	// Slave 4
	output wire s4_iob_poweron,
	output wire s4_iob_reset,
	output wire s4_datao_clear,
	output wire s4_datao_set,
	output wire s4_cono_clear,
	output wire s4_cono_set,
	output wire s4_iob_fm_datai,
	output wire s4_iob_fm_status,
	output wire s4_rdi_pulse,
	output wire [3:9] s4_ios,
	output wire [0:35] s4_iob_write,
	input wire [1:7] s4_pi_req,
	input wire [0:35] s4_iob_read,
	input wire s4_dr_split,
	input wire s4_rdi_data,

	// Slave 5
	output wire s5_iob_poweron,
	output wire s5_iob_reset,
	output wire s5_datao_clear,
	output wire s5_datao_set,
	output wire s5_cono_clear,
	output wire s5_cono_set,
	output wire s5_iob_fm_datai,
	output wire s5_iob_fm_status,
	output wire s5_rdi_pulse,
	output wire [3:9] s5_ios,
	output wire [0:35] s5_iob_write,
	input wire [1:7] s5_pi_req,
	input wire [0:35] s5_iob_read,
	input wire s5_dr_split,
	input wire s5_rdi_data
);
	assign m_pi_req = 0 | s0_pi_req | s1_pi_req | s2_pi_req | s3_pi_req | s4_pi_req | s5_pi_req;
	assign m_iob_read = m_iob_write | s0_iob_read | s1_iob_read | s2_iob_read | s3_iob_read | s4_iob_read | s5_iob_read;
	assign m_dr_split = 0 | s0_dr_split | s1_dr_split | s2_dr_split | s3_dr_split | s4_dr_split | s5_dr_split;
	assign m_rdi_data = 0 | s0_rdi_data | s1_rdi_data | s2_rdi_data | s3_rdi_data | s4_rdi_data | s5_rdi_data;


	assign s0_iob_poweron = m_iob_poweron;
	assign s0_iob_reset = m_iob_reset;
	assign s0_datao_clear = m_datao_clear;
	assign s0_datao_set = m_datao_set;
	assign s0_cono_clear = m_cono_clear;
	assign s0_cono_set = m_cono_set;
	assign s0_iob_fm_datai = m_iob_fm_datai;
	assign s0_iob_fm_status = m_iob_fm_status;
	assign s0_rdi_pulse = m_rdi_pulse;
	assign s0_ios = m_ios;
	assign s0_iob_write = m_iob_write;

	assign s1_iob_poweron = m_iob_poweron;
	assign s1_iob_reset = m_iob_reset;
	assign s1_datao_clear = m_datao_clear;
	assign s1_datao_set = m_datao_set;
	assign s1_cono_clear = m_cono_clear;
	assign s1_cono_set = m_cono_set;
	assign s1_iob_fm_datai = m_iob_fm_datai;
	assign s1_iob_fm_status = m_iob_fm_status;
	assign s1_rdi_pulse = m_rdi_pulse;
	assign s1_ios = m_ios;
	assign s1_iob_write = m_iob_write;

	assign s2_iob_poweron = m_iob_poweron;
	assign s2_iob_reset = m_iob_reset;
	assign s2_datao_clear = m_datao_clear;
	assign s2_datao_set = m_datao_set;
	assign s2_cono_clear = m_cono_clear;
	assign s2_cono_set = m_cono_set;
	assign s2_iob_fm_datai = m_iob_fm_datai;
	assign s2_iob_fm_status = m_iob_fm_status;
	assign s2_rdi_pulse = m_rdi_pulse;
	assign s2_ios = m_ios;
	assign s2_iob_write = m_iob_write;

	assign s3_iob_poweron = m_iob_poweron;
	assign s3_iob_reset = m_iob_reset;
	assign s3_datao_clear = m_datao_clear;
	assign s3_datao_set = m_datao_set;
	assign s3_cono_clear = m_cono_clear;
	assign s3_cono_set = m_cono_set;
	assign s3_iob_fm_datai = m_iob_fm_datai;
	assign s3_iob_fm_status = m_iob_fm_status;
	assign s3_rdi_pulse = m_rdi_pulse;
	assign s3_ios = m_ios;
	assign s3_iob_write = m_iob_write;

	assign s4_iob_poweron = m_iob_poweron;
	assign s4_iob_reset = m_iob_reset;
	assign s4_datao_clear = m_datao_clear;
	assign s4_datao_set = m_datao_set;
	assign s4_cono_clear = m_cono_clear;
	assign s4_cono_set = m_cono_set;
	assign s4_iob_fm_datai = m_iob_fm_datai;
	assign s4_iob_fm_status = m_iob_fm_status;
	assign s4_rdi_pulse = m_rdi_pulse;
	assign s4_ios = m_ios;
	assign s4_iob_write = m_iob_write;

	assign s5_iob_poweron = m_iob_poweron;
	assign s5_iob_reset = m_iob_reset;
	assign s5_datao_clear = m_datao_clear;
	assign s5_datao_set = m_datao_set;
	assign s5_cono_clear = m_cono_clear;
	assign s5_cono_set = m_cono_set;
	assign s5_iob_fm_datai = m_iob_fm_datai;
	assign s5_iob_fm_status = m_iob_fm_status;
	assign s5_rdi_pulse = m_rdi_pulse;
	assign s5_ios = m_ios;
	assign s5_iob_write = m_iob_write;
endmodule
