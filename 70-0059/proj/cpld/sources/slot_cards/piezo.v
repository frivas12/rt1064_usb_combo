`include "commands.v"

module piezo#(
	parameter DEV_ID				= 0,
	parameter UART_ADDRESS_WIDTH	= 0
)
(
	input wire 						clk,				
	input wire 						resetn,

	output wire						CS_DAC,			// C0	CS for DAC SPI
//	output wire						C_SCLK,			// C1	DAC SPI clock, not used for now
//	output wire						C_MOSI,			// C2	DAC SPI data in, not used for now
	output wire						CS_ADC,			// C3	CS for ADC SPI
	input wire						TRIGGER,		// C4	Input trigger for piezo card interrupt 
	inout wire						OW_ID,			// C5	in/out for one wire ID
//	inout wire						NA,			// C6	
//	output wire						NA,			// C7 	
//	input wire						NA,			// C8	
//	input wire						NA,			// C9	
	
	// SPI
	// used for sdi
	input wire 	[15:0]				spi_cmd_r,			
	input wire 	[7:0]				spi_addr_r,			
	input wire 	[39:0]				spi_data_r,			
	input wire 						spi_data_valid_r,	
	
	// between modules
	input wire 							cs_decoded_in,    
	input wire 							cs_decoded_in2,
	
	// uart for one wire
	input wire [UART_ADDRESS_WIDTH-1:0]	uart_slot_en,		
	output wire							rx_slot,
	input wire 							tx_slot,

	input wire	 						EM_STOP	
);

reg [2:0] mode;			// card mode or disabled or enabled

// All output must be tristated when system is reset and any output must be set is
// the card is enabled in a particular slot
// setup outputs to enable if module is enabled otherwise tristate
assign CS_DAC = 		(mode > `DISABLED_MODE) ?  cs_decoded_in : 1'bz;
assign CS_ADC = 		(mode > `DISABLED_MODE) ?  cs_decoded_in2 : 1'bz;

// Cable detection address starts on slot + 7 so we can address other uart on slot
assign OW_ID = 	 (mode > `DISABLED_MODE && uart_slot_en == (DEV_ID + 7) && tx_slot == 0) ?  tx_slot : 1'bz;
assign rx_slot = (mode > `DISABLED_MODE  && uart_slot_en == (DEV_ID + 7)) ?  OW_ID : 1'bz;

// ********** spi sdi sets **********
// Enable/disable piezo card
always @ (posedge clk) begin
	if (!resetn) begin 
		mode <= `DISABLED_MODE;
	end else if (spi_cmd_r == `C_SET_ENABLE_PEIZO_CARD && spi_addr_r == DEV_ID && spi_data_valid_r) begin
		mode <= spi_data_r[2:0];
	end else begin
		mode <= mode;
	end	
end


endmodule
