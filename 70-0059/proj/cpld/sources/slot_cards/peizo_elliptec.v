//
// This motor is on the extra hafl slot on the board
//

module peizo_elliptec#(
	parameter DEV_ID				= 0,
	parameter UART_ADDRESS_WIDTH	= 0
)
(
	input wire 							clk,				
	input wire 							resetn,

	input wire							rx,	// into uC
	output wire 						tx,	// out from uC

	// SPI
	// used for sdi
	input wire 	[15:0]				spi_cmd_r,			
	input wire 	[7:0]				spi_addr_r,			
	input wire 	[39:0]				spi_data_r,			
	input wire 						spi_data_valid_r,	

// uart between modules
	input wire [UART_ADDRESS_WIDTH-1:0]	uart_slot_en,		
	output wire							rx_slot,
	input wire 							tx_slot
);
reg [2:0] mode;			// card mode or disabled or enabled

assign tx = 	(mode > `DISABLED_MODE && uart_slot_en == (DEV_ID + 7)) ?  tx_slot : 1'bz;
assign rx_slot = (mode > `DISABLED_MODE  && uart_slot_en == (DEV_ID + 7)) ?  rx : 1'bz;
//assign rx_slot = 1'bz;

// ********** spi sdi sets **********
// Enable/disable card
always @ (posedge clk) begin
	if (!resetn) begin
		mode <= `DISABLED_MODE;
	end else if (spi_cmd_r == `C_SET_ENABLE_PEIZO_ELLIPTEC_CARD && spi_addr_r == DEV_ID && spi_data_valid_r) begin
		mode <= spi_data_r[2:0];
	end else begin
		mode <= mode;
	end	
end


endmodule
