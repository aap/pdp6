test.rim: main.rel tty.rel pt.rel dt.rel
	ld6 -o test.rim main.rel tty.rel pt.rel dt.rel

test.sav: main.rel tty.rel pt.rel dt.rel
	ld6 -f sav -o test.sav main.rel tty.rel pt.rel dt.rel

dtboot.rim: dtboot.rel
	ld6 -o dtboot.rim dtboot.rel

%.rel: %.s
	as6 -o $@ $<

%.lst: %.s
	as6 -o /dev/null -l $@ $<
