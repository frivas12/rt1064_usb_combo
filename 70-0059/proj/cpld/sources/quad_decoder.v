`include "commands.v"
// Company: 		Thorlabs Imaging, Sterling, VA
// Engineer: 		PR
// Create Date: 	06/22/2018
// Module Name: 	quad_decoder


module quad_decoder#(
	parameter DEV_ID			= 0,
	parameter QUAD_CNTR_WIDTH 	= 32
)
(
	input wire 								clk,		
	input wire 								clk_1MHz,		
	input wire 								resetn,
	
	// SPI
	// used for sdi
	input wire 	[15:0]						spi_cmd_r,			
	input wire 	[7:0]						spi_addr_r,			
	input wire 	[39:0]						spi_data_r,			
	input wire 								spi_data_valid_r,	
		
	// used for sd0
	input wire 	[15:0]						spi_cmd,			
	input wire 	[7:0]						spi_addr,			
	output reg 	[39:0]						spi_data_out_r,			
		
	// between modules
	input wire 								quad_index,	
	input wire 								quad_a,				// 5 MHz
	input wire 								quad_b
);

reg [2:0] 									quad_a_delayed;
reg [2:0]									quad_b_delayed;
wire 										count_en;
wire										count_dir;
	
reg 	[QUAD_CNTR_WIDTH-1:0]				quad_set;
reg		[1:0]								quad_homing;
reg											quad_set_valid;
reg											quad_set_complete;

reg 	[QUAD_CNTR_WIDTH-1:0]		quad_count;
reg 	[QUAD_CNTR_WIDTH-1:0]		quad_buffer;

// ********** spi sdi sets **********
always @ (posedge clk) begin
	if (!resetn) begin
		quad_set 			<= 'b0;
		quad_set_valid		<= 1'b0;
	end	
	else if (spi_cmd_r == `C_SET_QUAD_COUNTS && spi_addr_r == DEV_ID && spi_data_valid_r) begin
		quad_set 			<= spi_data_r[QUAD_CNTR_WIDTH-1:0];
		quad_set_valid		<= 1'b1;
	end	
	else begin	
		quad_set 			<= quad_set;
		if(quad_set_complete)
			quad_set_valid		<= 1'b0;
	end	
end

always @ (posedge clk) begin
	if (!resetn)
		quad_homing 			<= 2'b00;
	else if (spi_cmd_r == `C_SET_HOMING && spi_addr_r == DEV_ID && spi_data_valid_r) 
		quad_homing 			<= spi_data_r[1:0];
	else	
		quad_homing 			<= quad_homing;
end

// ********** spi sdo sets **********
always @ (posedge clk) begin
	if (!resetn) 
		spi_data_out_r 			<= 'bz;
	else if (spi_cmd == `C_READ_QUAD_COUNTS && spi_addr == DEV_ID) 
		spi_data_out_r 			<= quad_count;
	else if (spi_cmd == `C_READ_QUAD_BUFFER && spi_addr == DEV_ID) 
		spi_data_out_r 			<= quad_buffer;
	else	
		spi_data_out_r 			<= 'bz;
end


/*
// create count_en, count_dir
// count on every edge
always @ (posedge clk) begin
	if (!resetn) begin
		quad_a_delayed <= 0;
		quad_b_delayed <= 0;
	end
	else begin
		quad_a_delayed <= {quad_a_delayed[1:0], quad_a};
		quad_b_delayed <= {quad_b_delayed[1:0], quad_b};
	end
end

assign count_en = quad_a_delayed[1] ^ quad_a_delayed[2] ^ quad_b_delayed[1] ^ quad_b_delayed[2];
assign count_dir = quad_a_delayed[1] ^ quad_b_delayed[2];
*/


(*ASYNC_REG="TRUE"*)reg[1:0] sync, AB; // synchronization registers
reg[1:0] state;
localparam S00=2'b00, S01=2'b01, S10=2'b10, S11=2'b11; // states

always @ (posedge clk_1MHz) // two-stage input synchronizer
    begin
        sync <= {quad_a,quad_b};
        AB <= sync;
end

// using a state maching to filter noise

always @(posedge clk_1MHz) 
	begin 
		if(!resetn) begin
            state <= S00;
            quad_count <= 0;
			quad_set_complete <= 0;
        end else begin
			if (quad_set_valid) begin 				// set encoder counts
				quad_count <= quad_set;	
				quad_set_complete <= 1;
			end
			else
				quad_set_complete <= 0;
				
			if (quad_homing == 2'b01) begin 		// homing to a limit
				if (quad_index)
					quad_count <= 0;
			end
				
			case(state)              
					S00: if(AB == 2'b01) begin
							quad_count <= quad_count-1;
							state <= S01;
						end else if(AB == 2'b10) begin
							quad_count <= quad_count+1;
							state <= S10;
						end                                        
					S01: if(AB == 2'b00) begin
							quad_count <= quad_count+1;
							state <= S00;
						end else if(AB == 2'b11) begin
							quad_count <= quad_count-1;
							state <= S11;
						end                      
					S10: if(AB == 2'b00) begin
							quad_count <= quad_count-1;
							state <= S00;
						end else if(AB == 2'b11) begin
							quad_count <= quad_count+1;
							state <= S11;
						end                     
					S11: if(AB == 2'b01) begin
							quad_count <= quad_count+1;
							state <= S01;
						end else if(AB == 2'b10) begin
							quad_count <= quad_count-1;
							state <= S10;
						end
				endcase
		end 
			
end 


/*
// quad_count
always @ (posedge clk_1MHz) begin	// pr changed to slower clock
	if (!resetn) begin
		quad_count <= 0;
		// quad_buffer <= 0;
	end
	else if (quad_homing == 2'b00) begin // not homing
		// quad_buffer <= quad_buffer;
		if (!quad_set_valid) begin				// not setting encoder counts
			if (count_en && count_dir)
				quad_count <= quad_count + 1;	// count up
			else if (count_en && !count_dir)
				quad_count <= quad_count - 1;	// count down
		end		
		else
			quad_count <= quad_set;
	end
	else if (quad_homing == 2'b01) begin	// homing to limit
		// quad_buffer <= quad_buffer;
		if (quad_index)
				quad_count <= 0;
		else 
			if (count_en && count_dir)
				quad_count <= quad_count + 1;
			else if (count_en && !count_dir)
				quad_count <= quad_count - 1;
	end
	// else if (quad_homing == 2'b10) begin
		// quad_buffer <= quad_count;
	// 	if (count_en && count_dir)
	// 		quad_count <= quad_count + 1;
	// 	else if (count_en && !count_dir)
	// 		quad_count <= quad_count - 1;
	// end
	else begin
		quad_count <= quad_count;
		// quad_buffer <= quad_buffer;
	end
end
*/

always @ (posedge quad_index) begin
	quad_buffer <= quad_count;
end

endmodule