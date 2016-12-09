// UNUSED

module apr(
	input wire clk,
	input wire reset,
	input wire key_start,
	input wire key_read_in,
	input wire key_inst_cont,
	input wire key_mem_cont,
	input wire key_inst_stop,
	input wire key_mem_stop,
	input wire key_io_reset,
	input wire key_exec,
	input wire key_dep,
	input wire key_dep_nxt,
	input wire key_ex,
	input wire key_ex_nxt,

	input wire sw_repeat,
	input wire sw_addr_stop,
	input wire sw_power,
	input wire sw_mem_disable,
	input wire [0:35] datasw,
	input wire [18:35] mas,

	input wire sw_rim_maint,
	input wire sw_repeat_bypass,
	input wire sw_art3_maint,
	input wire sw_sct_maint,
	input wire sw_split_cyc,

	output reg [0:17] ir,
	output reg [0:35] mi,
	output reg [0:35] ar,
	output reg [0:35] mb,
	output reg [0:35] mq,
	output reg [18:35] pc,
	output reg [18:35] ma,
	output reg run,
	output reg mc_stop,
	output reg pi_active,
	output reg [1:7] pih,
	output reg [1:7] pir,
	output reg [1:7] pio,
	output reg [18:25] pr,
	output reg [18:25] rlr,
	output reg [18:25] rla,
	output wire [0:7] ff0,
	output wire [0:7] ff1,
	output wire [0:7] ff2,
	output wire [0:7] ff3,
	output wire [0:7] ff4,
	output wire [0:7] ff5,
	output wire [0:7] ff6,
	output wire [0:7] ff7,
	output wire [0:7] ff8,
	output wire [0:7] ff9,
	output wire [0:7] ff10,
	output wire [0:7] ff11,
	output wire [0:7] ff12,
	output wire [0:7] ff13,

	// membus
	output wire membus_wr_rs,
	output wire membus_rq_cyc,
	output wire membus_rd_rq,
	output wire membus_wr_rq,
	output wire [21:35] membus_ma,
	output wire [18:21] membus_sel,
	output wire membus_fmc_select,
	output wire [0:35] membus_mb_out,
	input  wire membus_addr_ack,
	input  wire membus_rd_rs,
	input  wire [0:35] membus_mb_in,

	// IO bus
	output wire iobus_iob_poweron,
	output wire iobus_iob_reset,
	output wire iobus_datao_clear,
	output wire iobus_datao_set,
	output wire iobus_cono_clear,
	output wire iobus_cono_set,
	output wire iobus_iob_fm_datai,
	output wire iobus_iob_fm_status,
	output wire [3:9]  iobus_ios,
	output wire [0:35] iobus_iob_out,
	input  wire [1:7]  iobus_pi_req,
	input  wire [0:35] iobus_iob_in
);

	wire key_any = key_start | key_read_in | key_inst_cont | key_mem_cont | key_inst_stop | key_mem_stop | key_ex | key_ex_nxt | key_dep | key_dep_nxt;
	wire key_pulse;
	pg pg0(.clk(clk), .reset(reset),
		.in(key_any),
		.p(key_pulse));

	assign ff1 = 2;
	assign ff2 = 3;
	assign ff3 = 4;
	assign ff4 = 5;
	assign ff5 = 6;
	assign ff6 = 7;
	assign ff7 = 8;
	assign ff8 = 9;
	assign ff9 = 10;
	assign ff10 = 11;
	assign ff11 = 12;
	assign ff12 = 13;
	assign ff13 = 14;

	//initial begin
	//	mb <= 36'o111111111111;
	//	ar <= 36'o222222222222;
	//	mq <= 36'o333333333333;
	//	mi <= 36'o444444444444;
	//	ir <= 18'o555555;
	//	ma <= 18'o666666;
	//	pc <= 18'o777777;
	//end


	wire kt0, kt1, kt2;
	wire key_rd, key_wr;
	pa key_pa0(.clk(clk), .reset(reset),
		.in(key_pulse),
		.p(kt0));
	pa key_pa1(.clk(clk), .reset(reset),
		.in(mc_rs_t1 & key_rdwr),
		.p(kt2));

	dly200ns dly0(.clk(clk), .reset(reset),
		.in(kt0),
		.p(kt1));
	dly200ns dly1(.clk(clk), .reset(reset),
		.in(kt1 & (key_ex | key_ex_nxt)),
		.p(key_rd));
	dly200ns dly2(.clk(clk), .reset(reset),
		.in(kt1 & (key_dep | key_dep_nxt)),
		.p(key_wr));

	always @(posedge clk) begin
		if(kt0 & (key_ex | key_dep))
			ma <= 0;
		if(kt0 & (key_ex_nxt | key_dep_nxt))
			ma <= ma + 18'b1;
		if(kt0 & (key_dep | key_dep_nxt))
			ar <= 0;

		if(kt1 & (key_ex | key_dep))
			ma <= ma | mas;
		if(kt1 & (key_dep | key_dep_nxt))
			ar <= ar | datasw;

		if(key_rd | key_wr)
			key_rdwr <= 1;
		if(kt2)
			key_rdwr <= 0;
	end


	assign membus_rq_cyc = mc_rq & (mc_rd | mc_wr);
	assign membus_rd_rq = mc_rd;
	assign membus_wr_rq = mc_wr;
	assign membus_ma = ma[21:35];
	assign membus_sel = 0;
	assign membus_fmc_select = sw_rim_maint;

	assign membus_mb_out = mc_membus_fm_mb1 ? mb : 0;

	wire mai_addr_ack, mai_rd_rs;
	wire mb_pulse;
	pg mc_pg0(.clk(clk), .reset(reset),
		.in(membus_addr_ack), .p(mai_addr_ack));
	pg mc_pg1(.clk(clk), .reset(reset),
		.in(membus_rd_rs), .p(mai_rd_rs));
	pg mc_pg2(.clk(clk), .reset(reset),
		.in(| membus_mb_in),
		.p(mb_pulse));

	wire mc_wr_rs, mc_membus_fm_mb1;
	bd  mc_bd0(.clk(clk), .reset(reset), .in(mc_wr_rs), .p(membus_wr_rs));
	bd2 mb_bd1(.clk(clk), .reset(reset), .in(mc_wr_rs), .p(mc_membus_fm_mb1));


	reg key_rdwr = 0;
	reg mc_rd = 0, mc_wr = 0, mc_rq = 0;

	wire mc_addr_ack;
	wire mc_rd_rq_pulse, mc_wr_rq_pulse, mc_rq_pulse;
	wire mc_rs_t0, mc_rs_t1;
	pa mc_pa1(.clk(clk), .reset(reset),
		.in(key_rd),
		.p(mc_rd_rq_pulse));
	pa mc_pa2(.clk(clk), .reset(reset),
		.in(key_wr),
		.p(mc_wr_rq_pulse));
	pa mc_pa3(.clk(clk), .reset(reset),
		.in(mc_rd_rq_pulse | mc_wr_rq_pulse),
		.p(mc_rq_pulse));
	pa mc_pa4(.clk(clk), .reset(reset),
		.in(mai_rd_rs | mc_wr_rs),
		.p(mc_rs_t0));
	pa mc_pa5(.clk(clk), .reset(reset),
		.in(mc_rs_t0),
		.p(mc_rs_t1));
	pa mc_pa6(.clk(clk), .reset(reset),
		.in(mai_addr_ack),
		.p(mc_addr_ack));
	pa mc_pa7(.clk(clk), .reset(reset),
		.in(mc_addr_ack & ~mc_rd & mc_wr),
		.p(mc_wr_rs));


	reg f0 = 0;
	reg f1 = 0;
	always @(posedge clk) begin
		if(mc_rd_rq_pulse) begin
			mc_rd <= 1;
			mc_wr <= 0;
			mb <= 0;
		end
		if(mc_wr_rq_pulse) begin
			mc_rd <= 0;
			mc_wr <= 1;
		end
		if(mc_rq_pulse)
			mc_rq <= 1;

		if(mc_addr_ack)
			mc_rq <= 0;
		if(mai_rd_rs)
			f1 <= 1;
		if(mb_pulse & mc_rd)
			mb <= mb | membus_mb_in;
		if(mc_rs_t1)
			mc_rd <= 0;

		if(key_wr)
			mb <= ar;
	end

	assign ff0 = { 2'b0, f0, f1, key_rdwr, mc_rd, mc_wr, mc_rq  };

endmodule
