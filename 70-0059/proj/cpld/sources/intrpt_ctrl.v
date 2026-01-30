`include "commands.v"
//  File:              intrpt_ctrl.v
//  Description:       interrupt controller
//  Engineer:		   PR

module intrpt_ctrl #( 
	parameter DEV_ID			   = 0,
	parameter NUM_INTRPTS_PER_SLOT = 3
)                                     
(           
	input wire 								clk,
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
	input wire 								intrpt_in_0,
	input wire 								intrpt_in_1,
	input wire 								intrpt_in_2,
	output reg 								intrpt_out
);
// temp
//reg 								intrpt_out;
//reg 	[47:0]						spi_data_out_r_t;	


wire [NUM_INTRPTS_PER_SLOT-1:0]				intrpt_in;
reg [NUM_INTRPTS_PER_SLOT-1:0]				intrpt_in_reg;
reg [NUM_INTRPTS_PER_SLOT-1:0]  			intrpt_in_dly;
wire [NUM_INTRPTS_PER_SLOT-1:0]				intrpt_edge;
wire 										intrpt_all_edges;
reg 										assert_intrpt;
reg 										clear_intrpt; 

assign intrpt_in = {intrpt_in_2, intrpt_in_1, intrpt_in_0};
assign intrpt_edge = (intrpt_in_reg & ~intrpt_in_dly) | (~intrpt_in_reg & intrpt_in_dly);
assign intrpt_all_edges = intrpt_edge[2] | intrpt_edge[1] |intrpt_edge[0];

always @ (posedge clk) begin
	if (!resetn)
		intrpt_in_reg 			<= 'b0;
	else
		intrpt_in_reg 			<= intrpt_in;	
end	

always @ (posedge clk) begin
	if (!resetn)
		intrpt_in_dly 			<= 'b0;
	else
		intrpt_in_dly 			<= intrpt_in_reg;	
end	

always @ (posedge clk) begin
	if (!resetn) begin
		assert_intrpt 			<= 1'b0;
	end	
	else if (intrpt_all_edges) begin
		assert_intrpt	 		<= 1'b1;	
	end	
	else begin
		assert_intrpt	 		<= 1'b0;	
	end
end	


// ********** spi sdo sets **********
// clear interrupt pulse
always @ (posedge clk) begin
	if (!resetn) begin
		clear_intrpt <= 1'b0;
		spi_data_out_r <= 'bz;
	end else if (spi_cmd == `C_READ_INTERUPTS && spi_addr == DEV_ID) begin
		clear_intrpt <= 1'b1;
		spi_data_out_r <= intrpt_in;
	end else begin
		clear_intrpt <= 1'b0;	
		spi_data_out_r <= 'bz;
	end	
end

// interrupt out register
always @ (posedge clk) begin
	if (!resetn || clear_intrpt)
		intrpt_out <= 1'b0;
	else if (assert_intrpt)	
		intrpt_out <= 1'b1;
	else
		intrpt_out <= intrpt_out;
end

endmodule     
    
    
    