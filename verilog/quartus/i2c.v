`default_nettype none

module i2cslv(
	input wire clk,
	input wire reset,
	input wire scl,
	inout reg sda,

	output wire ack,
	output reg dir,
	output reg [6:0] dev,
	input  wire ok,
	output wire [7:0] in,
	input  wire [7:0] out,
	output wire start,
	output wire stop
);
	localparam IDLE    = 0;
	localparam ADDR    = 1;
	localparam RECV    = 2;
	localparam SENDACK = 3;
	localparam SEND    = 4;
	localparam RECVACK = 5;

	localparam WRITE = 0;
	localparam READ = 1;

	reg [2:0] state;
	reg [2:0] n;
	reg [7:0] b;
	reg sda0, sda1, scl0, scl1;

	assign in = b;
	assign ack = state == SENDACK || state == RECVACK;

	assign start = scl1 & scl0 & sda1 & ~sda0;
	assign stop = scl1 & scl0 & ~sda1 & sda0;
	wire scl_rise = ~scl1 & scl0;
	wire scl_fall = scl1 & ~scl0;

	always @(posedge clk) begin
		{ sda1, sda0 } <= { sda0, sda };
		{ scl1, scl0 } <= { scl0, scl };
		if(scl_rise)
			n <= n + 3'b1;
		if(start) begin
			state <= ADDR;
			n <= 0;
		end
		if(stop)
			state <= IDLE;
		case(state)
		ADDR: begin
			if(scl_rise) begin
				if(n == 7) begin
					dir <= sda;
					if(ok)
						state <= SENDACK;
					else
						state <= IDLE;
				end else
					dev <= { dev[5:0], sda };
			end
		end
		RECV: begin
			if(scl_fall)
				sda <= 1'bz;
			if(scl_rise) begin
				if(n == 7)
					state <= SENDACK;
				b <= { b[6:0], sda };
			end
		end
		SENDACK: begin
			if(scl_fall)
				sda <= 1'b0;
			if(scl_rise) begin
				if(dir == WRITE)
					state <= RECV;
				else begin
					b <= out;
					state <= SEND;
				end
				n <= 0;
			end
		end
		SEND: begin
			if(scl_fall) begin
				if(b[7])
					sda <= 1'bz;
				else
					sda <= 1'b0;
				b <= b << 1;
			end
			if(scl_rise & n == 7)
				state <= RECVACK;
		end
		RECVACK: begin
			if(scl_fall)
				sda <= 1'bz;
			if(scl_rise) begin
				b <= out;
				if(sda)
					state <= IDLE;
				else
					state <= SEND;
				n <= 0;
			end
		end
		endcase
	end

endmodule
