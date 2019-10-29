`default_nettype none
`timescale 1ns/1ns
`define simulation

module tb_apr();

	wire clk, reset;
	clock clock(clk, reset);

	// membus
	wire membus_rq_cyc;
	wire membus_rd_rq;
	wire membus_wr_rq;
	wire [21:35] membus_ma;
	wire [18:21] membus_sel;
	wire membus_fmc_select;
	wire [0:35] membus_mb_write;
	wire membus_wr_rs;

	wire [0:35] membus_mb_read = membus_mb_write | membus_mb_read_0 | membus_mb_read_1;
	wire membus_addr_ack = membus_addr_ack_0 | membus_addr_ack_1;
	wire membus_rd_rs = membus_rd_rs_0 | membus_rd_rs_1;

	// iobus
	wire iobus_iob_poweron;
	wire iobus_iob_reset;
	wire iobus_datao_clear;
	wire iobus_datao_set;
	wire iobus_cono_clear;
	wire iobus_cono_set;
	wire iobus_iob_fm_datai;
	wire iobus_iob_fm_status;
	wire [3:9]  iobus_ios;
	wire [0:35] iobus_iob_in;
	wire [1:7]  iobus_pi_req = 0;
	wire [0:35] iobus_iob_out = 0;


	reg key_start = 0;
	reg key_read_in = 0;
	reg key_mem_cont = 0;
	reg key_inst_cont = 0;
	reg key_mem_stop = 0;
	reg key_inst_stop = 0;
	reg key_exec = 0;
	reg key_io_reset = 0;
	reg key_dep = 0;
	reg key_dep_nxt = 0;
	reg key_ex = 0;
	reg key_ex_nxt = 0;

	reg sw_addr_stop = 0;
	reg sw_mem_disable = 0;
	reg sw_repeat = 0;
	reg sw_power = 0;
	reg [0:35] datasw = 0;
	reg [18:35] mas = 0;

	reg sw_rim_maint = 0;
	reg sw_repeat_bypass = 0;
	reg sw_art3_maint = 0;
	reg sw_sct_maint = 0;
	reg sw_split_cyc = 0;

	apr apr(
		.clk(clk),
		.reset(~reset),

		.key_start(key_start),
		.key_read_in(key_read_in),
		.key_mem_cont(key_mem_cont),
		.key_inst_cont(key_inst_cont),
		.key_mem_stop(key_mem_stop),
		.key_inst_stop(key_inst_stop),
		.key_exec(key_exec),
		.key_io_reset(key_io_reset),
		.key_dep(key_dep),
		.key_dep_nxt(key_dep_nxt),
		.key_ex(key_ex),
		.key_ex_nxt(key_ex_nxt),

		.sw_addr_stop(sw_addr_stop),
		.sw_mem_disable(sw_mem_disable),
		.sw_repeat(sw_repeat),
		.sw_power(sw_power),
		.datasw(datasw),
		.mas(mas),

		.sw_rim_maint(sw_rim_maint),
		.sw_repeat_bypass(sw_repeat_bypass),
		.sw_art3_maint(sw_art3_maint),
		.sw_sct_maint(sw_sct_maint),
		.sw_split_cyc(sw_split_cyc),

		.membus_wr_rs(membus_wr_rs),
		.membus_rq_cyc(membus_rq_cyc),
		.membus_rd_rq(membus_rd_rq),
		.membus_wr_rq(membus_wr_rq),
		.membus_ma(membus_ma),
		.membus_sel(membus_sel),
		.membus_fmc_select(membus_fmc_select),
		.membus_mb_out(membus_mb_write),
		.membus_addr_ack(membus_addr_ack),
		.membus_rd_rs(membus_rd_rs),
		.membus_mb_in(membus_mb_read),

		.iobus_pi_req(iobus_pi_req),
		.iobus_iob_in(iobus_iob_in)
	);


	wire [0:35] membus_mb_read_0;
	wire membus_addr_ack_0;
	wire membus_rd_rs_0;
	core161c cmem(
		.clk(clk),
		.reset(~reset),
		.power(1'b1),
		.sw_single_step(1'b0),
		.sw_restart(1'b0),

		.membus_rq_cyc_p0(membus_rq_cyc),
		.membus_rd_rq_p0(membus_rd_rq),
		.membus_wr_rq_p0(membus_wr_rq),
		.membus_ma_p0(membus_ma),
		.membus_sel_p0(membus_sel),
		.membus_fmc_select_p0(membus_fmc_select),
		.membus_mb_in_p0(membus_mb_write),
		.membus_wr_rs_p0(membus_wr_rs),
		.membus_mb_out_p0(membus_mb_read_0),
		.membus_addr_ack_p0(membus_addr_ack_0),
		.membus_rd_rs_p0(membus_rd_rs_0),

		.membus_wr_rs_p1(1'b0),
		.membus_rq_cyc_p1(1'b0),
		.membus_rd_rq_p1(1'b0),
		.membus_wr_rq_p1(1'b0),
		.membus_ma_p1(15'b0),
		.membus_sel_p1(4'b0),
		.membus_fmc_select_p1(1'b0),
		.membus_mb_in_p1(36'b0),

		.membus_wr_rs_p2(1'b0),
		.membus_rq_cyc_p2(1'b0),
		.membus_rd_rq_p2(1'b0),
		.membus_wr_rq_p2(1'b0),
		.membus_ma_p2(15'b0),
		.membus_sel_p2(4'b0),
		.membus_fmc_select_p2(1'b0),
		.membus_mb_in_p2(36'b0),

		.membus_wr_rs_p3(1'b0),
		.membus_rq_cyc_p3(1'b0),
		.membus_rd_rq_p3(1'b0),
		.membus_wr_rq_p3(1'b0),
		.membus_ma_p3(15'b0),
		.membus_sel_p3(4'b0),
		.membus_fmc_select_p3(1'b0),
		.membus_mb_in_p3(36'b0)
	);

	wire [0:35] membus_mb_read_1;
	wire membus_addr_ack_1;
	wire membus_rd_rs_1;
	fast162 fmem(
		.clk(clk),
		.reset(~reset),
		.power(1'b1),
		.sw_single_step(1'b0),
		.sw_restart(1'b0),

		.membus_rq_cyc_p0(membus_rq_cyc),
		.membus_rd_rq_p0(membus_rd_rq),
		.membus_wr_rq_p0(membus_wr_rq),
		.membus_ma_p0(membus_ma),
		.membus_sel_p0(membus_sel),
		.membus_fmc_select_p0(membus_fmc_select),
		.membus_mb_in_p0(membus_mb_write),
		.membus_wr_rs_p0(membus_wr_rs),
		.membus_mb_out_p0(membus_mb_read_1),
		.membus_addr_ack_p0(membus_addr_ack_1),
		.membus_rd_rs_p0(membus_rd_rs_1),

		.membus_wr_rs_p1(1'b0),
		.membus_rq_cyc_p1(1'b0),
		.membus_rd_rq_p1(1'b0),
		.membus_wr_rq_p1(1'b0),
		.membus_ma_p1(15'b0),
		.membus_sel_p1(4'b0),
		.membus_fmc_select_p1(1'b0),
		.membus_mb_in_p1(36'b0),

		.membus_wr_rs_p2(1'b0),
		.membus_rq_cyc_p2(1'b0),
		.membus_rd_rq_p2(1'b0),
		.membus_wr_rq_p2(1'b0),
		.membus_ma_p2(15'b0),
		.membus_sel_p2(4'b0),
		.membus_fmc_select_p2(1'b0),
		.membus_mb_in_p2(36'b0),

		.membus_wr_rs_p3(1'b0),
		.membus_rq_cyc_p3(1'b0),
		.membus_rd_rq_p3(1'b0),
		.membus_wr_rq_p3(1'b0),
		.membus_ma_p3(15'b0),
		.membus_sel_p3(4'b0),
		.membus_fmc_select_p3(1'b0),
		.membus_mb_in_p3(36'b0)
	);

	initial begin: init
		integer i;

		$dumpfile("dump.vcd");
		$dumpvars();

		for(i = 0; i < 'o40000; i = i + 1)
			cmem.core[i] <= 0;

		#10;

		cmem.core['o42] <= 36'o334000_000000;
		cmem.core['o43] <= 36'o000000_000000;
		cmem.core['o44] <= 36'o334000_000000;
		cmem.core['o45] <= 36'o000000_000000;
		cmem.core['o46] <= 36'o334000_000000;
		cmem.core['o47] <= 36'o000000_000000;
		cmem.core['o50] <= 36'o334000_000000;
		cmem.core['o51] <= 36'o000000_000000;
		cmem.core['o52] <= 36'o334000_000000;
		cmem.core['o53] <= 36'o000000_000000;

		cmem.core['o100] <= 36'o173040000000;
		cmem.core['o101] <= 36'o254200000000;

		fmem.ff[0] <= 36'o611042323251;
		fmem.ff[1] <= 36'o472340710317;
		mas <= 'o100;

		#200;
		sw_power <= 1;

		#200;
//		key_mem_stop <= 1;

		key_start <= 1;
		#1000;
		key_start <= 0;

/*
		#500;
		key_mem_stop <= 0;

		key_inst_stop <= 1;
		#1000;
		key_inst_stop <= 0;


		#1000;
		key_inst_cont <= 1;
		#500;
		key_inst_cont <= 0;
*/
	end

	initial begin
		#50000;
		$finish;
	end

endmodule
