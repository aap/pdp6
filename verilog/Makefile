a.out: test.v apr.v coremem.v fastmem.v modules.v
	iverilog test.v apr.v coremem.v fastmem.v modules.v

run: a.out
	vvp a.out
