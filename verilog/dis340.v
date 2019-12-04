module dis340(
	input wire clk,
	input wire reset,

	/* IO bus - 344 interface */
	input  wire iobus_iob_poweron,
	input  wire iobus_iob_reset,
	input  wire iobus_datao_clear,
	input  wire iobus_datao_set,
	input  wire iobus_cono_clear,
	input  wire iobus_cono_set,
	input  wire iobus_iob_fm_datai,
	input  wire iobus_iob_fm_status,
	input  wire iobus_rdi_pulse,	// unused on 6
	input  wire [3:9]  iobus_ios,
	input  wire [0:35] iobus_iob_in,
	output wire [1:7]  iobus_pi_req,
	output wire [0:35] iobus_iob_out,
	output wire iobus_dr_split,
	output wire iobus_rdi_data,	// unused on 6

	/* Indicators */
	output wire [0:17] br_ind,
	output wire [0:6] brm_ind,
	output wire [0:9] x_ind,
	output wire [0:9] y_ind,
	output wire [1:4] s_ind,
	output wire [0:2] i_ind,
	output wire [0:2] mode_ind,
	output wire [0:1] sz_ind,
	output wire [0:8] flags_ind,
	output wire [0:4] fe_ind,
	output wire [31:0] foo_ind,

	/* Avalon slave */
	input wire s_read,
	output wire [31:0] s_readdata,

	output wire fe_data_rq
);
	assign iobus_dr_split = 0;
	assign iobus_rdi_data = 0;

	assign br_ind = br;
	assign brm_ind = brm;
	assign x_ind = x;
	assign y_ind = y;
	assign s_ind = s;
	assign i_ind = i;
	assign mode_ind = mode;
	assign sz_ind = sz;
	assign flags_ind = {
		rfd, cf, 1'b0,		// TODO: CONT?
		stop, move, halt,
		lp_flag, lp_enable,  lp_find
	};

	/* 344 - fantasy */

	wire dis_sel = iobus_ios == 7'b001_011_0;

	wire dis_data_clr;
	wire dis_data_set;
	wire dis_ic_clr;
	wire dis_ic_set;
	wire iob_reset;
	wire dis_datai = dis_sel & iobus_iob_fm_datai;
	wire dis_status = dis_sel & iobus_iob_fm_status;
	pa ptr_pa0(clk, reset, dis_sel & iobus_datao_clear, dis_data_clr);
	pa ptr_pa1(clk, reset, dis_sel & iobus_datao_set, dis_data_set);
	pa ptr_pa2(clk, reset, dis_sel & iobus_cono_clear | iob_reset, dis_ic_clr);
	pa ptr_pa3(clk, reset, dis_sel & iobus_cono_set, dis_ic_set);
	pa ptr_pa4(clk, reset, iobus_iob_reset, iob_reset);

	assign iobus_iob_out =
		dis_datai ? { y, 9'b0, x } :
		dis_status ? { edge_flag_vert, lp_flag, edge_flag_horiz,
			stop_inter, done_flag, 1'b0,
			dis_pia_spec, dis_pia_data } :
		36'b0;

	wire dis_flag_spec = edge_flag_vert | edge_flag_horiz |
		lp_flag | stop_inter;
	wire dis_flag_data = done_flag;
	wire [1:7] dis_req_spec = { dis_flag_spec, 7'b0 } >> dis_pia_spec;
	wire [1:7] dis_req_data = { dis_flag_data, 7'b0 } >> dis_pia_data;
	assign iobus_pi_req = dis_req_spec | dis_req_data;

	reg [30:32] dis_pia_spec;
	reg [33:35] dis_pia_data;
	reg [0:35] dis_ib;
	reg [0:1] dis_ibc;

	always @(posedge clk) begin
		if(dis_ic_clr) begin
			dis_pia_spec <= 0;
			dis_pia_data <= 0;

			// not quite sure..
			dis_ib <= 0;
			dis_ibc <= 0;
		end
		if(dis_ic_set) begin
			dis_pia_spec <= iobus_iob_in[30:32];
			dis_pia_data <= iobus_iob_in[33:35];
		end

		if(dis_data_clr) begin
			dis_ib <= 0;
			dis_ibc <= 0;
		end
		if(dis_data_set) begin
			dis_ib <= dis_ib | iobus_iob_in;
			dis_ibc <= 2'b11;
		end
		if(shift_ib) begin
			dis_ib[0:17] <= dis_ib[18:35];
			dis_ibc <= { dis_ibc[1], 1'b0 };
		end
	end

	wire done_flag = rfd & ~dis_ibc[0];
	pa dpy_pa100(clk, reset, rfd & dis_ibc[0], data_sync);

	// from interface (?)
	wire dpy_go = dis_ic_set & iobus_iob_in[29];
	wire resume = dis_ic_set & ~iobus_iob_in[29];
	wire data_sync;
	wire [0:17] br_input = dis_ib[0:17];

	// ??
	wire clr_flags = 0;


	/* light pen */
	wire lp_pulse = 0;


	/* 340 */

`ifdef simulation
	initial begin
		rfd <= 0;
		halt <= 0;
	end
`endif

	reg rfd;
	reg stop;
	reg halt;
	reg move;
	wire initiate;
	wire escape_pulse;
	wire rfd_pulse;
	wire idp;
	wire pm_pulse;
	wire x_start_pulse;
	wire y_start_pulse;
	wire cg_end_pulse;
	wire next_char;

	wire stop_inter = stop & br[8];

	wire [0:3] inc =
		{4{s[1]}} & br[2:5] |
		{4{s[2]}} & br[6:9] |
		{4{s[3]}} & br[10:13] |
		{4{s[4]}} & br[14:17];

	wire l = (vm | vcm) & horiz_vec & br[10] |
		im & inc[0] & inc[1] |
		cg_l;
	wire r = (vm | vcm) & horiz_vec & ~br[10] |
		im & inc[0] & ~inc[1] |
		cg_r;
	wire d = (vm | vcm) & vert_vec & br[2] |
		im & inc[2] & inc[3] |
		cg_d;
	wire u = (vm | vcm) & vert_vec & ~br[2] |
		im & inc[2] & ~inc[3] |
		cg_u;

	pa dpy_pa0(clk, reset, dpy_go, initiate);
	pa dpy_pa1(clk, reset,
		initiate | cg_escape |
		  count_x & br[0] & halt |
		  dly1_pulse & cf,
		escape_pulse);
	pa dpy_pa2(clk, reset,
		dly6_pulse | initiate | cg_escape |
		  count_x & halt |
		  dly1_pulse & cf & vcm |		// TODO: ???
		  pm_pulse & ~br[7] |
		  next_char & s[4],
		rfd_pulse);
	pa dpy_pa3(clk, reset, data_sync, clr_br);
	pa dpy_pa4(clk, reset, clr_br, clr_brm);
	pa dpy_pa5(clk, reset, read_to_s, load_br);
	wire shift_ib = load_br;
	// TODO: what's RI?
	pa dpy_pa7(clk, reset, idp & pm, pm_pulse);
	pa dpy_pa8(clk, reset, idp & xym & ~lp_flag & br[1], y_start_pulse);
	pa dpy_pa9(clk, reset, idp & xym & ~lp_flag & ~br[1], x_start_pulse);
	pa dpy_pa10(clk, reset,
		pm_pulse | x_start_pulse | y_start_pulse,
		read_to_mode);
	pa dpy_pa11(clk, reset, pm_pulse & br[11], store_scale);
	pa dpy_pa12(clk, reset, pm_pulse & br[14], store_int_level);
	pa dpy_pa13(clk, reset, y_start_pulse, clr_y);
	pa dpy_pa14(clk, reset, x_start_pulse | cg_cr, clr_x);
	pa dpy_pa15(clk, reset, idp & (im | vm | vcm), count_brm);
	pa dpy_pa16(clk, reset, count_brm | cg_count, count_x);
	pa dpy_pa17(clk, reset, count_brm | cg_count, count_y);
	pa dpy_pa18(clk, reset, count_brm | next_char, shift_s);

	pa dpy_pa6(clk, reset, cg_end_level, cg_end_pulse);
	assign next_char = cg_end_pulse & cm;

	wire int_dly1, int_dly2;
	wire int_dly = int_dly1 | int_dly2 | cg_intens;
	wire intensify =
		int_dly1 & move & br[1] |
		int_dly2 & br[7] |
		cg_intens & cg_int;

	wire dly1_pulse;	// sequence delay
	wire dly4_pulse;	// 35 deflection delay
	wire dly6_pulse;	// 0.5 after xy intensify
	ldly500ns intdly_1(clk, reset,
		load_br |	// actually through PA
		  dly1_pulse & ~cf & ~lp_flag & ~rfd |
		  next_char & ~s[4],
		intdly1_pulse /* idp */, int_dly1);
	dly2_8us dpy_dly3(clk, reset, clr_br, read_to_s);
	dly200ns dpy_dly5(clk, reset, x_start_pulse, load_x);
	dly200ns dpy_dly7(clk, reset, y_start_pulse, load_y);
`ifdef simulation
	dly2_8us dpy_dly4(clk, reset, 
`else
	dly35us dpy_dly4(clk, reset,
`endif
		x_start_pulse | y_start_pulse & br[7],
		dly4_pulse);
	ldly500ns intdly_2(clk, reset,
		dly4_pulse | y_start_pulse & ~br[7],
		intdly2_pulse /* dly6_pulse */, int_dly2);
	dly1us dpy_dly1(clk, reset,
		count_brm & ~halt |
		  resume & ~cm,
		dly1_pulse);

	always @(posedge clk) begin
		if(clr_brm) begin
			halt <= 0;
			move <= 0;
		end
		if(count_x & im & s[4] |
		   dly1_pulse & vm & brm == 'o177)
			halt <= 1;

		if(count_x & (l|r|u|d))
			move <= ~cm;
	
		if(initiate | clr_flags)
			stop <= 0;
		if(pm_pulse & br[7])
			stop <= 1;

		if(clr_flags | clr_br)
			rfd <= 0;
		if(rfd_pulse)
			rfd <= 1;
	end

	reg [0:17] br;
	wire clr_br;
	wire load_br;

	reg [0:2] mode;
	wire read_to_mode;
	wire pm  = mode == 3'b000;
	wire xym = mode == 3'b001;
	wire sm  = mode == 3'b010;
	wire cm  = mode == 3'b011;
	wire vm  = mode == 3'b100;
	wire vcm = mode == 3'b101;
	wire im  = mode == 3'b110;

	reg [0:1] sz;
	wire store_scale;
	wire scx8 = sz == 2'b11;
	wire scx4 = sz == 2'b10;
	wire sc = sz[0] | sz[1];

	reg [0:2] i;
	wire store_int_level;

	reg lp_find;
	reg lp_enable;
	reg lp_flag;

	always @(posedge clk) begin
		if(clr_br)
			br <= 0;
		if(load_br)
			br <= br | br_input;

		if(escape_pulse) begin
			mode <= 0;
			lp_find <= 0;
		end
		if(read_to_mode)
			mode <= br[2:4];

		if(store_int_level)
			i <= br[15:17];

		if(store_scale)
			sz <= br[12:13];

		if(initiate | resume)		// initiate not in drawings
			lp_enable <= 0;
		if(read_to_mode & br[5])
			lp_enable <= br[6];

		if(initiate | clr_flags | resume)
			lp_flag <= 0;
		if((count_y | read_to_s) & lp_enable & lp_find)
			lp_flag <= 1;

		if(initiate | resume)
			lp_find <= 0;
		if(lp_pulse & lp_enable)
			lp_find <= 1;
	end


	reg [0:6] brm;
	wire [0:6] brm_comp;
	wire clr_brm;
	wire count_brm;

	assign brm_comp[6] = 1;
	assign brm_comp[5] = brm[6] | ~br[3]&~br[11];
	assign brm_comp[4] = (&brm[5:6]) | (&(~br[3:4]))&(&(~br[11:12]));
	assign brm_comp[3] = (&brm[4:6]) | (&(~br[3:5]))&(&(~br[11:13]));
	assign brm_comp[2] = (&brm[3:6]) | (&(~br[3:6]))&(&(~br[11:14]));
	assign brm_comp[1] = (&brm[2:6]);
	assign brm_comp[0] = (&brm[1:6]);

	wire horiz_vec = (|(~brm[0:6] & brm_comp[0:6] &
		{ br[17], br[16], br[15], br[14], br[13], br[12], br[11] }));
	wire vert_vec = (|(~brm[0:6] & brm_comp[0:6] &
		{ br[9], br[8], br[7], br[6], br[5], br[4], br[3] }));

	always @(posedge clk) begin
		if(clr_brm)
			brm <= 0;
		if(count_brm)
			brm <= brm ^ brm_comp;
	end

	reg [1:4] s;
	wire clr_s = clr_brm;		// not on drawings
	wire shift_s;
	wire read_to_s;

	always @(posedge clk) begin
		if(clr_s)
			s <= 0;
		if(read_to_s & cm)
			s[2] <= 1;
		if(read_to_s & im)
			s[1] <= 1;
		if(shift_s)
			s <= { 1'b0, s[1:3] };
	end

	reg [0:9] x;
	reg [0:9] y;
	reg edge_flag_vert;
	reg edge_flag_horiz;
	wire clr_x, load_x, count_x;
	wire clr_y, load_y, count_y;
	wire clr_cf = clr_brm;		// not on drawings

	wire cf = edge_flag_vert | edge_flag_horiz;

	wire [0:9] xyinc = 1<<sz;
	wire [0:9] xinc = r ? xyinc : l ? -xyinc : 0;
	wire [0:9] yinc = u ? xyinc : d ? -xyinc : 0;
	wire [0:9] xsum = x + xinc;
	wire [0:9] ysum = y + yinc;

	always @(posedge clk) begin
		if(clr_y)
			y <= 0;
		if(load_y)
			y <= y | br[8:17];
		if(count_y) begin
			y <= ysum;
			if((y[0] ^ ysum[0]) & (y[0] ^ yinc[0]))
				edge_flag_vert <= 1;
		end

		if(clr_x)
			x <= 0;
		if(load_x)
			x <= x | br[8:17];
		if(count_x) begin
			x <= xsum;
			if((x[0] ^ xsum[0]) & (x[0] ^ xinc[0]))
				edge_flag_horiz <= 1;
		end

		if(clr_cf | clr_flags | rfd_pulse) begin
			edge_flag_vert <= 0;
			edge_flag_horiz <= 0;
		end
	end


	/* 342 char gen - fantasy */
	wire cg_end_level = cg_end;
	wire cg_cr;
	wire cg_count;
	wire cg_escape;
	wire cg_intens;
	wire [0:5] cg_char =
		{6{s[2]}} & br[0:5] |
		{6{s[3]}} & br[6:11] |
		{6{s[4]}} & br[12:17];
	reg cg_l;
	reg cg_r;
	reg cg_d;
	reg cg_u;
	reg cg_int;
	reg cg_end;

	reg cg_so;
	reg [0:4] cg_pulse;

	reg [31:0] rom_u [0:127];
	reg [31:0] rom_d [0:127];
	reg [31:0] rom_l [0:127];
	reg [31:0] rom_r [0:127];
	reg [31:0] rom_i [0:127];
	reg [31:0] rom_end [0:127];
	reg [29:0] rom_u_q, rom_d_q, rom_l_q, rom_r_q, rom_i_q, rom_end_q;

	initial begin
		$readmemh("roms/u.rom", rom_u);
		$readmemh("roms/d.rom", rom_d);
		$readmemh("roms/l.rom", rom_l);
		$readmemh("roms/r.rom", rom_r);
		$readmemh("roms/in.rom", rom_i);
		$readmemh("roms/end.rom", rom_end);
	end
	always @(posedge clk) begin
		rom_l_q <= rom_l[{cg_so, cg_char}];
		rom_r_q <= rom_r[{cg_so, cg_char}];
		rom_d_q <= rom_d[{cg_so, cg_char}];
		rom_u_q <= rom_u[{cg_so, cg_char}];
		rom_i_q <= rom_i[{cg_so, cg_char}];
		rom_end_q <= rom_end[{cg_so, cg_char}];
	end

	wire cg_start;
	wire cg_strobe, cg_strobe_0, cg_strobe_1;
	wire cg_count_0, cg_count_1;

	pa pa_cg0(clk, reset, idp & cm, cg_start);
	pa pa_cg1(clk, reset, cg_start | cg_count_1, cg_strobe);
	pa pa_cg2(clk, reset, cg_strobe, cg_strobe_0);
	pa pa_cg6(clk, reset, cg_strobe_0, cg_strobe_1);
	pa pa_cg3(clk, reset, cg_strobe_1 & ~cg_end, cg_count);
	pa pa_cg4(clk, reset, cg_start & (cg_char == 6'o37), cg_escape);
	pa pa_cg5(clk, reset, cg_start & (cg_char == 6'o34), cg_cr);

	dly1us dly_cg0(clk, reset, cg_count, cg_count_0);
	ldly500ns intdly_3(clk, reset,
		cg_count_0, intdly3_pulse /* cg_count_1 */, cg_intens);

	always @(posedge clk) begin
		if(iob_reset | reset) begin
			cg_l <= 0;
			cg_r <= 0;
			cg_d <= 0;
			cg_u <= 0;
			cg_int <= 0;
			cg_end <= 0;

			cg_so <= 0;
		end

		// TODO: figure out how shift is set
		if(initiate)
			cg_so <= 0;

		if(cg_start) begin
			cg_pulse <= 0;
			cg_end <= 0;	// has to be set before strobe so we can detect an edge
			if(cg_char == 6'o35)
				cg_so <= 0;				
			if(cg_char == 6'o36)
				cg_so <= 1;				
		end

		if(cg_strobe_0) begin
			cg_l <= rom_l_q[cg_pulse];
			cg_r <= rom_r_q[cg_pulse];
			cg_d <= rom_d_q[cg_pulse];
			cg_u <= rom_u_q[cg_pulse];
			cg_int <= rom_i_q[cg_pulse];
			cg_end <= rom_end_q[cg_pulse];
			cg_pulse <= cg_pulse + 1;
		end
	end


	/* FE interface */
	assign fe_data_rq = fe_req;
//	assign s_readdata = fe_data;
	assign s_readdata = { fe_req, 8'b0, i, y, x };
	reg fe_req;
	reg fe_rs;
//	reg [31:0] fe_data;
	wire int_start;
	wire intdly1_pulse, intdly2_pulse, intdly3_pulse;
	reg intdly1_sync, intdly2_sync, intdly3_sync;

	assign fe_ind = { intdly1_sync, intdly2_sync, intdly3_sync, fe_rs, fe_req };

	reg [29:0] foo;
	always @(posedge clk)
		foo <= rom_end[{cg_so, cg_char}];
	assign foo_ind = foo;

	wire fe_reset = reset | iob_reset;

	pa fe_pa0(clk, fe_reset, int_dly, int_start);
	pa fe_pa1(clk, fe_reset, intdly1_sync & fe_rs, idp);
	pa fe_pa2(clk, fe_reset, intdly2_sync & fe_rs, dly6_pulse);
	pa fe_pa3(clk, fe_reset, intdly3_sync & fe_rs, cg_count_1);

	always @(posedge clk) begin
		if(fe_reset) begin
			intdly1_sync <= 0;
			intdly2_sync <= 0;
			intdly3_sync <= 0;
			fe_req <= 0;
			fe_rs <= 0;
		end else begin
			if(int_start & intensify) begin
				fe_req <= 1;
				fe_rs <= 0;
			//	fe_data <= { 1'b1, 8'b0, i, y, x };
			end
			if(int_start & ~intensify | s_read) begin
				fe_req <= 0;
				fe_rs <= 1;
			//	fe_data <= 0;
			end

			if(intdly1_pulse)
				intdly1_sync <= 1;
			if(idp)
				intdly1_sync <= 0;

			if(intdly2_pulse)
				intdly2_sync <= 1;
			if(dly6_pulse)
				intdly2_sync <= 0;

			if(intdly3_pulse)
				intdly3_sync <= 1;
			if(cg_count_1)
				intdly3_sync <= 0;
		end
	end
endmodule
