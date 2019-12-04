module core32k(
	input wire clk,
	input wire reset,
	input wire power,
	input wire sw_single_step,
	input wire sw_restart,

	input  wire membus_wr_rs_p0,
	input  wire membus_rq_cyc_p0,
	input  wire membus_rd_rq_p0,
	input  wire membus_wr_rq_p0,
	input  wire [21:35] membus_ma_p0,
	input  wire [18:21] membus_sel_p0,
	input  wire membus_fmc_select_p0,
	input  wire [0:35] membus_mb_in_p0,
	output wire membus_addr_ack_p0,
	output wire membus_rd_rs_p0,
	output wire [0:35] membus_mb_out_p0,

	input  wire membus_wr_rs_p1,
	input  wire membus_rq_cyc_p1,
	input  wire membus_rd_rq_p1,
	input  wire membus_wr_rq_p1,
	input  wire [21:35] membus_ma_p1,
	input  wire [18:21] membus_sel_p1,
	input  wire membus_fmc_select_p1,
	input  wire [0:35] membus_mb_in_p1,
	output wire membus_addr_ack_p1,
	output wire membus_rd_rs_p1,
	output wire [0:35] membus_mb_out_p1,

	input  wire membus_wr_rs_p2,
	input  wire membus_rq_cyc_p2,
	input  wire membus_rd_rq_p2,
	input  wire membus_wr_rq_p2,
	input  wire [21:35] membus_ma_p2,
	input  wire [18:21] membus_sel_p2,
	input  wire membus_fmc_select_p2,
	input  wire [0:35] membus_mb_in_p2,
	output wire membus_addr_ack_p2,
	output wire membus_rd_rs_p2,
	output wire [0:35] membus_mb_out_p2,

	input  wire membus_wr_rs_p3,
	input  wire membus_rq_cyc_p3,
	input  wire membus_rd_rq_p3,
	input  wire membus_wr_rq_p3,
	input  wire [21:35] membus_ma_p3,
	input  wire [18:21] membus_sel_p3,
	input  wire membus_fmc_select_p3,
	input  wire [0:35] membus_mb_in_p3,
	output wire membus_addr_ack_p3,
	output wire membus_rd_rs_p3,
	output wire [0:35] membus_mb_out_p3,

	// 36 bit Avalon Master
	output wire [17:0] m_address,
	output reg m_write,
	output reg m_read,
	output wire [35:0] m_writedata,
	input wire [35:0] m_readdata,
	input wire m_waitrequest
);
	/* Jumpers */
	parameter [3:0] memsel_p0 = 4'b0;
	parameter [3:0] memsel_p1 = 4'b0;
	parameter [3:0] memsel_p2 = 4'b0;
	parameter [3:0] memsel_p3 = 4'b0;

	// TODO: SP
	wire cmc_sp = 0;

	reg [21:35] cma;
	reg cma_rd_rq, cma_wr_rq;
	reg [0:35] cmb;

	// TODO: interleave

	wire cmpc_p0_rq = (membus_sel_p0[18:20] == memsel_p0[3:1]) &
		~membus_fmc_select_p0 &
		membus_rq_cyc_p0 &
		cmc_await_rq;
	wire cmpc_p1_rq = (membus_sel_p1[18:20] == memsel_p1[3:1]) &
		~membus_fmc_select_p1 &
		membus_rq_cyc_p1 &
		cmc_await_rq;
	wire cmpc_p2_rq = (membus_sel_p2[18:20] == memsel_p2[3:1]) &
		~membus_fmc_select_p2 &
		membus_rq_cyc_p2 &
		cmc_await_rq;
	wire cmpc_p3_rq = (membus_sel_p3[18:20] == memsel_p3[3:1]) &
		~membus_fmc_select_p3 &
		membus_rq_cyc_p3 &
		cmc_await_rq;

	wire [21:35] ma_in =
		{15{cmc_p0_sel}}&membus_ma_p0 |
		{15{cmc_p1_sel}}&membus_ma_p1 |
		{15{cmc_p2_sel}}&membus_ma_p2 |
		{15{cmc_p3_sel}}&membus_ma_p3;
	wire rd_rq_in =
		cmc_p0_sel&membus_rd_rq_p0 |
		cmc_p1_sel&membus_rd_rq_p1 |
		cmc_p2_sel&membus_rd_rq_p2 |
		cmc_p3_sel&membus_rd_rq_p3;
	wire wr_rq_in =
		cmc_p0_sel&membus_wr_rq_p0 |
		cmc_p1_sel&membus_wr_rq_p1 |
		cmc_p2_sel&membus_wr_rq_p2 |
		cmc_p3_sel&membus_wr_rq_p3;
	wire [0:35] mb_in =
		{36{cmc_p0_sel}}&membus_mb_in_p0 |
		{36{cmc_p1_sel}}&membus_mb_in_p1 |
		{36{cmc_p2_sel}}&membus_mb_in_p2 |
		{36{cmc_p3_sel}}&membus_mb_in_p3;
	pa cmpc_pa0(clk, reset, cmc_t1b&cmc_p0_sel, membus_addr_ack_p0);
	pa cmpc_pa1(clk, reset, cmc_t1b&cmc_p1_sel, membus_addr_ack_p1);
	pa cmpc_pa2(clk, reset, cmc_t1b&cmc_p2_sel, membus_addr_ack_p2);
	pa cmpc_pa3(clk, reset, cmc_t1b&cmc_p3_sel, membus_addr_ack_p3);
	assign membus_rd_rs_p0 = cmc_rd_rs&cmc_p0_sel;
	assign membus_rd_rs_p1 = cmc_rd_rs&cmc_p1_sel;
	assign membus_rd_rs_p2 = cmc_rd_rs&cmc_p2_sel;
	assign membus_rd_rs_p3 = cmc_rd_rs&cmc_p3_sel;
	assign membus_mb_out_p0 = sa & {36{stb_pulse & cmc_p0_sel}};
	assign membus_mb_out_p1 = sa & {36{stb_pulse & cmc_p1_sel}};
	assign membus_mb_out_p2 = sa & {36{stb_pulse & cmc_p2_sel}};
	assign membus_mb_out_p3 = sa & {36{stb_pulse & cmc_p3_sel}};
	wire cmpc_rs_set = membus_wr_rs_p0 & cmc_p0_sel |
		membus_wr_rs_p1 & cmc_p1_sel |
		membus_wr_rs_p2 & cmc_p2_sel |
		membus_wr_rs_p3 & cmc_p3_sel;


	// TODO: this is all wrong
	wire pwr_t1, pwr_t2, pwr_t3;
	wire cmc_await_rq_reset, cmc_pwr_clr, cmc_pwr_start;
	pg pg0(clk, reset, power, pwr_t1);
	// 200ms
	ldly1us dly0(clk, reset, pwr_t1, pwr_t2, cmc_await_rq_reset);
	// 100μs
	ldly1us dly1(clk, reset, pwr_t2, pwr_t3, cmc_pwr_clr);
	pa pa0(clk, reset, pwr_t3, cmc_pwr_start);


	// core control, we don't really have a use for it
	reg cmc_read, cmc_write, cmc_inh;

	reg cmc_rq_sync, cmc_cyc_done;
	reg cmc_await_rq, cmc_pse_sync, cmc_proc_rs, cmc_stop;

	reg cmc_p0_act, cmc_p1_act, cmc_p2_act, cmc_p3_act;
	reg cmc_last_proc;

	// TODO: SP
	wire cmc_p0_sel = cmc_p0_act;
	wire cmc_p1_sel = cmc_p1_act;
	wire cmc_p2_sel = cmc_p2_act;
	wire cmc_p3_sel = cmc_p3_act;

	wire cmc_t0, cmc_t1b, cmc_t1a, cmc_t2, cmc_t3;
	wire cmc_t4, cmc_t5, cmc_t6, cmc_t6p;
	wire cmc_t0_D, cmc_t2_D1;
	wire cmc_t3_D1, cmc_t3_D2;

	wire cmc_restart;
	wire cmc_start;
	wire cmc_state_clr;
	wire cmc_pn_act = cmc_p0_act | cmc_p1_act | cmc_p2_act | cmc_p3_act;
	wire cmc_aw_rq_set = cmc_t0_D & ~cmc_pn_act;
	wire cmc_rq_sync_set = cmc_t0_D & ~cmc_sp & cmc_pn_act;
	wire cmc_jam_cma = cmc_t1b;
	wire cmc_cmb_clr;
	wire cmc_read_off;
	wire cmc_pse_sync_set;
	wire strobe_sense;
	wire cmc_rd_rs;
	wire cmpc_rs_set_D;
	wire cmc_proc_rs_pulse;

	pa pa1(clk, reset,
		(cmpc_p0_rq | cmpc_p1_rq | cmpc_p2_rq | cmpc_p3_rq),
		cmc_t0);
	pa pa2(clk, reset, cmc_restart | cmc_pwr_start, cmc_start);	
	pa pa3(clk, reset, cmc_start | cmc_t3_D1, cmc_t5);
	pa pa4(clk, reset, cmc_start | cmc_t3_D2, cmc_t6p);
	pa pa5(clk, reset,
		cmc_start | cmc_t3 & ~cma_wr_rq | cmc_proc_rs_pulse,
		cmc_state_clr);
	pa pa6(clk, reset, cmc_rq_sync&cmc_cyc_done, cmc_t1b);
	pa pa7(clk, reset, cmc_t1b, cmc_t1a);
	pa pa9(clk, reset,
		cmc_t1b | cmc_pse_sync_set&cma_rd_rq&cma_wr_rq,
		cmc_cmb_clr);
	pa pa11(clk, reset, cmc_t2_D1&cma_rd_rq, strobe_sense);
	pa pa12(clk, reset, stb_pulse, cmc_rd_rs);

	pa pa13(clk, reset, cmc_pse_sync&(cmc_proc_rs | ~cma_wr_rq), cmc_t3);
	pa pa14(clk, reset, cmc_proc_rs, cmc_proc_rs_pulse);
	// probably wrong
	pa pa15(clk, reset, sw_restart, cmc_restart);


	dly250ns dly2(clk, reset, cmc_t6p, cmc_t6);
	dly100ns dly3(clk, reset, cmc_t0, cmc_t0_D);
	dly250ns dly4(clk, reset, cmc_t1a, cmc_t2);
	dly200ns dly5(clk, reset, cmc_t2, cmc_read_off);
	dly100ns dly6(clk, reset, rd_pulse, cmc_pse_sync_set);
	// Variable 35-100ns
	dly70ns dly7(clk, reset, cmc_t2, cmc_t2_D1);
	dly50ns dly9(clk, reset, cmc_t3, cmc_t4);
	dly500ns dly10(clk, reset, cmc_t4, cmc_t3_D1);
	dly200ns dly11(clk, reset, cmc_t3_D1, cmc_t3_D2);
	dly50ns dly12(clk, reset, cmpc_rs_set, cmpc_rs_set_D);


	reg [0:35] sa;	// "sense amplifiers"
	wire [14:0] core_addr = cma[21:35];

	assign m_address = { 3'b0, core_addr };
	assign m_writedata = cmb;


	reg stb_sync, stb_done;
	wire stb_pulse;
	pa pa100(clk, reset, stb_sync&stb_done, stb_pulse);

	reg rd_sync, rd_done;
	wire rd_pulse;
	pa pa101(clk, reset, rd_sync&rd_done, rd_pulse);

	reg wr_sync, wr_done;
	wire wr_pulse;
	pa pa102(clk, reset, wr_sync&wr_done, wr_pulse);


	always @(posedge clk or posedge reset) begin
		if(reset) begin
			m_read <= 0;
			m_write <= 0;
			sa <= 0;

			stb_sync <= 0;
			stb_done <= 0;
			rd_sync <= 0;
			rd_done <= 0;
			wr_sync <= 0;
			wr_done <= 0;

			// value doesn't matter
			cmc_last_proc <= 0;
			// these should probably be reset but aren't
			cmc_proc_rs <= 0;
			cmc_pse_sync <= 0;
		end else begin
			if(m_write & ~m_waitrequest) begin
				m_write <= 0;
				wr_done <= 1;
			end
			if(m_read & ~m_waitrequest) begin
				m_read <= 0;
				sa <= m_readdata;
				stb_done <= 1;
			end

			if(cmc_start)
				wr_done <= 1;

			if(cmc_state_clr) begin
				cmc_p0_act <= 0;
				cmc_p1_act <= 0;
				cmc_p2_act <= 0;
				cmc_p3_act <= 0;
			end
			if(cmpc_p0_rq | cmpc_p1_rq | cmpc_p2_rq | cmpc_p3_rq) begin
				if(cmpc_p0_rq) begin
					cmc_p0_act <= 1;
					cmc_p1_act <= 0;
					cmc_p2_act <= 0;
					cmc_p3_act <= 0;
				end else if(cmpc_p1_rq) begin
					cmc_p0_act <= 0;
					cmc_p1_act <= 1;
					cmc_p2_act <= 0;
					cmc_p3_act <= 0;
				end else if(cmpc_p2_rq & cmpc_p3_rq) begin
					cmc_p0_act <= 0;
					cmc_p1_act <= 0;
					cmc_p2_act <= cmc_last_proc;
					cmc_p3_act <= ~cmc_last_proc;
					cmc_last_proc <= ~cmc_last_proc;
				end else if(cmpc_p2_rq) begin
					cmc_p0_act <= 0;
					cmc_p1_act <= 0;
					cmc_p2_act <= 1;
					cmc_p3_act <= 0;
				end else if(cmpc_p3_rq) begin
					cmc_p0_act <= 0;
					cmc_p1_act <= 0;
					cmc_p2_act <= 0;
					cmc_p3_act <= 1;
				end
			end
			if(cmc_t2) begin
				if(cmc_p2_act)
					cmc_last_proc <= 0;	
				if(cmc_p3_act)
					cmc_last_proc <= 1;	
			end

			if(cmc_t0 | cmc_await_rq_reset)
				cmc_await_rq <= 0;
			if(cmc_t5 | cmc_aw_rq_set)
				cmc_await_rq <= 1;

			if(cmc_start | cmc_pwr_clr)
				cmc_rq_sync <= 0;
			if(cmc_rq_sync_set | cmc_t0 & cmc_sp)
				cmc_rq_sync <= 1;

			if(cmc_pwr_clr)
				cmc_cyc_done <= 0;
			if(wr_pulse & ~cmc_stop)
				cmc_cyc_done <= 1;
			if(cmc_t6)
				wr_sync <= 1;

			if(cmc_t1b) begin
				cmc_pse_sync <= 0;
				cmc_proc_rs <= 0;
				cmc_stop <= 0;
			end

			// actually through another PA
			if(cmc_pse_sync_set)
				cmc_pse_sync <= 1;

			if(cmpc_rs_set_D)
				cmc_proc_rs <= 1;

			if(cmc_start)
				cmc_stop <= 0;
			if(cmc_t2 & sw_single_step)
				cmc_stop <= 1;

			if(cmc_t2) begin
				cmc_rq_sync <= 0;
				cmc_cyc_done <= 0;
			end


			if(cmc_jam_cma) begin
				cma <= ma_in;
				cma_rd_rq <= rd_rq_in;
				cma_wr_rq <= wr_rq_in;
			end

			cmb <= cmb | mb_in;
			if(cmc_cmb_clr)
				cmb <= 0;
			if(strobe_sense)
				stb_sync <= 1;
			if(stb_pulse) begin
				stb_sync <= 0;
				stb_done <= 0;
				rd_done <= 1;
				cmb <= cmb | sa;
			end

			/* Core */
			if(cmc_pwr_clr | cmc_t5) begin
				cmc_read <= 0;
				cmc_write <= 0;
				cmc_inh <= 0;
			end
			if(cmc_t1a) begin
				cmc_read <= 1;
				m_read <= cma_rd_rq;
				stb_done <= 0;
				rd_done <= ~cma_rd_rq;
				cmc_write <= 0;
			end
			if(cmc_read_off) begin
				cmc_read <= 0;
				rd_sync <= 1;
			end
			if(rd_pulse) begin
				rd_sync <= 0;
				rd_done <= 0;
			end
			if(cmc_t3) begin
				cmc_inh <= 1;
				m_write <= cma_wr_rq;
				wr_done <= ~cma_wr_rq;
			end
			if(cmc_t4) begin
				cmc_write <= 1;
				cmc_read <= 0;
			end
			if(wr_pulse) begin
				wr_sync <= 0;
				wr_done <= 0;
			end
		end
	end
endmodule
