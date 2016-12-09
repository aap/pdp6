`default_nettype none

`define synthesis

module fpdpga6(
	input wire clk,
	input wire [9:0] sw,
	input wire [4:0] key,
	output wire [7:0] ledg,
	output wire [9:0] ledr,

	input wire rx,
	output wire tx,

	output wire [17:0] sram_a,
	inout  wire [15:0] sram_d,
	output wire sram_ce,
	output wire sram_oe,
	output wire sram_we,
	output wire sram_lb,
	output wire sram_ub,

	input wire scl,
	inout wire sda
);

	// TODO: figure out what to do with this
	wire reset = ~key[0];

	wire ack;
	reg ack0;
	wire done = ~ack0 & ack;
	wire [6:0] dev;
	wire [7:0] in;
	wire dir;
	wire start, stop;
	// hardcoded devices:
	wire ok = panelok | coreok;
	wire [7:0] out = panelok ? panelout :
		coreok ? coreout : 8'b0;

	i2cslv slv(.clk(clk), .reset(reset),
		.scl(scl), .sda(sda),
		.dev(dev), .ok(ok),
		.ack(ack), .dir(dir),
		.in(in), .out(out),
		.start(start), .stop(stop));
	always @(posedge clk)
		ack0 <= ack;

	wire [17:0] core_sram_a;
	wire core_sram_ce;
	wire core_sram_oe;
	wire core_sram_we;
	wire core_sram_lb;
	wire core_sram_ub;
	wire [17:0] i2c_sram_a;
	wire i2c_sram_ce;
	wire i2c_sram_oe;
	wire i2c_sram_we;
	wire i2c_sram_lb;
	wire i2c_sram_ub;
	assign sram_a =  core_sram_a  | i2c_sram_a;
	assign sram_ce = core_sram_ce & i2c_sram_ce;
	assign sram_oe = core_sram_oe & i2c_sram_oe;
	assign sram_we = core_sram_we & i2c_sram_we;
	assign sram_lb = core_sram_lb & i2c_sram_lb;
	assign sram_ub = core_sram_ub & i2c_sram_ub;

	// i2cdev core
	wire coreok = dev == 7'h21;
	wire [7:0] coreout;
	i2c_core i2c_core0(.clk(clk), .reset(reset),
		.start(start), .stop(stop),
		.dir(dir),
		.ok(coreok),
		.done(done),
		.in(in),
		.out(coreout),

		.sram_a(i2c_sram_a),
		.sram_d(sram_d),
		.sram_ce(i2c_sram_ce),
		.sram_oe(i2c_sram_oe),
		.sram_we(i2c_sram_we),
		.sram_lb(i2c_sram_lb),
		.sram_ub(i2c_sram_ub));

	// i2cdev panel
	wire panelok = dev == 7'h26;
	reg [1:0] state;
	reg [7:0] addr;
	reg [7:0] panelout;

	always @(posedge clk) if(panelok) begin
		if(start)
			state <= 0;
		if(stop && dir == 1)
			addr <= addr - 8'b1;	// needed for consecutive reads
		if(dir == 0) begin	// WRITE
			if(done) begin
				case(state)
				0: begin	// got device address
					state <= 1;
				end
				1: begin	// got write address
					state <= 2;
					addr <= in;
				end
				2: begin
					case(addr)
					'h0: mas[18:25] <= in;
					'h1: mas[26:33] <= in;
					'h2: mas[34:35] <= in[7:6];

					'h3: datasw[0:7] <= in;
					'h4: datasw[8:15] <= in;
					'h5: datasw[16:23] <= in;
					'h6: datasw[24:31] <= in;
					'h7: datasw[32:35] <= in[7:4];

					'h8: { sw_repeat, sw_addr_stop,
						sw_power, sw_mem_disable } <= in[7:4];

					'h9: { sw_rim_maint, sw_repeat_bypass, sw_art3_maint,
						sw_sct_maint, sw_split_cyc } <= in[7:3];

					'hA: { key_start,
						key_inst_cont,
						key_inst_stop,
						key_io_reset,
						key_dep,
						key_ex,
						key_reader_off,
						key_punch_feed } <= in;

					'hB: { key_read_in,
						key_mem_cont,
						key_mem_stop,
						key_exec,
						key_dep_nxt,
						key_ex_nxt,
						key_reader_on,
						key_reader_feed } <= in;

					endcase
					addr <= addr + 8'b1;
				end
				endcase
			end
		end else if(dir == 1) begin
			if(done) begin
				if(sw_power)
					case(addr)
					8'h0: panelout <= ir[0:7];
					8'h1: panelout <= ir[8:15];
					8'h2: panelout <= { ir[16:17], 6'b0 };
					8'h3: panelout <= pc[18:25];
					8'h4: panelout <= pc[26:33];
					8'h5: panelout <= { pc[34:35], 6'b0 };
					8'h6: panelout <= mi[0:7];
					8'h7: panelout <= mi[8:15];
					8'h8: panelout <= mi[16:23];
					8'h9: panelout <= mi[24:31];
					8'hA: panelout <= { mi[32:35], 4'b0 };
					8'hB: panelout <= ma[18:25];
					8'hC: panelout <= ma[26:33];
					8'hD: panelout <= { ma[34:35], 6'b0 };
					8'hE: panelout <= { run, pih };
					8'hF: panelout <= { mc_stop, pir };
					8'h10: panelout <= { pi_active, pio };
					8'h11: panelout <= { sw_repeat, sw_addr_stop,
						sw_power, sw_mem_disable, 4'b0 };

					8'h12: panelout <= mb[0:7];
					8'h13: panelout <= mb[8:15];
					8'h14: panelout <= mb[16:23];
					8'h15: panelout <= mb[24:31];
					8'h16: panelout <= { mb[32:35], 4'b0 };
					8'h17: panelout <= ar[0:7];
					8'h18: panelout <= ar[8:15];
					8'h19: panelout <= ar[16:23];
					8'h1A: panelout <= ar[24:31];
					8'h1B: panelout <= { ar[32:35], 4'b0 };
					8'h1C: panelout <= mq[0:7];
					8'h1D: panelout <= mq[8:15];
					8'h1E: panelout <= mq[16:23];
					8'h1F: panelout <= mq[24:31];
					8'h20: panelout <= { mq[32:35], 4'b0 };

					8'h21: panelout <= ff[0];
					8'h22: panelout <= ff[1];
					8'h23: panelout <= ff[2];
					8'h24: panelout <= ff[3];
					8'h25: panelout <= ff[4];
					8'h26: panelout <= ff[5];
					8'h27: panelout <= ff[6];
					8'h28: panelout <= ff[7];
					8'h29: panelout <= ff[8];
					8'h2A: panelout <= ff[9];
					8'h2B: panelout <= ff[10];
					8'h2C: panelout <= ff[11];
					8'h2D: panelout <= ff[12];
					8'h2E: panelout <= ff[13];

					8'h2F: panelout <= pr;
					8'h30: panelout <= rlr;
					8'h31: panelout <= rla;

					8'h32: panelout <= { 1'b0, tty_ind };
					8'h33: panelout <= tti_ind;
					default: panelout <= 8'hFF;
					endcase
				else
					panelout <= 8'b0;
				addr <= addr + 8'b1;
			end
		end
	end


	// front panel
	reg [0:35] datasw;
	reg [18:35] mas;
	reg sw_repeat, sw_addr_stop;
	reg sw_power, sw_mem_disable;
	reg sw_rim_maint, sw_repeat_bypass;
	reg sw_art3_maint, sw_sct_maint, sw_split_cyc;
	reg key_start, key_read_in;
	reg key_inst_cont, key_mem_cont;
	reg key_inst_stop, key_mem_stop;
	reg key_io_reset, key_exec;
	reg key_dep, key_dep_nxt;
	reg key_ex, key_ex_nxt;
	reg key_reader_off, key_reader_on;
	reg key_punch_feed, key_reader_feed;

	wire [0:17] ir;
	wire [0:35] mi;
	wire [0:35] ar;
	wire [0:35] mb;
	wire [0:35] mq;
	wire [18:35] pc;
	wire [18:35] ma;
	wire run;
	wire mc_stop;
	wire pi_active;
	wire [1:7] pih;
	wire [1:7] pir;
	wire [1:7] pio;
	wire [18:25] pr;
	wire [18:25] rlr;
	wire [18:25] rla;
	wire [0:7] ff[13:0];




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
	wire [1:7]  iobus_pi_req = tty_pi_req;
	wire [0:35] iobus_iob_in = apr_iob_out | tty_iob_out;

	wire [0:35] apr_iob_out;

	assign membus_mb_in_p0 = membus_mb_out_p0_p | membus_mb_out_p0_0 | membus_mb_out_p0_1;
	assign membus_addr_ack_p0 = membus_addr_ack_p0_0 | membus_addr_ack_p0_1;
	assign membus_rd_rs_p0 = membus_rd_rs_p0_0 | membus_rd_rs_p0_1;

	apr apr0(
		.clk(clk),
		.reset(reset),
		.key_start(key_start),
		.key_read_in(key_read_in),
		.key_inst_cont(key_inst_cont),
		.key_mem_cont(key_mem_cont),
		.key_inst_stop(key_inst_stop),
		.key_mem_stop(key_mem_stop),
		.key_io_reset(key_io_reset),
		.key_exec(key_exec),
		.key_dep(key_dep),
		.key_dep_nxt(key_dep_nxt),
		.key_ex(key_ex),
		.key_ex_nxt(key_ex_nxt),

		.sw_repeat(sw_repeat),
		.sw_addr_stop(sw_addr_stop),
		.sw_power(sw_power),
		.sw_mem_disable(sw_mem_disable),
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
		.run(run),
		.mc_stop(mc_stop),
		.pi_active(pi_active),
		.pih(pih),
		.pir(pir),
		.pio(pio),
		.pr(pr),
		.rlr(rlr),
		.rla(rla),
		.ff0(ff[0]),
		.ff1(ff[1]),
		.ff2(ff[2]),
		.ff3(ff[3]),
		.ff4(ff[4]),
		.ff5(ff[5]),
		.ff6(ff[6]),
		.ff7(ff[7]),
		.ff8(ff[8]),
		.ff9(ff[9]),
		.ff10(ff[10]),
		.ff11(ff[11]),
		.ff12(ff[12]),
		.ff13(ff[13]),

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
		.iobus_iob_out(apr_iob_out),
		.iobus_pi_req(iobus_pi_req),
		.iobus_iob_in(iobus_iob_in)
	);

	reg mem0_sw_single_step = 0;
	reg mem0_sw_restart = 0;

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
		.membus_fmc_select_p3(1'b0),

		.sram_a(core_sram_a),
		.sram_d(sram_d),
		.sram_ce(core_sram_ce),
		.sram_oe(core_sram_oe),
		.sram_we(core_sram_we),
		.sram_lb(core_sram_lb),
		.sram_ub(core_sram_ub)
	);

	wire [7:0] tti_ind;
	wire [6:0] tty_ind;

	wire [1:7]  tty_pi_req;
	wire [0:35] tty_iob_out;

	tty tty0(
		.clk(clk),
		.rx(rx),
		.tx(tx),

		.tti_ind(tti_ind),
		.status_ind(tty_ind),

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
		.iobus_pi_req(tty_pi_req),
		.iobus_iob_out(tty_iob_out)
	);

	assign ledr = { run, 1'b0, tti_ind };
	assign ledg = { 1'b0, tty_ind };
/*
	assign ledr[7:0] = sw == 0 ? datasw[0:5] :
		sw == 1 ? datasw[6:11] :
		sw == 2 ? datasw[12:17] :
		sw == 3 ? datasw[18:23] :
		sw == 4 ? datasw[24:29] :
		sw == 5 ? datasw[30:35] :
		sw == 6 ? { 4'b0, sw_repeat, sw_addr_stop, sw_power, sw_mem_disable } :
		sw == 7 ? { key_start, key_inst_cont, key_inst_stop, key_io_reset, key_dep, key_ex, key_reader_off, key_punch_feed } :
		6'b0;
	assign ledg[7:0] = sw == 3 ? { 2'b0, mas[18:23] } :
		sw == 4 ? { 2'b0, mas[24:29] } :
		sw == 5 ? { 2'b0, mas[30:35] } :
		sw == 6 ? { 3'b0, sw_rim_maint, sw_repeat_bypass, sw_art3_maint, sw_sct_maint, sw_split_cyc } :
		sw == 7 ? { key_read_in, key_mem_cont, key_mem_stop, key_exec, key_dep_nxt, key_ex_nxt, key_reader_on, key_reader_feed } :
		6'b0;

	assign ledr[9:8] = 2'b0;

	assign ledr[5:0] = sw == 0 ? mb[0:5] :
		sw == 1 ? mb[6:11] :
		sw == 2 ? mb[12:17] :
		sw == 3 ? mb[18:23] :
		sw == 4 ? mb[24:29] :
		sw == 5 ? mb[30:35] :
		sw == 6 ? ar[0:5] :
		sw == 7 ? ar[6:11] :
		sw == 8 ? ar[12:17] :
		sw == 9 ? ar[18:23] :
		sw == 10 ? ar[24:29] :
		sw == 11 ? ar[30:35] :
		sw == 12 ? mq[0:5] :
		sw == 13 ? mq[6:11] :
		sw == 14 ? mq[12:17] :
		sw == 15 ? mq[18:23] :
		sw == 16 ? mq[24:29] :
		sw == 17 ? mq[30:35] : 0;
	assign ledr[9:6] = 0;
	assign ledg = 0;
*/

endmodule
