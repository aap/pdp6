`timescale 1ns/1ns

module clock(
	output clk
);
	reg clk;
	initial
		clk = 0;
	always
		#25 clk = ~clk;
	initial
		#20000 $finish;
//		#150000 $finish;

endmodule

//`define TESTKEY key_start
//`define TESTKEY key_read_in
//`define TESTKEY key_ex_nxt
`define TESTKEY key_ex
//`define TESTKEY key_dep
//`define TESTKEY key_mem_cont

module test;
	reg key_start, key_read_in;
	reg key_inst_cont, key_mem_cont;
	reg key_inst_stop, key_mem_stop;
	reg key_exec, key_ioreset;
	reg key_dep, key_dep_nxt;
	reg key_ex, key_ex_nxt;
	reg sw_addr_stop;
	reg sw_mem_disable;
	reg sw_repeat;
	reg sw_power;
	reg [18:35] mas;
	reg [0:35]  datasw;
	reg sw_rim_maint;

	wire  [0:35] mi;
	wire [21:35] ma_p0;
	wire [18:21] sel_p0;
	wire  [0:35] mb_in_p0;
	wire  [0:35] mb_out_p0_p;
	wire  [0:35] mb_out_p0_0;
	wire  [0:35] mb_out_p0_1;

	clock clock(.clk(clk));
	apr apr(
		.clk(clk),
		.key_start(key_start),
		.key_read_in(key_read_in),
		.key_inst_cont(key_inst_cont),
		.key_mem_cont(key_mem_cont),
		.key_inst_stop(key_inst_stop),
		.key_mem_stop (key_mem_stop),
		.key_exec(key_exec),
		.key_ioreset(key_ioreset),
		.key_dep(key_dep),
		.key_dep_nxt(key_dep_nxt),
		.key_ex(key_ex),
		.key_ex_nxt(key_ex_nxt),
		.sw_addr_stop(sw_addr_stop),
		.sw_mem_disable(sw_mem_disable),
		.sw_repeat(sw_repeat),
		.sw_power(sw_power),
		.sw_rim_maint(sw_rim_maint),
		.mas(mas),
		.datasw(datasw),
		.mi(mi),

		.membus_mc_wr_rs(mc_wr_rs_p0),
		.membus_mc_rq_cyc(mc_rq_cyc_p0),
		.membus_mc_rd_rq(mc_rd_rq_p0),
		.membus_mc_wr_rq(mc_wr_rq_p0),
		.membus_ma(ma_p0),
		.membus_sel(sel_p0),
		.membus_fmc_select(fmc_select_p0),
		.membus_mb_out(mb_out_p0_p),

		.membus_mai_cmc_addr_ack(cmc_addr_ack_p0),
		.membus_mai_cmc_rd_rs(cmc_rd_rs_p0),
		.membus_mb_in(mb_in_p0)
	);

 	assign cmc_addr_ack_p0 = cmc_addr_ack_p0_0 | cmc_addr_ack_p0_1;
	assign cmc_rd_rs_p0 = cmc_rd_rs_p0_0 | cmc_rd_rs_p0_1;
	assign mb_in_p0 = mb_out_p0_p | mb_out_p0_0 | mb_out_p0_1;

	fastmem fmem0(
		.clk(clk),

		.mc_wr_rs_p0(mc_wr_rs_p0),
		.mc_rq_cyc_p0(mc_rq_cyc_p0),
		.mc_rd_rq_p0(mc_rd_rq_p0),
		.mc_wr_rq_p0(mc_wr_rq_p0),
		.ma_p0(ma_p0),
		.sel_p0(sel_p0),
		.fmc_select_p0(fmc_select_p0),
		.mb_in_p0(mb_in_p0),

		.cmc_addr_ack_p0(cmc_addr_ack_p0_0),
		.cmc_rd_rs_p0(cmc_rd_rs_p0_0),
		.mb_out_p0(mb_out_p0_0),

		.mc_rq_cyc_p1(1'b1),
		.sel_p1(4'b0000),
		.fmc_select_p1(1'b1),

		.mc_rq_cyc_p2(1'b1),
		.sel_p2(4'b0000),
		.fmc_select_p2(1'b1),

		.mc_rq_cyc_p3(1'b1),
		.sel_p3(4'b0000),
		.fmc_select_p3(1'b1)
	);

	coremem16k mem0(
		.clk(clk),

		.mc_wr_rs_p0(mc_wr_rs_p0),
		.mc_rq_cyc_p0(mc_rq_cyc_p0),
		.mc_rd_rq_p0(mc_rd_rq_p0),
		.mc_wr_rq_p0(mc_wr_rq_p0),
		.ma_p0(ma_p0),
		.sel_p0(sel_p0),
		.fmc_select_p0(fmc_select_p0),
		.mb_in_p0(mb_in_p0),

		.cmc_addr_ack_p0(cmc_addr_ack_p0_1),
		.cmc_rd_rs_p0(cmc_rd_rs_p0_1),
		.mb_out_p0(mb_out_p0_1),

		.mc_rq_cyc_p1(1'b0),
		.sel_p1(4'b0000),
		.fmc_select_p1(1'b0),

		.mc_rq_cyc_p2(1'b0),
		.sel_p2(4'b0000),
		.fmc_select_p2(1'b0),

		.mc_rq_cyc_p3(1'b0),
		.sel_p3(4'b0000),
		.fmc_select_p3(1'b0)
	);

	initial begin
//		#1000 apr.rlr = 8'o201;
//		apr.ex_user = 1;
	end

	integer i;
	initial begin
		mem0.memsel_p0 = 0;
		mem0.memsel_p1 = 0;
		mem0.memsel_p2 = 0;
		mem0.memsel_p3 = 0;

		fmem0.memsel_p0 = 0;
		fmem0.memsel_p1 = 0;
		fmem0.memsel_p2 = 0;
		fmem0.memsel_p3 = 0;
		fmem0.fmc_p0_sel = 1;
		fmem0.fmc_p1_sel = 0;
		fmem0.fmc_p2_sel = 0;
		fmem0.fmc_p3_sel = 0;

		for(i = 0; i < 040000; i = i+1)
			mem0.core[i] = 0;
		mem0.core[4] = 36'o222333111666;
		mem0.core['o20] = 36'o123234345456;

		for(i = 0; i < 16; i = i + 1)
			fmem0.ff[i] = i+1 | 36'o50000;

		key_start <= 0;
		key_read_in <= 0;
		key_inst_cont <= 0;
		key_mem_cont <= 0;
		key_inst_stop <= 0;
		key_mem_stop <= 0;
		key_exec <= 0;
		key_ioreset <= 0;
		key_dep <= 0;
		key_dep_nxt <= 0;
		key_ex <= 0;
		key_ex_nxt <= 0;
		sw_addr_stop <= 0;
		sw_mem_disable <= 0;
		sw_repeat <= 0;
		sw_power <= 0;
		sw_rim_maint <= 0;
//		mas <= 18'o777777;
//		mas <= 18'o000000;
		mas <= 18'o000004;
//		mas <= 18'o000020;
//		mas <= 18'o000104;
//		mas <= 18'o300004;
		datasw <= 36'o123456654321;

		$dumpfile("dump.vcd");
		$dumpvars();
	end

	initial begin
		#10 sw_power = 1;
	end

	initial begin
		#400 `TESTKEY = 1;
		#1000 `TESTKEY = 0;
	end
//	initial begin
//		#7000;
//		#400 key_dep = 1;
//		#1000 key_dep = 0;
//	end

endmodule
