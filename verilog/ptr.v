module ptr(
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

	/* Console panel */
	input wire key_start,
	input wire key_stop,
	input wire key_tape_feed,
	output wire [35:0] ptr_ind,
	output wire [6:0] status_ind,	// also includes motor on

	/* Avalon slave */
	input wire s_write,
	input wire [31:0] s_writedata,

	output wire fe_data_rq
);
	assign iobus_dr_split = 0;
	assign iobus_rdi_data = 0;

	assign ptr_ind = ptr;
	assign status_ind = { motor_on, ptr_b, ptr_busy, ptr_flag, ptr_pia };


	wire ptr_sel = iobus_ios == 7'b001_000_1;

	wire [8:1] stb_hole = ptr_b ? { 2'b0, hole[6:1] } :  hole[8:1];

	wire ptr_data_clr;
	wire ptr_data_set;
	wire ptr_ic_clr;
	wire ptr_ic_set;
	wire iob_reset;
	wire ptr_datai = ptr_sel & iobus_iob_fm_datai;
	wire ptr_status = ptr_sel & iobus_iob_fm_status;
	wire ptr_start_clr, ptr_stop_clr;
	wire ptr_busy_set;
	pa ptr_pa0(clk, reset, ptr_sel & iobus_datao_clear, ptr_data_clr);
	pa ptr_pa1(clk, reset, ptr_sel & iobus_datao_set, ptr_data_set);
	pa ptr_pa2(clk, reset, ptr_sel & iobus_cono_clear | iob_reset, ptr_ic_clr);
	pa ptr_pa3(clk, reset, ptr_sel & iobus_cono_set, ptr_ic_set);
	pa ptr_pa4(clk, reset, iobus_iob_reset, iob_reset);
	pg ptr_pg0(clk, reset, motor_on, ptr_start_clr);
	pg ptr_pg1(clk, reset, ~motor_on, ptr_stop_clr);
	pa ptr_pa5(clk, reset, ~ptr_datai, ptr_busy_set);	// CDG actually

	wire ptr_clr;
	pa ptr_pa6(clk, reset, ptr_busy, ptr_clr);

	reg [36:31] ptr_sr;	// actually 36,30,24,18,12,6
	reg [35:0] ptr;
	reg motor_on = 0;
	wire ptr_lead;
	wire ptr_mid;	// mid hole, this is where the strobe happens.
			// normally 400μs after leading edge of feed hole
	wire ptr_strobe = ptr_mid & (~ptr_b | hole[8]);
	wire ptr_trail;

	assign iobus_iob_out =
		ptr_datai ? ptr :
		ptr_status ? { 27'b0, motor_on, 2'b0, ptr_b, ptr_busy, ptr_flag, ptr_pia } :
		36'b0;

	wire [1:7] ptr_req = { ptr_flag, 7'b0 } >> ptr_pia;
	assign iobus_pi_req = ptr_req;

	reg [33:35] ptr_pia;
	reg ptr_busy;
	reg ptr_flag;
	reg ptr_b;

`ifdef simulation
	initial begin
		ptr_busy <= 0;
		ptr_flag <= 0;
		ptr_b <= 0;
	end
`endif

	always @(posedge clk) begin
		if(ptr_ic_clr) begin
			ptr_pia <= 0;
			ptr_busy <= 0;
			ptr_flag <= 0;
			ptr_b <= 0;
		end
		if(ptr_ic_set) begin
			ptr_pia <= iobus_iob_in[33:35];
			if(iobus_iob_in[32])
				ptr_flag <= 1;
			if(iobus_iob_in[31])
				ptr_busy <= 1;
			if(iobus_iob_in[30])
				ptr_b <= 1;
		end

		if(ptr_busy_set)
			ptr_busy <= 1;
		if(ptr_start_clr)
			ptr_busy <= 0;

		if(ptr_start_clr | ptr_stop_clr)
			ptr_flag <= 1;
		if(ptr_datai)
			ptr_flag <= 0;

		if(ptr_trail & ptr_busy & (~ptr_b | ptr_sr[36])) begin
			ptr_busy <= 0;
			ptr_flag <= 1;
		end

		if(ptr_clr) begin
			ptr <= 0;
			ptr_sr <= 0;
		end

		if(ptr_strobe) begin
			ptr_sr <= { ptr_sr[35:31], 1'b1 };
			ptr <= { ptr[29:0], 6'b0 } | stb_hole;
		end

		if(key_start)
			motor_on <= 1;
		if(key_stop | reset)
			motor_on <= 0;
	end


	// front end interface
	assign fe_data_rq = fe_req;
	wire moving = motor_on & (key_tape_feed | ptr_busy);
	reg fe_req;	// requesting data from FE
	reg fe_rs;	// FE responded with data
	reg [8:1] hole;	// FE data
	reg mid_sync;	// set when mid hole
	reg trail_sync;	// set when trailing edge of feed hole would happen

	wire start_signal = ~moving | ptr_trail;
	wire start_pulse;
	dly50ns fe_dly3(clk, reset, start_signal, start_pulse);

	wire fe_mid_pulse, fe_trail_pulse;
`ifdef simulation
	dly200ns fe_dly0(clk, reset, start_signal, ptr_lead);
	dly800ns fe_dly1(clk, reset, start_signal, fe_mid_pulse);
	dly1us fe_dly2(clk, reset, start_signal, fe_trail_pulse);
`else
	dly1us fe_dly0(clk, reset, start_signal, ptr_lead);
	dly2_1ms fe_dly1(clk, reset, start_signal, fe_mid_pulse);
	dly2_5ms fe_dly2(clk, reset, start_signal, fe_trail_pulse);
`endif

	pa fe_pa0(clk, reset, fe_rs & mid_sync & ptr_busy, ptr_mid);
	pa fe_pa1(clk, reset, fe_rs & trail_sync, ptr_trail);

	always @(posedge clk) begin
		if(~moving | start_pulse) begin
			fe_req <= 0;
			fe_rs <= 0;
			hole <= 0;
			mid_sync <= 0;
			trail_sync <= 0;
		end


		// start FE request
		if(ptr_lead)
			fe_req <= 1;
		// got response from FE
		if(s_write & fe_req) begin
			hole <= s_writedata[7:0];
			fe_req <= 0;
			fe_rs <= 1;
		end
		if(fe_mid_pulse)
			mid_sync <= 1;
		if(fe_trail_pulse)
			trail_sync <= 1;
		// all done
		if(ptr_trail)
			fe_rs <= 0;
	end

endmodule
