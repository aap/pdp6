module tty(
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

	/* UART pins */
	input wire rx,
	output wire tx,

	/* Panel */
	output wire [7:0] tti_ind,
	output wire [6:0] status_ind
);
	assign iobus_dr_split = 0;
	assign iobus_rdi_data = 0;

	wire clk2;
	clk14khz clock2(.inclk(clk),
		.outclk(clk2));

	wire tti_clock, tto_clock;
	clk16div ttidiv(.clk(clk),
		.inclk(clk2),
		.outclk(tti_clock));
	clk4div ttodiv(.clk(clk),
		.inclk(tti_clock),
		.outclk(tto_clock));

	wire tty_sel = iobus_ios == 7'b001_010_0;

	wire tty_data_clr;
	wire tty_data_set;
	wire tty_ic_clr;
	wire tty_ic_set;
	wire tty_reset;
	wire tty_datai = tty_sel & iobus_iob_fm_datai;
	wire tty_status = tty_sel & iobus_iob_fm_status;
	pg tty_pg0(.clk(clk), .reset(reset),
		.in(tty_sel & iobus_datao_clear),
		.p(tty_data_clr));
	pg tty_pg1(.clk(clk), .reset(reset),
		.in(tty_sel & iobus_datao_set),
		.p(tty_data_set));
	pg tty_pg2(.clk(clk), .reset(reset),
		.in(tty_sel & iobus_cono_clear | iobus_iob_reset),
		.p(tty_ic_clr));
	pg tty_pg3(.clk(clk), .reset(reset),
		.in(tty_sel & iobus_cono_set),
		.p(tty_ic_set));
	pg tty_pg4(.clk(clk), .reset(reset),
		.in(iobus_iob_reset),
		.p(tty_reset));

	assign iobus_iob_out =
		tty_datai ? { 28'b0, tti_ind } :
		tty_status ? { 29'b0, tti_busy, tti_flag, tto_busy, tto_flag, tty_pia } :
		36'b0;

	wire [0:7] tty_req = { tti_flag | tto_flag, 7'b0 } >> tty_pia;
	assign iobus_pi_req = tty_req[1:7];

	reg [33:35] tty_pia = 0;
	reg tti_busy = 0;
	reg tti_flag = 0;
	reg tto_busy = 0;
	reg tto_flag = 0;
	wire tto_done;
	reg tto_done0;
	wire tti_done;
	reg tti_done0;
	wire tti_active;
	reg tti_active0;

	assign status_ind = { tti_busy, tti_flag, tto_busy, tto_flag, tty_pia };

	always @(posedge clk) begin
		tti_done0 <= tti_done;
		tto_done0 <= tto_done;
		tti_active0 <= tti_active;

		if(tty_ic_clr)
			tty_pia <= 0;
		if(tty_reset) begin
			tto_busy <= 0;
			tto_flag <= 0;
			tti_busy <= 0;
			tti_flag <= 0;
		end
		if(tty_ic_set) begin
			tty_pia <= iobus_iob_in[33:35];
			if(iobus_iob_in[25])
				tti_busy <= 0;
			if(iobus_iob_in[26])
				tti_flag <= 0;
			if(iobus_iob_in[27])
				tto_busy <= 0;
			if(iobus_iob_in[28])
				tto_flag <= 0;
			if(iobus_iob_in[29])
				tti_busy <= 1;
			if(iobus_iob_in[30])
				tti_flag <= 1;
			if(iobus_iob_in[31])
				tto_busy <= 1;
			if(iobus_iob_in[32])
				tto_flag <= 1;
		end

		if(tty_data_clr) begin
			tto_flag <= 0;
			tto_busy <= 1;
		end
		if(~tto_done0 & tto_done) begin
			tto_flag <= 1;
			tto_busy <= 0;
		end

		if(tty_datai)
			tti_flag <= 0;
		if(~tti_active0 & tti_active)
			tti_busy <= 1;
		if(~tti_done0 & tti_done) begin
			tti_flag <= 1;
			tti_busy <= 0;
		end
	end

	wire [8:1] iob;

	tti tti0(.clk(clk),
		.tti_clock(tti_clock),
		.rx(rx),
		.iob(iob),
		.tti_active(tti_active),
		.tti_done(tti_done),
		.tti(tti_ind));
	tto tto0(.clk(clk),
		.tto_clock(tto_clock),
		.iob(iobus_iob_in[28:35]),
		.tty_data_clr(tty_data_clr),
		.tty_data_set(tty_data_set),
		.tx(tx),
		.tto_done(tto_done));

endmodule

module tto(input wire clk,
	input wire tto_clock,
	input wire [8:1] iob,
	input wire tty_data_clr,
	input wire tty_data_set,
	output wire tx,
	output reg tto_done = 0
);
	reg [8:1] tto;
	reg tto_out_line;
	reg tto_enable = 0;
	reg tto_active = 0;
	reg tto_active0;
	reg tto_div2 = 0;
	reg tto_div20;
	wire tto_4count;

	wire tto_shift = tto_div20 & ~tto_div2;

	count4 c(.clk(clk),
		.reset(tto_active0 & ~tto_active),
		.enable(tto_clock),
		.out(tto_4count));

	always @(posedge clk) begin
		tto_active0 <= tto_active;
		tto_div20 <= tto_div2;

		if(tty_data_clr) begin
			tto_done <= 0;
		end
		if(tty_data_set) begin
			tto <= iob;
			tto_enable <= 1;
		end
		if(tto_clock) begin
			if(tto_active)
				tto_div2 <= ~tto_div2;
			if(tto_4count & tto_enable)
				tto_active <= 1;
		end
		if(tto_shift) begin
			tto_enable <= 0;
			{ tto, tto_out_line } <= { tto_enable, tto };
			if(~tto_enable & tto[8:2] == 0) begin
				tto_active <= 0;
				tto_done <= 1;
			end
		end
		if(~tto_active)
			tto_out_line <= 1;
		else
			if(~tto_active0)
				tto_out_line <= 0;
	end
	assign tx = tto_out_line;
endmodule

module tti(input wire clk,
	input wire tti_clock,
	input wire rx,
	output wire [8:1] iob,
	output reg tti_active = 0,
	output reg tti_done = 0,
	output reg [8:1] tti = 0
);
	assign iob = tti;

	wire tti_shift = tti_4count_rise & ~tti_last_unit;
	reg tti_last_unit = 0;
	reg tti_active0;
	wire tti_4count;
	reg tti_4count0;

	wire tti_space = ~rx;
	wire tti_4count_rise = ~tti_4count0 & tti_4count;
	wire tti_set = ~tti_active0 & tti_active;

	div8 d(.clk(clk),
		.reset(tti_set),
		.enable(tti_clock & tti_active),
		.out(tti_4count));

	always @(posedge clk) begin
		tti_4count0 <= tti_4count;
		tti_active0 <= tti_active;

		if(tti_set) begin
			tti <= 8'o377;
			tti_last_unit <= 0;
		end
		if(tti_4count_rise & tti_last_unit)
			tti_active <= 0;
		if(tti_shift) begin
			tti <= { rx, tti[8:2] };
			if(~tti[1]) begin
				tti_last_unit <= 1;
				tti_done <= 1;
			end
			if(tti[1])
				tti_done <= 0;
			if(~tti_space & (& tti))
				tti_active <= 0;
		end
		if(tti_clock)
			if(~tti_active & tti_space)
				tti_active <= 1;
	end
endmodule

module clk14khz(input wire inclk,
	output wire outclk);
	reg [11:0] cnt = 0;
	assign outclk = cnt == 3551;
	always @(posedge inclk)
		if(outclk)
			cnt <= 0;
		else
			cnt <= cnt + 12'b1;
endmodule

module clk16div(input wire clk,
	input wire inclk,
	output wire outclk
);
	reg [4:0] cnt = 0;
	assign outclk = cnt == 16;
	always @(posedge clk)
		if(outclk)
			cnt <= 0;
		else if(inclk)
			cnt <= cnt + 5'b1;

endmodule

module div8(
	input wire clk,
	input wire reset,
	input wire enable,
	output wire out
);
	reg [2:0] cnt = 4;
	always @(posedge clk)
		if(reset)
			cnt <= 0;
		else if(enable)
			cnt <= cnt + 3'b1;
	assign out = cnt[2];
endmodule

module count4(
	input wire clk,
	input wire reset,
	input wire enable,
	output wire out
);
	reg [1:0] cnt = 0;
	always @(posedge clk)
		if(reset)
			cnt <= 0;
		else if(enable && cnt != 3)
			cnt <= cnt + 2'b1;
	assign out = cnt == 3;
endmodule

module clk4div(input wire clk,
	input wire inclk,
	output wire outclk
);
	reg [2:0] cnt = 0;
	assign outclk = cnt == 4;
	always @(posedge clk)
		if(outclk)
			cnt <= 0;
		else if(inclk)
			cnt <= cnt + 3'b1;
endmodule
