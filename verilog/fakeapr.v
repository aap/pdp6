module fakeapr(
	input wire clk,
	input wire reset,

	// keys
	input wire key_start,
	input wire key_read_in,
	input wire key_mem_cont,
	input wire key_inst_cont,
	input wire key_mem_stop,
	input wire key_inst_stop,
	input wire key_exec,
	input wire key_io_reset,
	input wire key_dep,
	input wire key_dep_nxt,
	input wire key_ex,
	input wire key_ex_nxt,

	// switches
	input wire sw_addr_stop,
	input wire sw_mem_disable,
	input wire sw_repeat,
	input wire sw_power,
	input wire [0:35] datasw,
	input wire [18:35] mas,

	// maintenance switches
	input wire sw_rim_maint,
	input wire sw_repeat_bypass,
	input wire sw_art3_maint,
	input wire sw_sct_maint,
	input wire sw_split_cyc,

	// lights
	output wire power,
	output wire [0:17] ir,
	output wire [0:35] mi,
	output wire [0:35] ar,
	output wire [0:35] mb,
	output wire [0:35] mq,
	output wire [18:35] pc,
	output wire [18:35] ma,
	output wire run,
	output wire mc_stop,
	output wire pi_active,
	output wire [1:7] pih,
	output wire [1:7] pir,
	output wire [1:7] pio,
	output wire [18:25] pr,
	output wire [18:25] rlr,
	output wire [18:25] rla,
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
	output wire [0:7] ff13
);

	assign power = sw_power;
	assign ir = 18'o111111;
	assign mi = 36'o222222333333;
	assign ar = 36'o444444555555;
	assign mb = 36'o666666777777;
	assign mq = 36'o101010202020;
	assign pc = 18'o303030;
	assign ma = 18'o404040;
	assign run = datasw[35];
	assign mc_stop = datasw[34];
	assign pi_active = 0;
	assign pih = 7'o123;
	assign pir = 7'o134;
	assign pio = 7'o145;
	assign pr = 8'o352;
	assign rlr = 8'o333;
	assign rla = 8'o222;
	assign ff0 = 8'o201;
	assign ff1 = 8'o202;
	assign ff2 = 8'o203;
	assign ff3 = 8'o204;
	assign ff4 = 8'o205;
	assign ff5 = 8'o206;
	assign ff6 = 8'o207;
	assign ff7 = 8'o210;
	assign ff8 = 8'o211;
	assign ff9 = 8'o212;
	assign ff10 = 8'o213;
	assign ff11 = 8'o214;
	assign ff12 = 8'o215;
	assign ff13 = 8'o216;

endmodule
