`timescale 1ns/1ns

module test;
	reg reset;

	pdp6 pdp6(.clk(1'b0), .reset(reset));

	initial begin
		$dumpfile("inst.vcd");
		$dumpvars();
//		$monitor("ir=%o ir_uuo_a=%b ir_fpch=%b ir_2xx=%b ir_accp_V_memac=%b ir_boole=%b ir_hwt=%b ir_acbm=%b ir_iot_a=%b", pdp6.apr0.ir, pdp6.apr0.ir_uuo_a, pdp6.apr0.ir_fpch, pdp6.apr0.ir_2xx, pdp6.apr0.ir_accp_OR_memac, pdp6.apr0.ir_boole, pdp6.apr0.ir_hwt, pdp6.apr0.ir_acbm, pdp6.apr0.ir_iot_a);
//		$monitor("ir_%o ir_130=%o ir_131=%b ir_fsc=%b ir_cao=%b ir_ldci=%b ir_ldc=%b ir_dpci=%b ir_dpc=%b", pdp6.apr0.ir_130, pdp6.apr0.ir, pdp6.apr0.ir_131, pdp6.apr0.ir_fsc, pdp6.apr0.ir_cao, pdp6.apr0.ir_ldci, pdp6.apr0.ir_ldc, pdp6.apr0.ir_dpci, pdp6.apr0.ir_dpc);
	end

	initial begin: inst
		integer i;
		pdp6.apr0.ar = 0;
		pdp6.apr0.ir = 0;
		for(i = 0; i < 'o700; i = i + 1)
			#10 pdp6.apr0.ir[0:8] = i;
		for(i = 'o700000; i <= 'o700340; i = i + 'o000040)
			#10 pdp6.apr0.ir = i;
		#10;
	end
endmodule
