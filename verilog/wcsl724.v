module wcsl724(
	input wire clk,
	input wire reset,

	/* IO bus */
	input  wire iobus_iob_poweron,
	input  wire iobus_iob_reset,
	input  wire iobus_datao_clear,
	input  wire iobus_datao_set,
	input  wire iobus_cono_clear,
	input  wire iobus_cono_set,
	input  wire iobus_iob_fm_datai,
	input  wire iobus_iob_fm_status,
	input  wire iobus_rdi_pulse,	// unused on 6
	input  wire [3:9]  iobus_ios,
	input  wire [0:35] iobus_iob_in,
	output wire [1:7]  iobus_pi_req,
	output wire [0:35] iobus_iob_out,
	output wire iobus_dr_split,
	output wire iobus_rdi_data,	// unused on 6

	/* Externals */
	input wire [0:17] ctl1,
	input wire [0:17] ctl2
);
	assign iobus_dr_split = 0;
	assign iobus_rdi_data = 0;
	assign iobus_pi_req = 0;

	wire wcnsls_sel = iobus_ios == 7'b111_010_1;
	wire wcnsls_datai = wcnsls_sel & iobus_iob_fm_datai;

	assign iobus_iob_out =
		wcnsls_datai ? { ctl2, ctl1 } :
		36'b0;
endmodule
