`default_nettype none
`timescale 1ns/1ns

module tb_dly();

	wire clk, reset;
	clock clock(clk, reset);

	reg in;
	wire start;
	wire out50ns, out70ns, out100ns, out150ns;
	wire out200ns, out250ns, out400ns, out800ns;
	dly50ns dstart(clk, ~reset, in, start);
	dly50ns d50ns(clk, ~reset, start, out50ns);
	dly70ns d70ns(clk, ~reset, start, out70ns);
	dly100ns d100ns(clk, ~reset, start, out100ns);
	dly150ns d150ns(clk, ~reset, start, out150ns);
	dly200ns d200ns(clk, ~reset, start, out200ns);
	dly250ns d250ns(clk, ~reset, start, out250ns);
	dly400ns d400ns(clk, ~reset, start, out400ns);
	dly800ns d800ns(clk, ~reset, start, out800ns);

	wire out1us, out1_5us, out2us, out100us;
	wire lv1us, lv1_5us, lv2us, lv100us;
	ldly1us d1us(clk, ~reset, start, out1us, lv1us);
	ldly1_5us d1_5us(clk, ~reset, start, out1_5us, lv1_5us);
	ldly2us d2us(clk, ~reset, start, out2us, lv2us);
	ldly100us d100us(clk, ~reset, start, out100us, lv100us);

	wire driveedge;
	edgedet drive(clk, reset, iot_drive, driveedge);

	wire iot_t2, iot_t3, iot_t3a, iot_t4;
	wire iot_init_setup, iot_final_setup, iot_reset, iot_restart;
	ldly1us iot_dly1(clk, ~reset, start, iot_t2, iot_init_setup);
	ldly1_5us iot_dly2(clk, ~reset,
		iot_t2,
		iot_t3a,
		iot_final_setup);
	ldly2us iot_dly3(clk, ~reset,
		iot_t3a,
		iot_t4,
		iot_reset);
	ldly1us iot_dly4(clk, ~reset,
		iot_t2,
		iot_t3,
		iot_restart);
	wire iot_drive = iot_init_setup | iot_final_setup | iot_t2;

	initial begin
		$dumpfile("dump.vcd");
		$dumpvars();

		in = 0;

		#110;
		in = 1;
		#20;
		in = 0;
	end

	initial begin
		#40000;
		$finish;
	end
endmodule
