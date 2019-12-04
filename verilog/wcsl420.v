module wcsl420(
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
	input wire [0:17] ctl2,
	input wire [0:17] ctl3,
	input wire [0:17] ctl4
);
	assign iobus_dr_split = 0;
	assign iobus_rdi_data = 0;
	assign iobus_pi_req = 0;

	wire wcnsls_sel = iobus_ios == 7'b100_010_0;
	wire wcnsls_datai = wcnsls_sel & iobus_iob_fm_datai;

	wire [0:8] ctl1n = {
		ctl1[12], ctl1[13], ctl1[10]|ctl1[11],
		ctl1[15], ctl1[14], 1'b0,
		3'b0
	};
	wire [0:8] ctl2n = {
		ctl2[12], ctl2[13], ctl2[10]|ctl2[11],
		ctl2[15], ctl2[14], 1'b0,
		3'b0
	};
	wire [0:8] ctl3n = {
		ctl3[12], ctl3[13], ctl3[10]|ctl3[11],
		ctl3[15], ctl3[14], 1'b0,
		3'b0
	};
	wire [0:8] ctl4n = {
		ctl4[12], ctl4[13], ctl4[10]|ctl4[11],
		ctl4[15], ctl4[14], 1'b0,
		3'b0
	};

	assign iobus_iob_out =
		wcnsls_datai ? ~{ ctl4n, ctl3n, ctl2n, ctl1n } :
		36'b0;
endmodule
