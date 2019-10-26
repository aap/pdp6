`default_nettype none
`timescale 1ns/1ns
`define simulation

module tb_ptp();

	wire clk, reset;
	clock clock(clk, reset);

	reg read = 0;
	wire [31:0] readdata;
	reg [31:0] data;

	reg iobus_iob_poweron = 1;
	reg iobus_iob_reset = 0;
	reg iobus_datao_clear = 0;
	reg iobus_datao_set = 0;
	reg iobus_cono_clear = 0;
	reg iobus_cono_set = 0;
	reg iobus_iob_fm_datai = 0;
	reg iobus_iob_fm_status = 0;
	reg [3:9]  iobus_ios = 0;
	reg [0:35] iobus_iob_in = 0;
	wire [1:7]  iobus_pi_req;
	wire [0:35] iobus_iob_out;

	reg key_tape_feed = 0;

	wire data_rq;

	ptp ptp(.clk(clk), .reset(~reset),

		.iobus_iob_poweron(iobus_iob_poweron),
		.iobus_iob_reset(iobus_iob_reset),
		.iobus_datao_clear(iobus_datao_clear),
		.iobus_datao_set(iobus_datao_set),
		.iobus_cono_clear(iobus_cono_clear),
		.iobus_cono_set(iobus_cono_set),
		.iobus_iob_fm_datai(iobus_iob_fm_datai),
		.iobus_iob_fm_status(iobus_iob_fm_status),
		.iobus_ios(iobus_ios),
		.iobus_iob_in(iobus_iob_in),
		.iobus_pi_req(iobus_pi_req),
		.iobus_iob_out(iobus_iob_out),

		.key_tape_feed(key_tape_feed),

		.s_read(read),
		.s_readdata(readdata),

		.fe_data_rq(data_rq));


	initial begin
		$dumpfile("dump.vcd");
		$dumpvars();

		#100;
		iobus_iob_reset <= 1;
		#100;
		iobus_iob_reset <= 0;
		#100;
		iobus_ios <= 7'b001_000_0;


		#200;
		key_tape_feed <= 1;
		ptp.ptp_pia <= 1;
		ptp.ptp_flag <= 0;
//		ptp.ptp_busy <= 1;
		ptp.ptp <= 'o123;

/*
		#200;
		ptp.ptp_busy <= 0;
		#2200;
		ptp.ptp_busy <= 1;
*/
	end

	initial begin: foo
		integer i;

		@(posedge data_rq);
		for(i = 0; i < 20; i = i+1) begin
			@(posedge clk);
		end
		read <= 1;
		@(posedge clk);
		data <= readdata;
		read <= 0;

/*
		@(posedge (|iobus_pi_req));
		@(posedge clk);
		@(posedge clk);
		@(posedge clk);
		@(posedge clk);
		@(posedge clk);
		@(posedge clk);
		@(posedge clk);
		iobus_datao_clear <= 1;
		@(posedge clk);
		@(posedge clk);
		@(posedge clk);
		iobus_datao_clear <= 0;
		iobus_datao_set <= 1;
		iobus_iob_in <= 'o321;
		@(posedge clk);
		@(posedge clk);
		@(posedge clk);
		iobus_datao_set <= 0;
*/

		@(posedge data_rq);
		for(i = 0; i < 200; i = i+1) begin
			@(posedge clk);
		end
		read <= 1;
		@(posedge clk);
		data <= readdata;
		read <= 0;

/*
		@(posedge data_rq);
		for(i = 0; i < 20; i = i+1) begin
			@(posedge clk);
		end
		write <= 1;
		writedata <= 'o255;
		@(posedge clk);
		write <= 0;

		@(posedge data_rq);
		for(i = 0; i < 20; i = i+1) begin
			@(posedge clk);
		end
		write <= 1;
		writedata <= 'o244;
		@(posedge clk);
		write <= 0;

		@(posedge data_rq);
		for(i = 0; i < 20; i = i+1) begin
			@(posedge clk);
		end
		write <= 1;
		writedata <= 'o233;
		@(posedge clk);
		write <= 0;

		@(posedge data_rq);
		for(i = 0; i < 20; i = i+1) begin
			@(posedge clk);
		end
		write <= 1;
		writedata <= 'o222;
		@(posedge clk);
		write <= 0;


		@(posedge (|iobus_pi_req));
		iobus_iob_fm_datai <= 1;
		#400;
		iobus_iob_fm_datai <= 0;

		key_stop <= 1;
		#20;
		key_stop <= 0;
*/
	end

	initial begin
		#50000;
		$finish;
	end
endmodule
