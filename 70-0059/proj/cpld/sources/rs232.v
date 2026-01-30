`include "commands.v"

module rs232#(
	parameter DEV_ID				= 0,
	parameter UART_ADDRESS_WIDTH	= 0
)
(
	input wire 						clk,				
	input wire 						resetn,

	output wire 					TX_IN,	// C0	TX_IN data out to RS232
	input wire 						RX_OUT,	// C1	RX_OUT data in from RS232
	
	// SPI
	// used for sdi
	input wire 	[15:0]				spi_cmd_r,			
	input wire 	[7:0]				spi_addr_r,			
	input wire 	[39:0]				spi_data_r,			
	input wire 						spi_data_valid_r,
	
	// between modules
	// uart for one wire
	input wire [UART_ADDRESS_WIDTH-1:0]	uart_slot_en,		
	output wire 					rx_slot,
	input wire 						tx_slot
);

reg mode;			// card mode or disabled or enabled

// All output must be tristated when system is reset and any output must be set is
// the card is enabled in a particular slot
// setup outputs to enable if module is enabled otherwise tristate
//assign TX_IN = 		(mode > `DISABLED_MODE ) ?  tx_slot : 1'bz;
assign TX_IN = 		(mode > `DISABLED_MODE && uart_slot_en == DEV_ID) ?  tx_slot : 1'bz;
assign rx_slot = 	(mode > `DISABLED_MODE  && uart_slot_en == DEV_ID) ?  TX_IN : 1'bz;

// ********** spi sdi sets **********
// Enable/disable card
always @ (posedge clk) begin
	if (!resetn) begin
		mode <= `DISABLED_MODE;
	end else if (spi_cmd_r == `C_SET_ENABLE_RS232_CARD && spi_addr_r == DEV_ID && spi_data_valid_r) begin
		mode <= spi_data_r[0];
	end else begin
		mode <= mode;
	end	
end


endmodule