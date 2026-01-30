`include "commands.v"

module uart_controller#(
	parameter DEV_ID				= 0,
	parameter UART_ADDRESS_WIDTH	= 0
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
	
	// between modules
	output wire [UART_ADDRESS_WIDTH-1:0]		uart_slot_en
);

/*
reg chan_sel_valid;
reg [UART_ADDRESS_WIDTH-1:0]		chan_set;

// set active slot pins for rx, tx
always @ (posedge clk) begin
	if (!resetn) begin
		chan_sel_valid		<= 1'b0;
	end else if (spi_cmd_r == `C_SET_UART_SLOT && spi_addr_r == DEV_ID && spi_data_valid_r) begin
		chan_set <= spi_data_r[UART_ADDRESS_WIDTH-1:0];
		chan_sel_valid		<= 1'b1;
	end else begin
		chan_set <= chan_set;
		chan_sel_valid		<= 1'b0;
	end	
end

always @ (posedge clk) begin
	if (!resetn) 
		uart_slot_en <= 0;	// 15 is no slot selected
	else if (chan_sel_valid) 
		uart_slot_en <= chan_set;
	else	
		uart_slot_en <= uart_slot_en;
end


endmodule*/


reg [UART_ADDRESS_WIDTH-1:0]		uart_slot_en_r;
assign uart_slot_en = uart_slot_en_r;

// ********** spi sdi sets **********
// set active slot pins for rx, tx
always @ (posedge clk) begin
	if (!resetn) begin
		uart_slot_en_r <= 0;	// 15 is no slot selected
	end else if (spi_cmd_r == `C_SET_UART_SLOT && spi_addr_r == `UART_CONTROLLER_ADDRESS && spi_data_valid_r) begin
		uart_slot_en_r <= spi_data_r[UART_ADDRESS_WIDTH-1:0];
	end else begin
		uart_slot_en_r <= uart_slot_en_r;
	end	
end

endmodule
