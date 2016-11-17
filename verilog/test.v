`timescale 1ns/1ns

module clock(output reg clk);
	initial
		clk = 0;
	always
		#5 clk = ~clk;
endmodule

//`define TESTKEY pdp6.key_inst_stop
`define TESTKEY pdp6.key_read_in
//`define TESTKEY pdp6.key_start
//`define TESTKEY pdp6.key_exec
//`define TESTKEY pdp6.key_ex
//`define TESTKEY pdp6.key_dep
//`define TESTKEY pdp6.key_mem_cont

module test;
	wire clk;
	reg reset;

	clock clock0(clk);
	pdp6 pdp6(.clk(clk), .reset(reset));

	initial begin
//		#110000 $finish;
		#20000 $finish;
	end
	always @(pdp6.apr0.st7)
		if(pdp6.apr0.st7)
			$finish;

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
		pdp6.apr0.cpa_pia = 5;
/*
		pdp6.apr0.pio = 7'b1111111;
		pdp6.apr0.pih = 7'b0000100;
		pdp6.apr0.pi_active = 1;
*/
	end
//	assign pdp6.apr0.iobus_pi_req = 0;
	assign pdp6.apr0.iobus_pi_req = 7'b0010000;

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
		pdp6.fmem0.memsel_p0 = 0;
		pdp6.fmem0.memsel_p1 = 0;
		pdp6.fmem0.memsel_p2 = 0;
		pdp6.fmem0.memsel_p3 = 0;
		pdp6.fmem0.fmc_p0_sel = 1;
		pdp6.fmem0.fmc_p1_sel = 0;
		pdp6.fmem0.fmc_p2_sel = 0;
		pdp6.fmem0.fmc_p3_sel = 0;
		pdp6.mem0.memsel_p0 = 0;
		pdp6.mem0.memsel_p1 = 0;
		pdp6.mem0.memsel_p2 = 0;
		pdp6.mem0.memsel_p3 = 0;

	end

	initial begin
		#80 pdp6.apr0.pr = 8'o003;
		    pdp6.apr0.rlr = 8'o002;
		    //pdp6.apr0.ex_user = 1;
	end

	initial begin
		#1  reset = 1;
		#20 reset = 0;

		pdp6.datasw = 36'o111777222666;
//		pdp6.mas = 18'o010100;
//		pdp6.mas = 18'o000004;
		pdp6.mas = 18'o000023;
		//pdp6.mas = 18'o777777;

		pdp6.fmem0.ff['o0] = 36'o000000_010000;
		pdp6.fmem0.ff['o1] = 36'o000000_000222;
		pdp6.fmem0.ff['o2] = 36'o700000_000006;
		pdp6.fmem0.ff['o3] = 36'o500000_000004;
		pdp6.fmem0.ff['o4] = 36'o000000_010304;
		pdp6.fmem0.ff['o5] = 36'o377777_777777;
		// MOVE 1,@104(4)	FAC_INH
		pdp6.mem0.core['o20] = 36'o200_064_000104;
		// MOVEM 1,@104(4)
		pdp6.mem0.core['o21] = 36'o202_064_000104;
		// ROTC 2,3
		pdp6.mem0.core['o22] = 36'o245_100_000003;

		pdp6.mem0.core['o23] = 36'o700200_000005;

		pdp6.mem0.core['o10410] = 36'o000_000_000333;
	end

	initial begin
		#25 pdp6.sw_power = 1;
		#25 pdp6.sw_power = 0;
	end

endmodule
