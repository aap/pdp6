test.rim: main.rel tty.rel pt.rel
	ld6 -o test.rim main.rel tty.rel pt.rel

test.sav: main.rel tty.rel pt.rel
	ld6 -f sav -o test.sav main.rel tty.rel pt.rel

%.rel: %.s
	as6 -o $@ $<

%.lst: %.s
	as6 -o /dev/null -l $@ $<
