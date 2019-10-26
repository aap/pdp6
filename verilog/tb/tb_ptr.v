`default_nettype none
`timescale 1ns/1ns
`define simulation

module tb_ptr();

	wire clk, reset;
	clock clock(clk, reset);

	reg write = 0;
	reg [31:0] writedata = 0;

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

	reg key_start = 0;
	reg key_stop = 0;
	reg key_tape_feed = 0;

	wire data_rq;

	ptr ptr(.clk(clk), .reset(~reset),

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

		.key_start(key_start),
		.key_stop(key_stop),
		.key_tape_feed(key_tape_feed),

		.s_write(write),
		.s_writedata(writedata),

		.fe_data_rq(data_rq));


	initial begin
		$dumpfile("dump.vcd");
		$dumpvars();

		#100;
		iobus_iob_reset <= 1;
		#100;
		iobus_iob_reset <= 0;
		#100;
		iobus_ios <= 7'b001_000_1;


		key_start <= 1;
		#20;
		key_start <= 0;
		#200;
//		key_tape_feed <= 1;
		ptr.ptr_pia <= 1;
		ptr.ptr_flag <= 0;
		ptr.ptr_busy <= 1;
		ptr.ptr <= 1;
	end

	initial begin: foo
		integer i;

		@(posedge data_rq);
		for(i = 0; i < 20; i = i+1) begin
			@(posedge clk);
		end
		write <= 1;
		writedata <= 'o277;
		@(posedge clk);
		write <= 0;

/*
		@(posedge data_rq);
		for(i = 0; i < 20; i = i+1) begin
			@(posedge clk);
		end
		write <= 1;
		writedata <= 'o266;
		@(posedge clk);
		write <= 0;

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
		#40000;
		$finish;
	end
endmodule
