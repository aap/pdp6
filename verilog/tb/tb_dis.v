`default_nettype none
`timescale 1ns/1ns
`define simulation

module tb_dis();

	wire clk, reset;
	clock clock(clk, reset);

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

	reg read = 0;

	dis340 dis(.clk(clk), .reset(~reset),

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

		.s_read(read));

task cono;
	input [18:35] data;
begin
	iobus_iob_in <= data;
	iobus_cono_clear <= 1;
	#20;
	iobus_cono_clear <= 0;
	#20;
	iobus_cono_set <= 1;
	#20;
	iobus_cono_set <= 0;
	#20;
end
endtask

task datao;
	input [0:35] data;
begin
	@(posedge dis.dis_flag_data);
	#600;
	@(posedge clk);
	iobus_iob_in <= data;
	iobus_datao_clear <= 1;
	#20;
	iobus_datao_clear <= 0;
	#20;
	iobus_datao_set <= 1;
	#20;
	iobus_datao_set <= 0;
	#20;
end
endtask

task disp;
	input [0:17] word;
begin
	list[i] = word;
	i = i + 1;
end
endtask

	reg [0:17] list[0:1000];
	integer i, n;

	wire [0:18] Esc  = 18'o400000;
	wire [0:18] Vert = 18'o200000;
	wire [0:18] Iv   = 18'o200000;
	wire [0:18] Ixy  = 18'o002000;
	wire [0:18] Stop = 18'o002000;
	wire [0:18] PM   = 18'o000000;
	wire [0:18] XYM  = 18'o020000;
	wire [0:18] CM   = 18'o060000;
	wire [0:18] VM   = 18'o100000;
	wire [0:18] VCM  = 18'o120000;
	wire [0:18] IM   = 18'o140000;

	initial begin
		$dumpfile("dump.vcd");
		$dumpvars();

		for(i = 0; i < 1000; i = i + 1)
			list[i] = 0;

		i = 0;
/*
		disp(XYM | 'o100 | 'o17);
		disp(XYM | 'o77);
		disp(XYM | Vert | 'o66 | Ixy);
		disp(VM | Vert | 'o55 | Ixy);
		disp(Esc | Iv | { 8'o100, 8'o25 });
		disp(XYM | 'o160);
		disp(XYM | 'o1000);
		disp(IM | 'o1000 | Vert);
		disp(Iv | { 4'b1000, 4'b1000, 4'b1000, 4'b1000 });
		disp(Iv | { 4'b0010, 4'b0010, 4'b0010, 4'b0010 });
		disp(Iv | { 4'b1100, 4'b1100, 4'b1100, 4'b1100 });
		disp(Iv | { 4'b0011, 4'b0011, 4'b0011, 4'b0011 } | Esc);
		disp(VCM);
		disp(Esc | Iv | { 8'b1, 8'b1 });
*/
		disp(XYM | 'o160 | 'o17);
		disp(XYM | 'o400 | Vert);
		disp(CM | 'o1000);
		disp({ 6'o01, 6'o36, 6'o03 });
		disp({ 6'o37, 6'o02, 6'o03 });
		disp(Stop | 'o1000);
		n = i;

		for(i = 0; i < n; i = i + 1)
			$display("list[%d] = %o", i, list[i]);


		#100;
		iobus_iob_reset <= 1;
		#100;
		iobus_iob_reset <= 0;
		#100;
		iobus_ios <= 7'b001_011_0;


		#200;
		cono(18'o100);

		for(i = 0; i < n; i = i + 2)
			datao({ list[i], list[i+1] });
/*
		#2000;
		$display("");

		cono(18'o100);
		for(i = 0; i < n; i = i + 2)
			datao({ list[i], list[i+1] });
*/
	end

	initial begin
		while(1) begin
		//	@(posedge dis.intensify);
		//	$display("%o %o %o", dis.i, dis.x, dis.y);

			@(posedge dis.fe_data_rq);
			#700;
			read <= 1;
			#1;
			$display("0x%X, // %t", dis.s_readdata, $time);
		//	$display("%o %o %o", dis.i, dis.x, dis.y);
			@(posedge clk);
			read <= 0;
		end
	end

	initial begin
		@(posedge dis.stop);
		#100;
		$finish;
	end

	initial begin
		#2000000;
		$finish;
	end
endmodule
