module core161c_x(
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
	parameter memsel_p0 = 4'b0;
	parameter memsel_p1 = 4'b0;
	parameter memsel_p2 = 4'b0;
	parameter memsel_p3 = 4'b0;


	reg [22:35] cma;
	reg cma_rd_rq;
	reg cma_wr_rq;

	reg [0:35] cmb;

	reg cmc_p0_act, cmc_p1_act, cmc_p2_act, cmc_p3_act;
	reg cmc_last_proc;
	reg cmc_rd;
	reg cmc_inhibit;	// not really used
	reg cmc_wr;
	reg cmc_await_rq = 0;
	reg cmc_proc_rs = 0;
	reg cmc_pse_sync;
	reg cmc_stop;

	wire cyc_rq_p0 = memsel_p0 == membus_sel_p0 &
		~membus_fmc_select_p0 & membus_rq_cyc_p0;
	wire cyc_rq_p1 = memsel_p1 == membus_sel_p1 &
		~membus_fmc_select_p1 & membus_rq_cyc_p1;
	wire cyc_rq_p2 = memsel_p2 == membus_sel_p2 &
		~membus_fmc_select_p2 & membus_rq_cyc_p2;
	wire cyc_rq_p3 = memsel_p3 == membus_sel_p3 &
		~membus_fmc_select_p3 & membus_rq_cyc_p3;
	wire cmpc_p0_rq = cyc_rq_p0 & cmc_await_rq;
	wire cmpc_p1_rq = cyc_rq_p1 & cmc_await_rq;
	wire cmpc_p2_rq = cyc_rq_p2 & cmc_await_rq;
	wire cmpc_p3_rq = cyc_rq_p3 & cmc_await_rq;

	wire wr_rs = cmc_p0_act ? membus_wr_rs_p0 :
	             cmc_p1_act ? membus_wr_rs_p1 :
	             cmc_p2_act ? membus_wr_rs_p2 :
	             cmc_p3_act ? membus_wr_rs_p3 : 1'b0;
	wire rd_rq = cmc_p0_act ? membus_rd_rq_p0 :
	             cmc_p1_act ? membus_rd_rq_p1 :
	             cmc_p2_act ? membus_rd_rq_p2 :
	             cmc_p3_act ? membus_rd_rq_p3 : 1'b0;
	wire wr_rq = cmc_p0_act ? membus_wr_rq_p0 :
	             cmc_p1_act ? membus_wr_rq_p1 :
	             cmc_p2_act ? membus_wr_rq_p2 :
	             cmc_p3_act ? membus_wr_rq_p3 : 1'b0;
	wire [21:35] ma = cmc_p0_act ? membus_ma_p0 :
	                  cmc_p1_act ? membus_ma_p1 :
	                  cmc_p2_act ? membus_ma_p2 :
	                  cmc_p3_act ? membus_ma_p3 : 15'b0;
	wire [0:35] mb_in = cmc_p0_act ? membus_mb_in_p0 :
	                    cmc_p1_act ? membus_mb_in_p1 :
	                    cmc_p2_act ? membus_mb_in_p2 :
	                    cmc_p3_act ? membus_mb_in_p3 : 36'b0;
	assign membus_addr_ack_p0 = cmc_addr_ack & cmc_p0_act;
	assign membus_rd_rs_p0 = cmc_rd_rs & cmc_p0_act;
	assign membus_mb_out_p0 = cmc_p0_act ? mb_out : 36'b0;
	assign membus_addr_ack_p1 = cmc_addr_ack & cmc_p1_act;
	assign membus_rd_rs_p1 = cmc_rd_rs & cmc_p1_act;
	assign membus_mb_out_p1 = cmc_p1_act ? mb_out : 36'b0;
	assign membus_addr_ack_p2 = cmc_addr_ack & cmc_p2_act;
	assign membus_rd_rs_p2 = cmc_rd_rs & cmc_p2_act;
	assign membus_mb_out_p2 = cmc_p2_act ? mb_out : 36'b0;
	assign membus_addr_ack_p3 = cmc_addr_ack & cmc_p3_act;
	assign membus_rd_rs_p3 = cmc_rd_rs & cmc_p3_act;
	assign membus_mb_out_p3 = cmc_p3_act ? mb_out : 36'b0;

	wire cmc_addr_ack;
	wire cmc_rd_rs;
	wire [0:35] mb_out = mb_pulse_out ? sa : 36'b0;
	wire cmpc_rs_strb;

	wire cmc_pwr_clr;
	wire cmc_pwr_start;
	wire cmc_key_restart;
	wire cmc_state_clr;
	wire cmc_cmb_clr;
	wire cmc_strb_sa;
	wire cmc_proc_rs_P;
	wire mb_pulse_out;
	wire cmc_wr_rs;
	wire cmc_t0;
	wire cmc_t1;
	wire cmc_t2;
	wire cmc_t4;
	wire cmc_t5;
	wire cmc_t6;
	wire cmc_t7;
	wire cmc_t8;
	wire cmc_t9;
	wire cmc_t9a;
	wire cmc_t10;
	wire cmc_t11;
	wire cmc_t12;

	// power-on timing is totally wrong

	pg cmc_pg0(.clk(clk), .reset(reset), .in(power), .p(cmc_pwr_clr));
	pg cmc_pg1(.clk(clk), .reset(reset),
		.in(sw_restart & cmc_stop), .p(cmc_key_restart));
	pg cmc_pg2(.clk(clk), .reset(reset),
		.in(cmpc_p0_rq | cmpc_p1_rq | cmpc_p2_rq | cmpc_p3_rq),
		.p(cmc_t0));
	pg cmc_pg4(.clk(clk), .reset(reset), .in(wr_rs), .p(cmpc_rs_strb));
	pg cmc_pg5(.clk(clk), .reset(reset), .in(cmc_proc_rs), .p(cmc_proc_rs_P));
	pg cmc_pg6(.clk(clk), .reset(reset), .in(cmc_pse_sync & cmc_proc_rs), .p(cmc_wr_rs));


	pa cmc_pa0(.clk(clk), .reset(reset), .in(cmc_pwr_clr | cmc_t9a_D), .p(cmc_t12));
	pa cmc_pa1(.clk(clk), .reset(reset), .in(cmc_pwr_clr_D), .p(cmc_pwr_start));
	pa cmc_pa2(.clk(clk), .reset(reset),
		.in(cmc_t9a & ~cmc_stop |
		    cmc_pwr_start |
		    cmc_key_restart),
		.p(cmc_t10));
	pa cmc_pa3(.clk(clk), .reset(reset), .in(cmc_t10_D), .p(cmc_t11));
	pa cmc_pa4(.clk(clk), .reset(reset),
		.in(cmc_t10 |
		    cmc_strb_sa_D1 & ~cma_wr_rq |
		    cmc_proc_rs_P),
		.p(cmc_state_clr));
	pa cmc_pa5(.clk(clk), .reset(reset),
		.in(cmc_t0 |
		    cmc_strb_sa_D2 & cma_wr_rq),
		.p(cmc_cmb_clr));
	pa cmc_pa6(.clk(clk), .reset(reset), .in(cmc_t0_D), .p(cmc_t1));
	pa cmc_pa7(.clk(clk), .reset(reset), .in(cmc_t1_D), .p(cmc_t2));
	pa cmc_pa8(.clk(clk), .reset(reset), .in(cmc_t2_D0), .p(cmc_t4));
	pa cmc_pa9(.clk(clk), .reset(reset), .in(cmc_t4_D), .p(cmc_t5));
	pa cmc_pa10(.clk(clk), .reset(reset),
		.in(cmc_t5 & ~cma_wr_rq | cmc_wr_rs),
		.p(cmc_t6));
	pa cmc_pa11(.clk(clk), .reset(reset), .in(cmc_t6_D), .p(cmc_t7));
	pa cmc_pa12(.clk(clk), .reset(reset), .in(cmc_t7_D), .p(cmc_t8));
	pa cmc_pa13(.clk(clk), .reset(reset), .in(cmc_t8_D), .p(cmc_t9));
	pa cmc_pa14(.clk(clk), .reset(reset), .in(cmc_t9_D), .p(cmc_t9a));
	pa cmc_pa15(.clk(clk), .reset(reset),
		.in(cmc_t2_D1 & cma_rd_rq),
		.p(cmc_strb_sa));

	// not on schematics
	bd cmc_bd0(.clk(clk), .reset(reset), .in(cmc_t1), .p(cmc_addr_ack));
	bd cmc_bd1(.clk(clk), .reset(reset), .in(cmc_strb_sa_D0), .p(cmc_rd_rs));
	bd cmc_bd2(.clk(clk), .reset(reset), .in(cmc_strb_sa), .p(mb_pulse_out));

	wire cmc_pwr_clr_D;
	wire cmc_t0_D, cmc_t1_D, cmc_t2_D0, cmc_t2_D1, cmc_t4_D;
	wire cmc_t6_D, cmc_t7_D, cmc_t8_D, cmc_t9_D, cmc_t9a_D, cmc_t10_D;
	wire cmc_strb_sa_D0, cmc_strb_sa_D1, cmc_strb_sa_D2;
	dly100ns cmc_dly0(.clk(clk), .reset(reset), .in(cmc_pwr_clr), .p(cmc_pwr_clr_D));
	dly100ns cmc_dly1(.clk(clk), .reset(reset), .in(cmc_t10), .p(cmc_t10_D));
	dly200ns cmc_dly2(.clk(clk), .reset(reset), .in(cmc_t0), .p(cmc_t0_D));
	dly1us   cmc_dly3(.clk(clk), .reset(reset), .in(cmc_t1), .p(cmc_t1_D));
	dly1us   cmc_dly4(.clk(clk), .reset(reset), .in(cmc_t2), .p(cmc_t2_D0));
	dly200ns cmc_dly5(.clk(clk), .reset(reset), .in(cmc_t4), .p(cmc_t4_D));
	dly200ns cmc_dly6(.clk(clk), .reset(reset), .in(cmc_t6), .p(cmc_t6_D));
	dly200ns cmc_dly7(.clk(clk), .reset(reset), .in(cmc_t7), .p(cmc_t7_D));
	dly1us   cmc_dly8(.clk(clk), .reset(reset), .in(cmc_t8), .p(cmc_t8_D));
	dly400ns cmc_dly9(.clk(clk), .reset(reset), .in(cmc_t9), .p(cmc_t9_D));
	dly200ns cmc_dly10(.clk(clk), .reset(reset), .in(cmc_t9a), .p(cmc_t9a_D));
	dly800ns cmc_dly11(.clk(clk), .reset(reset), .in(cmc_t2), .p(cmc_t2_D1));
	dly100ns cmc_dly12(.clk(clk), .reset(reset),
		.in(cmc_strb_sa), .p(cmc_strb_sa_D0));
	dly200ns cmc_dly13(.clk(clk), .reset(reset),
		.in(cmc_strb_sa), .p(cmc_strb_sa_D1));
	dly250ns cmc_dly14(.clk(clk), .reset(reset),
		.in(cmc_strb_sa), .p(cmc_strb_sa_D2));

	reg [0:35] sa;	// "sense amplifiers"
	wire [13:0] core_addr = cma[22:35];

	assign m_address = { 4'b0, core_addr };
	assign m_writedata = cmb;

	always @(posedge clk or posedge reset) begin
		if(reset) begin
			cmc_await_rq <= 0;
			cmc_last_proc <= 0;
			cmc_proc_rs <= 0;

			m_read <= 0;
			m_write <= 0;
			sa <= 0;
		end else begin
			if(m_write & ~m_waitrequest) begin
				m_write <= 0;
			end
			if(m_read & ~m_waitrequest) begin
				m_read <= 0;
				sa <= m_readdata;
			end

			if(cmc_state_clr) begin
				cmc_p0_act <= 0;
				cmc_p1_act <= 0;
				cmc_p2_act <= 0;
				cmc_p3_act <= 0;
			end
			cmb <= cmb | mb_in;
			if(cmc_cmb_clr)
				cmb <= 0;
			if(cmc_strb_sa)
				cmb <= cmb | sa;
			if(cmpc_rs_strb)
				cmc_proc_rs <= 1;

			if(cmc_t0) begin
				cmc_await_rq <= 0;
				cmc_proc_rs <= 0;
				cmc_pse_sync <= 0;
				cmc_stop <= 0;
				cma <= 0;
				cma_rd_rq <= 0;
				cma_wr_rq <= 0;

				// this happens between t0 and t1 */
				if(cmpc_p0_rq)
					cmc_p0_act <= 1;
				else if(cmpc_p1_rq)
					cmc_p1_act <= 1;
				else if(cmpc_p2_rq) begin
					if(~cmpc_p3_rq | cmc_last_proc)
						cmc_p2_act <= 1;
				end else if(cmpc_p3_rq) begin
					if(~cmpc_p2_rq | ~cmc_last_proc)
						cmc_p3_act <= 1;
				end
			end
			if(cmc_t1) begin	// this seems to be missing from the schematics
				cma <= cma | ma[22:35];
				if(rd_rq)
					cma_rd_rq <= 1;
				if(wr_rq)
					cma_wr_rq <= 1;
			end
			if(cmc_t2) begin
				cmc_rd <= 1;
				if(cmc_p2_act)
					cmc_last_proc <= 0;
				if(cmc_p3_act)
					cmc_last_proc <= 1;

				/* start read */
				m_read <= 1;
			end
			// hack: zero memory at cmc_t4
			if(cmc_t5) begin
				cmc_rd <= 0;
				cmc_pse_sync <= 1;
			end
			if(cmc_t7) begin
				cmc_inhibit <= 1;
				if(sw_single_step)
					cmc_stop <= 1;
			end
			if(cmc_t8) begin
				cmc_wr <= 1;
				/* again a hack. core is written some time after t8. */
				m_write <= 1;
			end
			if(cmc_t11)
				cmc_await_rq <= 1;
			if(cmc_t12) begin
				cmc_rd <= 0;
				cmc_inhibit <= 0;
				cmc_wr <= 0;
			end
		end
	end

endmodule
