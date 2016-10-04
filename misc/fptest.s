. = 1000
#	MOVE	0,FP
#	FMP	0,FP+4
#	FSC	0,1
	MOVE	0,DV
#	FDV	0,DV+1
	FSB	0,DV+1
#	FAD	0,DV+1
	JRST	4,

FP:
	2.0
	-1.0
	1.5
	-1.5
	10.0
	-10.0

DV:
	12.0
	4.0
