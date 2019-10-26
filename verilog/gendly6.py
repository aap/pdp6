#!/usr/bin/python
from math import *

# delays are rounded down
clock=20	# cycle time of clock in ns

nsdly="""module dly{ns}ns(input clk, input reset, input in, output p);
	reg [{width}-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + {width}'b1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == {n};
endmodule
"""

def gendlyns(ns):
	n = ns//clock
	nb = ceil(log(n+1,2))
	print(nsdly.format(ns=ns, width=nb, n=n))

gendlyns(50)
gendlyns(70)
gendlyns(100)
gendlyns(150)
gendlyns(200)
gendlyns(250)
gendlyns(300)
gendlyns(400)
gendlyns(450)
gendlyns(550)
gendlyns(750)
gendlyns(800)


usdly="""
module dly{us}us(input clk, input reset, input in, output p);
	reg [{width}-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + {width}'b1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == {n};
endmodule
"""

usldly="""
module ldly{us}us(input clk, input reset, input in, output p, output reg l);
	reg [{width}-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset) begin
			r <= 0;
			l <= 0;
		end else begin
			if(r)
				r <= r + {width}'b1;
			if(in) begin
				r <= 1;
				l <= 1;
			end
			if(p) begin
				r <= 0;
				l <= 0;
			end
		end
	end
	assign p = r == {n};
endmodule
"""

def genldlyus(us):
	s = str(us).replace('.', '_')
	n = int(us*1000//clock)
	nb = ceil(log(n+1,2))
	print(usldly.format(us=s, width=nb, n=n))

def gendlyus(us):
	s = str(us).replace('.', '_')
	n = int(us*1000//clock)
	nb = ceil(log(n+1,2))
	print(usdly.format(us=s, width=nb, n=n))

gendlyus(1)
genldlyus(1)
genldlyus(1.5)
genldlyus(2)
gendlyus(100)
genldlyus(100)


dly="""
module dly{type}(input clk, input reset, input in, output p);
	reg [{width}-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset)
			r <= 0;
		else begin
			if(r)
				r <= r + {width}'b1;
			if(in)
				r <= 1;
		end
	end
	assign p = r == {n};
endmodule
"""

ldly="""
module ldly{type}(input clk, input reset, input in, output p, output reg l);
	reg [{width}-1:0] r;
	always @(posedge clk or posedge reset) begin
		if(reset) begin
			r <= 0;
			l <= 0;
		end else begin
			if(r)
				r <= r + {width}'b1;
			if(in) begin
				r <= 1;
				l <= 1;
			end
			if(p) begin
				r <= 0;
				l <= 0;
			end
		end
	end
	assign p = r == {n};
endmodule
"""

def gendlyms(ms):
	t = str(ms).replace('.', '_')
	n = int(ms*1000*1000//clock)
	nb = ceil(log(n+1,2))
	print(dly.format(type='%sms' % t, width=nb, n=n))

def genldlyms(ms):
	t = str(ms).replace('.', '_')
	n = int(ms*1000*1000//clock)
	nb = ceil(log(n+1,2))
	print(ldly.format(type='%sms' % t, width=nb, n=n))

def genldlys(s):
	t = str(s).replace('.', '_')
	n = int(s*1000*1000*1000//clock)
	nb = ceil(log(n+1,2))
	print(ldly.format(type='%ss' % t, width=nb, n=n))


gendlyms(2.1)
gendlyms(2.5)
gendlyms(5)
genldlyms(5)

genldlys(1)
genldlys(5)
