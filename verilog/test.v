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
			for(i = 0; i < 'o50; i = i + 1)
				if(i < 'o20)
					$display("%o %o %o", i, pdp6.mem0.core[i], pdp6.fmem0.ff[i]);
				else
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

/*
	initial begin
		#80 pdp6.apr0.pr = 8'o003;
		    pdp6.apr0.rlr = 8'o002;
		    //pdp6.apr0.ex_user = 1;
	end
*/

	initial begin
		#1  reset = 1;
		#20 reset = 0;

		pdp6.datasw = 36'o111777222666;
		pdp6.mas = 18'o000034;

		pdp6.fmem0.ff['o0] = 36'o000000_010000;
		pdp6.fmem0.ff['o1] = 36'o000000_010222;
		pdp6.fmem0.ff['o2] = 36'o700000_200006;
		pdp6.fmem0.ff['o3] = 36'o500000_000004;
		pdp6.fmem0.ff['o4] = 36'o000000_010304;
		pdp6.fmem0.ff['o5] = 36'o377777_777777;
		pdp6.fmem0.ff['o6] = 36'o444000_222000;
		pdp6.fmem0.ff['o17] = 36'o777000_001000;	// PDL ptr
//		pdp6.fmem0.ff['o17] = 36'o777000_777777;	// PDL ptr
		pdp6.mem0.core['o20] = 36'o200_064_000104;	// MOVE 1,@104(4)	FAC_INH
		pdp6.mem0.core['o21] = 36'o202_064_000104;	// MOVEM 1,@104(4)
		pdp6.mem0.core['o22] = 36'o245_100_000003;	// ROTC 2,3
		pdp6.mem0.core['o23] = 36'o700200_675550;	// CONO	APR,675550
		pdp6.mem0.core['o24] = 36'o700200_102227;	// CONO	APR,102227
		pdp6.mem0.core['o25] = 36'o700240_000005;	// CONI	APR,5
		pdp6.mem0.core['o26] = 36'o700140_000006;	// DATAO	APR,6
		pdp6.mem0.core['o27] = 36'o700040_000005;	// DATAI	APR,5
		pdp6.mem0.core['o30] = 36'o700640_000005;	// CONI	APR,5
		pdp6.mem0.core['o31] = 36'o260740_000020;	// PUSHJ 17,20
		pdp6.mem0.core['o31] = 36'o250040_000000;	// AOS	1,
		pdp6.mem0.core['o32] = 36'o270000_000001;	// ADD	0,1
		pdp6.mem0.core['o33] = 36'o274000_000001;	// SUB	0,1

		pdp6.mem0.core['o34] = 36'o245_100_000003;	// ROTC 2,3
		pdp6.mem0.core['o35] = 36'o245_100_777775;	// ROTC 2,-3
		pdp6.mem0.core['o36] = 36'o244_100_000001;	// ASHC 2,1

		pdp6.mem0.core['o10410] = 36'o000_000_000333;
	end

	initial begin
		#25 pdp6.sw_power = 1;
	end

endmodule
