`default_nettype none
`timescale 1ns/1ns

module tb_panel();

	wire clk, reset;
	clock clock(clk, reset);

	reg a_write = 0;
	reg a_read = 0;
	reg [31:0] a_writedata = 0;
	reg [4:0] a_address;
	wire [31:0] a_readdata;
	wire a_waitrequest;


	wire key_start;
	wire key_read_in;
	wire key_mem_cont;
	wire key_inst_cont;
	wire key_mem_stop;
	wire key_inst_stop;
	wire key_exec;
	wire key_io_reset;
	wire key_dep;
	wire key_dep_nxt;
	wire key_ex;
	wire key_ex_nxt;

	// switches
	wire sw_addr_stop;
	wire sw_mem_disable;
	wire sw_repeat;
	wire sw_power;
	wire [0:35] datasw;
	wire [18:35] mas;

	// maintenance switches
	wire sw_rim_maint;
	wire sw_repeat_bypass;
	wire sw_art3_maint;
	wire sw_sct_maint;
	wire sw_split_cyc;

	// lights
	wire power;
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
	wire [0:7] ff0;
	wire [0:7] ff1;
	wire [0:7] ff2;
	wire [0:7] ff3;
	wire [0:7] ff4;
	wire [0:7] ff5;
	wire [0:7] ff6;
	wire [0:7] ff7;
	wire [0:7] ff8;
	wire [0:7] ff9;
	wire [0:7] ff10;
	wire [0:7] ff11;
	wire [0:7] ff12;
	wire [0:7] ff13;

	panel_6 panel(
		.clk(clk),
		.reset(reset),

		.s_address(a_address),
		.s_write(a_write),
		.s_read(a_read),
		.s_writedata(a_writedata),
		.s_readdata(a_readdata),
		.s_waitrequest(a_waitrequest),

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
		.power(power),
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
		.ff0(ff0),
		.ff1(ff1),
		.ff2(ff2),
		.ff3(ff3),
		.ff4(ff4),
		.ff5(ff5),
		.ff6(ff6),
		.ff7(ff7),
		.ff8(ff8),
		.ff9(ff9),
		.ff10(ff10),
		.ff11(ff11),
		.ff12(ff12),
		.ff13(ff13)
	);

	fakeapr apr(
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
		.power(power),
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
		.ff0(ff0),
		.ff1(ff1),
		.ff2(ff2),
		.ff3(ff3),
		.ff4(ff4),
		.ff5(ff5),
		.ff6(ff6),
		.ff7(ff7),
		.ff8(ff8),
		.ff9(ff9),
		.ff10(ff10),
		.ff11(ff11),
		.ff12(ff12),
		.ff13(ff13)
	);

	initial begin
		$dumpfile("dump.vcd");
		$dumpvars();

		memory.mem[4] = 123;
		memory.mem[5] = 321;
		memory.mem['o123] = 36'o112233445566;

		#5;

		#200;

		@(posedge clk);
		a_address <= 6;
		a_write <= 1;
		a_writedata = 32'o123456;
		@(negedge a_write);

		@(posedge clk);
		a_address <= 7;
		a_write <= 1;
		a_writedata = 32'o654321;
		@(negedge a_write);

		@(posedge clk);
		a_address <= 'o10;
		a_write <= 1;
		a_writedata = 32'o112233;
		@(negedge a_write);

		@(posedge clk);
		a_address <= 'o01;
		a_write <= 1;
		a_writedata = 32'o7777777;
		@(negedge a_write);


		@(posedge clk);
		a_address <= 4;
		a_read <= 1;
		@(negedge a_read);

		@(posedge clk);
		a_address <= 5;
		a_read <= 1;
		@(negedge a_read);

/*
		// write low word
		@(posedge clk);
		a_address <= 1;
		a_write <= 1;
		a_writedata <= 32'o111222;
		@(negedge a_write);

		// write high word
		@(posedge clk);
		a_address <= 2;
		a_write <= 1;
		a_writedata <= 32'o333444;
		@(negedge a_write);
*/
	end

	initial begin
		#40000;
		$finish;
	end

	reg [0:35] data;
	always @(posedge clk) begin
		if(~a_waitrequest & a_write)
			a_write <= 0;
		if(~a_waitrequest & a_read) begin
			a_read <= 0;
			data <= a_readdata;
		end
	end

endmodule
