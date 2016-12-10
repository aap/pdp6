`timescale 1ns/1ns
`define simulation

module clock(output reg clk);
	initial
		clk = 0;
	always
		#5 clk = ~clk;
endmodule

//`define TESTKEY pdp6.key_inst_stop
//`define TESTKEY pdp6.key_read_in
`define TESTKEY pdp6.key_start
//`define TESTKEY pdp6.key_exec
//`define TESTKEY pdp6.key_ex
//`define TESTKEY pdp6.key_dep
//`define TESTKEY pdp6.key_mem_cont

module test;
	wire clk;
	reg reset;
	reg stop;

	clock clock0(clk);
	pdp6 pdp6(.clk(clk), .reset(reset));

	initial begin
		stop = 0;
//		#110000 stop = 1;
		#20000 stop = 1;
	end
	always @(pdp6.apr0.st7)
		if(pdp6.apr0.st7)
			stop = 1;

	// dump memory on exit
	always @(stop)
		if(stop) begin: fin
			integer i;
			#4000;
			for(i = 0; i < 'o20; i = i + 1)
				$display("%o %o %o", i, pdp6.mem0.core[i], pdp6.fmem0.ff[i]);
			for(i = 'o1000; i < 'o1010; i = i + 1)
				$display("%o %o", i, pdp6.mem0.core[i]);
			$finish;
		end

	initial begin
		#100 `TESTKEY = 1;
		#1000 `TESTKEY = 0;

//		#3000 pdp6.key_dep = 1;
//		#1000 pdp6.key_dep = 0;

//		#3000 pdp6.key_inst_stop = 1;
//		#1000 pdp6.key_inst_stop = 0;
	end

	initial begin
		#400;
//		pdp6.apr0.cpa_pia = 5;
		pdp6.apr0.pio = 7'b1111100;
		pdp6.apr0.pir = 7'b0000000;
		pdp6.apr0.pih = 7'b0000100;
		#10;
		pdp6.apr0.pi_active = 1;
	end
//	assign pdp6.apr0.iobus_pi_req = 7'b0010000;
	assign pdp6.apr0.iobus_pi_req = 0;

/*
	initial begin
		#300;
		pdp6.apr0.cpa_iot_user <= 1;
		#20;
		pdp6.apr0.cpa_illeg_op <= 1;
		#20;
		pdp6.apr0.cpa_non_exist_mem <= 1;
		#20;
		pdp6.apr0.cpa_clock_enable <= 1;
		#20;
		pdp6.apr0.cpa_clock_flag <= 1;
		#20;
		pdp6.apr0.cpa_pc_chg_enable <= 1;
		#20;
		pdp6.apr0.cpa_pdl_ov <= 1;
		#20;
		pdp6.apr0.cpa_arov_enable <= 1;
		#20;
		pdp6.apr0.cpa_pia <= 7;
	end
*/

/*	initial begin
		#100;
		pdp6.mem0_sw_single_step = 1;
		#6000;
		pdp6.mem0_sw_restart = 1;
	end*/

	initial begin
		$dumpfile("dump.vcd");
		$dumpvars();

		reset = 0;

		pdp6.key_start = 0;
		pdp6.key_read_in = 0;
		pdp6.key_mem_cont = 0;
		pdp6.key_inst_cont = 0;
		pdp6.key_mem_stop = 0;
		pdp6.key_inst_stop = 0;
		pdp6.key_exec = 0;
		pdp6.key_io_reset = 0;
		pdp6.key_dep = 0;
		pdp6.key_dep_nxt = 0;
		pdp6.key_ex = 0;
		pdp6.key_ex_nxt = 0;

		pdp6.sw_power = 0;
		pdp6.sw_addr_stop = 0;
		pdp6.sw_mem_disable = 0;
		pdp6.sw_repeat = 0;
		pdp6.sw_power = 0;
		pdp6.datasw = 0;
		pdp6.mas = 0;

		pdp6.sw_rim_maint = 0;
		pdp6.sw_repeat_bypass = 0;
		pdp6.sw_art3_maint = 0;
		pdp6.sw_sct_maint = 0;
		pdp6.sw_split_cyc = 0;

		pdp6.mem0_sw_single_step = 0;
		pdp6.mem0_sw_restart = 0;
	end

/*
	initial begin
		#80 pdp6.apr0.pr = 8'o003;
		    pdp6.apr0.rlr = 8'o002;
		    //pdp6.apr0.ex_user = 1;
	end
*/

	initial begin: meminit
		integer i;
		#1  reset = 1;
		#20 reset = 0;

		pdp6.datasw = 36'o111777222666;
		pdp6.mas = 18'o000000;

		for(i = 0; i < 'o40000; i = i + 1)
			pdp6.mem0.core[i] = 0;
		for(i = 0; i < 'o20; i = i + 1)
			pdp6.fmem0.ff[i] = 0;

		//`include "test1.inc"
		`include "test2.inc"
	end

	wire [0:35] mem0scope = pdp6.mem0.core['o1000];
	wire [0:35] fmem0scope = pdp6.fmem0.ff[2];

	initial begin
		#25 pdp6.sw_power = 1;
	end

endmodule
