/*

0	CTL1_DN
1	CTL1_UP
2	CTL2_DN
3	CTL2_UP
4	MAINT_DN
5	MAINT_UP

6	DS LT
7	DS RT
10	MAS
11	REPEAT
12	IR
13	MI LT
14	MI RT
15	PC
16	MA
17	PI

20	MB LT
21	MB RT
22	AR LT
23	AR RT
24	MQ LT
25	MQ RT
26	FF1
27	FF2
30	FF3
31	FF4

32	MMU
33	TTY
34	PTP
35	PTR
36	PTR B LT
37	PTR B RT


40	IO STATUS

*/

module panel_6(
	input wire clk,
	input wire reset,

	// Avalon Slave
	input wire [5:0] s_address,
	input wire s_write,
	input wire s_read,
	input wire [31:0] s_writedata,
	output reg [31:0] s_readdata,
	output wire s_waitrequest,

	/*
	 * APR
	 */

	// keys
	output reg key_start,
	output reg key_read_in,
	output reg key_mem_cont,
	output reg key_inst_cont,
	output reg key_mem_stop,
	output reg key_inst_stop,
	output reg key_exec,
	output reg key_io_reset,
	output reg key_dep,
	output reg key_dep_nxt,
	output reg key_ex,
	output reg key_ex_nxt,

	// switches
	output reg sw_addr_stop,
	output reg sw_mem_disable,
	output reg sw_repeat,
	output reg sw_power,
	output reg [0:35] datasw,
	output reg [18:35] mas,

	// maintenance switches
	output reg sw_rim_maint,
	output reg sw_repeat_bypass,
	output reg sw_art3_maint,
	output reg sw_sct_maint,
	output reg sw_split_cyc,

	// lights
	input wire power,
	input wire [0:17] ir,
	input wire [0:35] mi,
	input wire [0:35] ar,
	input wire [0:35] mb,
	input wire [0:35] mq,
	input wire [18:35] pc,
	input wire [18:35] ma,
	input wire run,
	input wire mc_stop,
	input wire pi_active,
	input wire [1:7] pih,
	input wire [1:7] pir,
	input wire [1:7] pio,
	input wire [18:25] pr,
	input wire [18:25] rlr,
	input wire [18:25] rla,
	input wire [0:7] ff0,
	input wire [0:7] ff1,
	input wire [0:7] ff2,
	input wire [0:7] ff3,
	input wire [0:7] ff4,
	input wire [0:7] ff5,
	input wire [0:7] ff6,
	input wire [0:7] ff7,
	input wire [0:7] ff8,
	input wire [0:7] ff9,
	input wire [0:7] ff10,
	input wire [0:7] ff11,
	input wire [0:7] ff12,
	input wire [0:7] ff13,

	/*
	 * TTY
	 */
	input wire [7:0] tty_tti,
	input wire [6:0] tty_status,

	/*
	 * PTR
	 */
	output reg ptr_key_start,
	output reg ptr_key_stop,
	output reg ptr_key_tape_feed,
	input wire [35:0] ptr,
	input wire [6:0] ptr_status,	// also includes motor on

	/*
	 * PTP
	 */
	output reg ptp_key_tape_feed,
	input wire [7:0] ptp,
	input wire [6:0] ptp_status,	// also includes motor on

	/*
	 * 340 display
	 */
	input wire [0:17] dis_br,
	input wire [0:6] dis_brm,
	input wire [0:9] dis_x,
	input wire [0:9] dis_y,
	input wire [1:4] dis_s,
	input wire [0:2] dis_i,
	input wire [0:2] dis_mode,
	input wire [0:1] dis_sz,
	input wire [0:8] dis_flags,
	input wire [0:4] dis_fe,
	input wire [31:0] dis_foo,

	/*
	 * External panel
	 */
	input wire [3:0] switches,
	input wire [7:0] ext,
	output reg [7:0] leds
);

	wire ext_sw_power = switches[0];

	wire [7:0] apr_status = { 5'b0, mc_stop, run, power };
	always @(*) begin
		case(switches[3:1])
		3'b000: leds <= apr_status;
		3'b001: leds <= tty_tti;
		3'b010: leds <= tty_status;
		3'b011: leds <= ptr;
		3'b100: leds <= ptr_status;

//		3'b101: leds <= ptp;
//		3'b110: leds <= ptp_status;
		3'b101: leds <= dis_fe;

		3'b111: leds <= ext;
		default: leds <= 0;
		endcase
	end


	always @(*) begin
		case(s_address)
		6'o00: s_readdata <=
			{ 20'b0,
			  power, mc_stop, run, sw_addr_stop,
			  key_exec, key_io_reset, key_mem_stop, key_inst_stop,
			  key_mem_cont, key_inst_cont, key_read_in, key_start
			};
		6'o01: s_readdata <= 0;

		6'o02: s_readdata <=
			{ 22'b0,
			  sw_mem_disable, sw_repeat,
			  ptr_key_tape_feed, ptp_key_tape_feed,
			  ptr_key_start, ptr_key_stop,
			  key_ex_nxt, key_ex, key_dep_nxt, key_dep
			};
		6'o03: s_readdata <= 0;

		6'o04: s_readdata <=
			{ 26'b0,
			  sw_split_cyc, sw_sct_maint,
			  sw_art3_maint, sw_repeat_bypass, sw_rim_maint,
			  1'b0	// spare?
			};
		6'o05: s_readdata <= 0;

		6'o06: s_readdata <= { 14'b0, datasw[0:17] };
		6'o07: s_readdata <= { 14'b0, datasw[18:35] };
		6'o10: s_readdata <= { 14'b0, mas };
		6'o11: s_readdata <= 0;	// TODO: repeat
		6'o12: s_readdata <= { 14'b0, ir };
		6'o13: s_readdata <= { 14'b0, mi[0:17] };
		6'o14: s_readdata <= { 14'b0, mi[18:35] };
		6'o15: s_readdata <= { 14'b0, pc };
		6'o16: s_readdata <= { 14'b0, ma };
		6'o17: s_readdata <= { 10'b0, pih, pir, pio, pi_active };
		6'o20: s_readdata <= { 14'b0, mb[0:17] };
		6'o21: s_readdata <= { 14'b0, mb[18:35] };
		6'o22: s_readdata <= { 14'b0, ar[0:17] };
		6'o23: s_readdata <= { 14'b0, ar[18:35] };
		6'o24: s_readdata <= { 14'b0, mq[0:17] };
		6'o25: s_readdata <= { 14'b0, mq[18:35] };
		6'o26: s_readdata <= { ff0, ff1, ff2, ff3 };
		6'o27: s_readdata <= { ff4, ff5, ff6, ff7 };
		6'o30: s_readdata <= { ff8, ff9, ff10, ff11 };
		6'o31: s_readdata <= { ff12, ff13, 16'b0 };
		6'o32: s_readdata <= { 8'b0, rla, rlr, pr };
		6'o33: s_readdata <= { tty_tti, 2'b0, tty_status };
		6'o34: s_readdata <= { ptp, 2'b0, ptp_status };
		6'o35: s_readdata <= ptr_status;
		6'o36: s_readdata <= ptr[35:18];
		6'o37: s_readdata <= ptr[17:0];
		6'o40: s_readdata <= dis_br;
		6'o41: s_readdata <= { dis_brm, dis_y, dis_x };
		6'o42: s_readdata <= { dis_flags, dis_s, dis_i,
			dis_sz, dis_mode };
		6'o43: s_readdata <= dis_foo;
		default: s_readdata <= 0;
		endcase
	end

	assign s_waitrequest = 0;

	always @(posedge clk or negedge reset) begin
		if(~reset) begin
			// keys
			key_start <= 0;
			key_read_in <= 0;
			key_mem_cont <= 0;
			key_inst_cont <= 0;
			key_mem_stop <= 0;
			key_inst_stop <= 0;
			key_exec <= 0;
			key_io_reset <= 0;
			key_dep <= 0;
			key_dep_nxt <= 0;
			key_ex <= 0;
			key_ex_nxt <= 0;

			ptr_key_start <= 0;
			ptr_key_stop <= 0;
			ptr_key_tape_feed <= 0;
			ptp_key_tape_feed <= 0;


			// switches
			sw_addr_stop <= 0;
			sw_mem_disable <= 0;
			sw_repeat <= 0;
