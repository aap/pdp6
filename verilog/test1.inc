		pdp6.fmem0.ff['o0] = 36'o000000_010001;
		pdp6.fmem0.ff['o1] = 36'o000000_010222;
		pdp6.fmem0.ff['o2] = 36'o700000_200006;
		pdp6.fmem0.ff['o3] = 36'o500000_000004;
		pdp6.fmem0.ff['o4] = 36'o000000_010304;
		pdp6.fmem0.ff['o5] = 36'o377777_777777;
		pdp6.fmem0.ff['o6] = 36'o444000_222000;
		pdp6.fmem0.ff['o7] = 36'o777776_000010;		// BLK ptr
		pdp6.fmem0.ff['o10] = 36'o000002_001000;	// BLT ptr
		pdp6.fmem0.ff['o11] = 36'o440600_001000;	// char ptr
		pdp6.fmem0.ff['o12] = 36'o300600_001000;	// char ptr
		pdp6.fmem0.ff['o13] = 36'o000000_005555;	// char
		pdp6.fmem0.ff['o14] = 36'o010700_001017;
		pdp6.fmem0.ff['o17] = 36'o777000_001000;	// PDL ptr
//		pdp6.fmem0.ff['o17] = 36'o777000_777777;	// PDL ptr

		pdp6.mem0.core['o20] = 36'o200_064_000104;	// MOVE 1,@104(4)	FAC_INH
		pdp6.mem0.core['o21] = 36'o202_064_000104;	// MOVEM 1,@104(4)
		pdp6.mem0.core['o22] = 36'o245_100_000003;	// ROTC 2,3
		pdp6.mem0.core['o23] = 36'o700200_675550;	// CONO	APR,675550
		pdp6.mem0.core['o24] = 36'o700200_102227;	// CONO	APR,102227
		pdp6.mem0.core['o25] = 36'o700240_000005;	// CONI	APR,5
		pdp6.mem0.core['o26] = 36'o700140_000006;	// DATAO	APR,6
		pdp6.mem0.core['o27] = 36'o700040_000005;	// DATAI	APR,5
		pdp6.mem0.core['o30] = 36'o700000_000007;	// BLKI	APR,7

		pdp6.mem0.core['o40] = 36'o700640_000005;	// CONI	APR,5
		pdp6.mem0.core['o41] = 36'o260740_000020;	// PUSHJ 17,20
		pdp6.mem0.core['o41] = 36'o250040_000000;	// AOS	1,
		pdp6.mem0.core['o42] = 36'o270000_000001;	// ADD	0,1
		pdp6.mem0.core['o43] = 36'o274000_000001;	// SUB	0,1

		pdp6.mem0.core['o54] = 36'o245_100_000003;	// ROTC 2,3
		pdp6.mem0.core['o55] = 36'o245_100_777775;	// ROTC 2,-3
		pdp6.mem0.core['o56] = 36'o244_100_000001;	// ASHC 2,1

		pdp6.mem0.core['o60] = 36'o251_400_001001;	// BLT 10,1007
		pdp6.mem0.core['o64] = 36'o133_000_000011;	// IBP 11
		pdp6.mem0.core['o65] = 36'o135_000_000012;	// LBP 0,12
		pdp6.mem0.core['o66] = 36'o134_000_000012;	// ILBP 0,12
		pdp6.mem0.core['o67] = 36'o137_540_000012;	// DBP 13,12
		pdp6.mem0.core['o70] = 36'o134_000_000014;	// ILBP 0,14

		pdp6.mem0.core['o100] = 36'o221_000_000123;	// IMULI 0,123

		pdp6.mem0.core['o110] = 36'o231_200_000123;	// IDIVI 4,123
		pdp6.mem0.core['o111] = 36'o231_340_000123;	// IDIVI 7,123

		pdp6.mem0.core['o1000] = 36'o50_45_54_54_57_00;
		pdp6.mem0.core['o10410] = 36'o000_000_000333;
