`default_nettype none

module pdp6(
	input wire clk,
	input wire reset
);
	// keys
	reg key_start;
	reg key_read_in;
	reg key_mem_cont;
	reg key_inst_cont;
	reg key_mem_stop;
	reg key_inst_stop;
	reg key_exec;
	reg key_io_reset;
	reg key_dep;
	reg key_dep_nxt;
	reg key_ex;
	reg key_ex_nxt;

	// switches
	reg sw_addr_stop;
	reg sw_mem_disable;
	reg sw_repeat;
	reg sw_power;
	reg [0:35] datasw;
	reg [18:35] mas;

	// maintenance switches
	reg sw_rim_maint;
	reg sw_repeat_bypass;
	reg sw_art3_maint;
	reg sw_sct_maint;
	reg sw_split_cyc;

	// lights
	wire [0:17] ir;
	wire [0:35] mi;
	wire [0:35] ar;
	wire [0:35] mb;
	wire [0:35] mq;
	wire [18:35] pc;
	wire [18:35] ma;
	wire [0:8] fe;
	wire [0:8] sc;
	wire run;
	wire mc_stop;
	wire pi_active;
	wire [1:7] pih;
	wire [1:7] pir;
	wire [1:7] pio;
	wire [18:25] pr;
	wire [18:25] rlr;
	wire [18:25] rla;


	/* Mem bus */
	wire membus_wr_rs_p0;
	wire membus_rq_cyc_p0;
	wire membus_rd_rq_p0;
	wire membus_wr_rq_p0;
	wire [21:35] membus_ma_p0;
	wire [18:21] membus_sel_p0;
	wire membus_fmc_select_p0;
	wire membus_addr_ack_p0;
	wire membus_rd_rs_p0;
	wire [0:35] membus_mb_in_p0;

	/* Out of apr0 */
	wire [0:35] membus_mb_out_p0_p;

	/* Out of fmem0 */
	wire [0:35] membus_mb_out_p0_0;
	wire membus_addr_ack_p0_0;
	wire membus_rd_rs_p0_0;

	/* Out of mem0 */
	wire [0:35] membus_mb_out_p0_1;
	wire membus_addr_ack_p0_1;
	wire membus_rd_rs_p0_1;

	/* IO bus */
	wire iobus_iob_poweron;
	wire iobus_iob_reset;
	wire iobus_datao_clear;
	wire iobus_datao_set;
	wire iobus_cono_clear;
	wire iobus_cono_set;
	wire iobus_iob_fm_datai;
	wire iobus_iob_fm_status;
	wire [3:9]  iobus_ios;
	wire [0:35] iobus_iob_out;
	wire [1:7]  iobus_pi_req;
	wire [0:35] iobus_iob_in = iobus_iob_out;


	assign membus_mb_in_p0 = membus_mb_out_p0_p | membus_mb_out_p0_0 | membus_mb_out_p0_1;
	assign membus_addr_ack_p0 = membus_addr_ack_p0_0 | membus_addr_ack_p0_1;
	assign membus_rd_rs_p0 = membus_rd_rs_p0_0 | membus_rd_rs_p0_1;


	apr apr0(
		.clk(clk),
		.reset(reset),

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

		.ir(ir),
		.mi(mi),
		.ar(ar),
		.mb(mb),
		.mq(mq),
		.pc(pc),
		.ma(ma),
		.fe(fe),
		.sc(sc),
		.run(run),
		.mc_stop(mc_stop),
		.pi_active(pi_active),
		.pih(pih),
		.pir(pir),
		.pio(pio),
		.pr(pr),
		.rlr(rlr),
		.rla(rla),

		.membus_wr_rs(membus_wr_rs_p0),
		.membus_rq_cyc(membus_rq_cyc_p0),
		.membus_rd_rq(membus_rd_rq_p0),
		.membus_wr_rq(membus_wr_rq_p0),
		.membus_ma(membus_ma_p0),
		.membus_sel(membus_sel_p0),
		.membus_fmc_select(membus_fmc_select_p0),
		.membus_mb_out(membus_mb_out_p0_p),
		.membus_addr_ack(membus_addr_ack_p0),
		.membus_rd_rs(membus_rd_rs_p0),
		.membus_mb_in(membus_mb_in_p0),

		.iobus_iob_poweron(iobus_iob_poweron),
		.iobus_iob_reset(iobus_iob_reset),
		.iobus_datao_clear(iobus_datao_clear),
		.iobus_datao_set(iobus_datao_set),
		.iobus_cono_clear(iobus_cono_clear),
		.iobus_cono_set(iobus_cono_set),
		.iobus_iob_fm_datai(iobus_iob_fm_datai),
		.iobus_iob_fm_status(iobus_iob_fm_status),
		.iobus_ios(iobus_ios),
		.iobus_iob_out(iobus_iob_out),
		.iobus_pi_req(iobus_pi_req),
		.iobus_iob_in(iobus_iob_in)
	);

	reg mem0_sw_single_step;
	reg mem0_sw_restart;

	fast162 fmem0(
		.clk(clk),
		.reset(reset),
		.power(sw_power),
		.sw_single_step(mem0_sw_single_step),
		.sw_restart(mem0_sw_restart),

		.membus_wr_rs_p0(membus_wr_rs_p0),
		.membus_rq_cyc_p0(membus_rq_cyc_p0),
		.membus_rd_rq_p0(membus_rd_rq_p0),
		.membus_wr_rq_p0(membus_wr_rq_p0),
		.membus_ma_p0(membus_ma_p0),
		.membus_sel_p0(membus_sel_p0),
		.membus_fmc_select_p0(membus_fmc_select_p0),
		.membus_mb_in_p0(membus_mb_in_p0),
		.membus_addr_ack_p0(membus_addr_ack_p0_0),
		.membus_rd_rs_p0(membus_rd_rs_p0_0),
		.membus_mb_out_p0(membus_mb_out_p0_0),

		.membus_rq_cyc_p1(1'b0),
		.membus_sel_p1(4'b0),
		.membus_fmc_select_p1(1'b0),

		.membus_rq_cyc_p2(1'b0),
		.membus_sel_p2(4'b0),
		.membus_fmc_select_p2(1'b0),

		.membus_rq_cyc_p3(1'b0),
		.membus_sel_p3(4'b0),
		.membus_fmc_select_p3(1'b0)
	);

	core161c mem0(
		.clk(clk),
		.reset(reset),
		.power(sw_power),
		.sw_single_step(mem0_sw_single_step),
		.sw_restart(mem0_sw_restart),

		.membus_wr_rs_p0(membus_wr_rs_p0),
		.membus_rq_cyc_p0(membus_rq_cyc_p0),
		.membus_rd_rq_p0(membus_rd_rq_p0),
		.membus_wr_rq_p0(membus_wr_rq_p0),
		.membus_ma_p0(membus_ma_p0),
		.membus_sel_p0(membus_sel_p0),
		.membus_fmc_select_p0(membus_fmc_select_p0),
		.membus_mb_in_p0(membus_mb_in_p0),
		.membus_addr_ack_p0(membus_addr_ack_p0_1),
		.membus_rd_rs_p0(membus_rd_rs_p0_1),
		.membus_mb_out_p0(membus_mb_out_p0_1),

		.membus_rq_cyc_p1(1'b0),
		.membus_sel_p1(4'b0),
		.membus_fmc_select_p1(1'b0),

		.membus_rq_cyc_p2(1'b0),
		.membus_sel_p2(4'b0),
		.membus_fmc_select_p2(1'b0),

		.membus_rq_cyc_p3(1'b0),
		.membus_sel_p3(4'b0),
		.membus_fmc_select_p3(1'b0)
	);

endmodule
