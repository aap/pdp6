test: test.v pdp6.v apr.v core161c.v fast162.v modules.v test1.inc test2.inc
	iverilog -o test test.v pdp6.v apr.v core161c.v fast162.v modules.v

test_dec: test_dec.v pdp6.v apr.v core161c.v fast162.v modules.v
	iverilog -o test_dec test_dec.v pdp6.v apr.v core161c.v fast162.v modules.v

run: test
	vvp test
