test.rim: main.rel tty.rel
	ld6 -r 100 -o test.rim main.rel tty.rel

%.rel: %.s
	as6 -o $@ $<