/**/			sw_power <= 0;
			datasw <= 0;
			mas <= 0;

			// maintenance switches
			sw_rim_maint <= 0;
			sw_repeat_bypass <= 0;
			sw_art3_maint <= 0;
			sw_sct_maint <= 0;
			sw_split_cyc <= 0;
		end else begin
			sw_power <= ext_sw_power;

			if(s_write) case(s_address)
			6'o00: begin
				if(s_writedata[0])
					{ key_read_in, key_start } <= 2'b01;
				if(s_writedata[1])
					{ key_read_in, key_start } <= 2'b10;
				if(s_writedata[2])
					{ key_mem_cont, key_inst_cont } <= 2'b01;
				if(s_writedata[3])
					{ key_mem_cont, key_inst_cont } <= 2'b10;
				if(s_writedata[4])
					{ key_mem_stop, key_inst_stop } <= 2'b01;
				if(s_writedata[5])
					{ key_mem_stop, key_inst_stop } <= 2'b10;
				if(s_writedata[6])
					{ key_exec, key_io_reset } <= 2'b01;
				if(s_writedata[7])
					{ key_exec, key_io_reset } <= 2'b10;
				if(s_writedata[8])
					sw_addr_stop <= 1;
			end
			6'o01: begin
				if(s_writedata[0] | s_writedata[1])
					{ key_read_in, key_start } <= 2'b00;
				if(s_writedata[2] | s_writedata[3])
					{ key_mem_cont, key_inst_cont } <= 2'b00;
				if(s_writedata[4] | s_writedata[5])
					{ key_mem_stop, key_inst_stop } <= 2'b00;
				if(s_writedata[6]  |s_writedata[7])
					{ key_exec, key_io_reset } <= 2'b00;
				if(s_writedata[8])
					sw_addr_stop <= 0;
			end
			6'o02: begin
				if(s_writedata[0])
					{ key_dep_nxt, key_dep } <= 2'b01;
				if(s_writedata[1])
					{ key_dep_nxt, key_dep } <= 2'b10;
				if(s_writedata[2])
					{ key_ex_nxt, key_ex } <= 2'b01;
				if(s_writedata[3])
					{ key_ex_nxt, key_ex } <= 2'b10;
				if(s_writedata[4])
					{ ptr_key_start, ptr_key_stop } <= 2'b01;
				if(s_writedata[5])
					{ ptr_key_start, ptr_key_stop } <= 2'b10;
				if(s_writedata[6])
					{ ptp_key_tape_feed, ptr_key_tape_feed } <= 2'b10;
				if(s_writedata[7])
					{ ptp_key_tape_feed, ptr_key_tape_feed } <= 2'b01;
				if(s_writedata[8])
					sw_repeat <= 1;
				if(s_writedata[9])
					sw_mem_disable <= 1;
			end
			6'o03: begin
				if(s_writedata[0] | s_writedata[1])
					{ key_dep_nxt, key_dep } <= 2'b00;
				if(s_writedata[2] | s_writedata[3])
					{ key_ex_nxt, key_ex } <= 2'b00;
				if(s_writedata[4] | s_writedata[5])
					{ ptr_key_start, ptr_key_stop } <= 2'b00;
				if(s_writedata[6] | s_writedata[7])
					{ ptp_key_tape_feed, ptr_key_tape_feed } <= 2'b00;
				if(s_writedata[8])
					sw_repeat <= 0;
				if(s_writedata[9])
					sw_mem_disable <= 0;
			end
			6'o04: begin
				if(s_writedata[1])
					sw_rim_maint <= 1;
				if(s_writedata[2])
					sw_repeat_bypass <= 1;
				if(s_writedata[3])
					sw_art3_maint <= 1;
				if(s_writedata[4])
					sw_sct_maint <= 1;
				if(s_writedata[5])
					sw_split_cyc <= 1;
			end
			6'o05: begin
				if(s_writedata[1])
					sw_rim_maint <= 0;
				if(s_writedata[2])
					sw_repeat_bypass <= 0;
				if(s_writedata[3])
					sw_art3_maint <= 0;
				if(s_writedata[4])
					sw_sct_maint <= 0;
				if(s_writedata[5])
					sw_split_cyc <= 0;
			end
			6'o06: datasw[0:17] <= s_writedata;
			6'o07: datasw[18:35] <= s_writedata;
			6'o10: mas <= s_writedata;
			// TODO: 11 REPEAT
			endcase
		end
	end

endmodule
