DEP=
V=../clk.v ../memory.v ../arbiter.v ../memif.v ../membusif.v ../testcore161c.v ../core161c.v \
	../core164.v ../core32k_x.v ../fast162.v ../fast162_dp.v ../modules_50.v \
	../dly_50.v ../onchip_ram.v ../memory_16.v ../memory_16k.v ../memory_32k.v \
	../panel_6.v ../fakeapr.v ../apr.v ../ptr.v ../ptp.v ../dis340.v

tb_apr: tb_apr.v $(V) $(DEP)
	iverilog -o $@ tb_apr.v $(V)

tb_dis: tb_dis.v $(V) $(DEP)
	iverilog -o $@ tb_dis.v $(V)

tb_ptp: tb_ptp.v $(V) $(DEP)
	iverilog -o $@ tb_ptp.v $(V)

tb_ptr: tb_ptr.v $(V) $(DEP)
	iverilog -o $@ tb_ptr.v $(V)

tb_panel: tb_panel.v $(V) $(DEP)
	iverilog -o $@ tb_panel.v $(V)

tb_membusif_x: tb_membusif_x.v $(V) $(DEP)
	iverilog -o $@ tb_membusif_x.v $(V)

tb_membusif: tb_membusif.v $(V) $(DEP)
	iverilog -o $@ tb_membusif.v $(V)

tb_memif: tb_memif.v $(V) $(DEP)
	iverilog -o $@ tb_memif.v $(V)

tb_arbit: tb_arbit.v $(V) $(DEP)
	iverilog -o $@ tb_arbit.v $(V)

tb_mem: tb_mem.v $(V) $(DEP)
	iverilog -o $@ tb_mem.v $(V)
