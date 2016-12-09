module i2c_core(
	input wire clk,
	input wire reset,

	input wire start,
	input wire stop,
	input wire dir,
	input wire ok,
	input wire done,
	input wire [7:0] in,
	output reg [7:0] out,

	output reg [17:0] sram_a,
	inout  reg [15:0] sram_d,
	output reg sram_ce,
	output reg sram_oe,
	output reg sram_we,
	output reg sram_lb,
	output reg sram_ub
);
	localparam DEV = 0;
	localparam ADDR = 1;
	localparam DATA = 2;

	reg [17:0] caddr;	// core address
	reg [11:0] d;	// a third of a word
	reg [1:0] state;	// major state
	reg [1:0] n;		// minor state
	//  memory controller
	reg [1:0] memstate;
	reg memdir;	// 0: read, 1: write
	reg memdone;

	initial begin
		memstate <= 0;
		memdone <= 0;
		sram_a <= 0;
		sram_d <= 16'bz;
		sram_ce <= 1;
		sram_oe <= 1;
		sram_we <= 1;
		sram_lb <= 1;
		sram_ub <= 1;
	end

	always @(posedge clk) if(ok) begin
		if(memdone)
			memdone <= 0;
		if(start) begin
			state <= DEV;
			n <= 0;
		end

		if(stop)
			sram_a <= 0;
		if(dir == 0 && done)	// WRITE
			case(state)
			DEV:	state <= ADDR;
			ADDR: begin
				caddr <= { caddr[11:0], in[5:0] };
				if(n == 2) begin
					state <= DATA;
					n <= 0;
				end else
					n <= n + 2'b1;
			end
			DATA: begin
				d <= { d[5:0], in[5:0] };
				case(n)
				0: begin	// first 6 bits, came from ADDR
					sram_a <= (caddr << 1) + caddr;
					n <= 2;
				end
				1:	n <= 2;	// first 6 bits, else
				2: begin	// read second 6 bits
					n <= 1;
					// start write
					memdir <= 1;
					memstate <= 1;
				end
				endcase
			end
			endcase
		else if(done)		// READ
			case(state)
			DEV: begin
				state <= DATA;
				sram_a <= (caddr << 1) + caddr;
				// two dummy bytes
				out <= 8'b0;
				d <= 12'b0000;
			end
			DATA: begin
				case(n)
				0: begin
					out <= { 2'b0, d[5:0] };
					n <= 1;
					// get a new 16 bit word
					memdir <= 0;
					memstate <= 1;
				end
				1: begin
					out <= { 2'b0, d[11:6] };
					n <= 0;
				end
				endcase
			end
			endcase

		// Talk to SRAM
		if(memdir == 0)		// read
			case(memstate)
			1: begin
				sram_ce <= 0;
				sram_oe <= 0;
				sram_lb <= 0;
				sram_ub <= 0;
				memstate <= 2;
			end
			2:	memstate <= 3;
			3: begin
				sram_a <= sram_a + 18'b1;
				d <= sram_d[15:4];
				sram_ce <= 1;
				sram_oe <= 1;
				sram_lb <= 1;
				sram_ub <= 1;
				memstate <= 0;
				memdone <= 1;
			end
			endcase
		else			// write
			case(memstate)
			1: begin
				sram_d <= { d, 4'b0 };
				sram_ce <= 0;
				sram_we <= 1;
				sram_lb <= 0;
				sram_ub <= 0;
				memstate <= 2;
			end
			2: begin
				sram_we <= 0;
				memstate <= 3;
			end
			3: begin
				sram_a <= sram_a + 18'b1;
				sram_d <= 16'bz;
				sram_ce <= 1;
				sram_we <= 1;
				sram_lb <= 1;
				sram_ub <= 1;
				memstate <= 0;
				memdone <= 1;
			end
			endcase
	end

endmodule
