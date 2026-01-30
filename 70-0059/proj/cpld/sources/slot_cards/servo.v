`include "commands.v"

module servo#(
	parameter DEV_ID				= 0,
	parameter UART_ADDRESS_WIDTH	= 0
)
(
	input wire 						clk,				
	input wire 						resetn,

	output wire						MODE,			// C0	set to high
	input wire						ul,				// C1	in (limit is high when on the limit)
	input wire						ll,				// C2	in (limit is high when on the limit)
//	input wire						nFault,			// C3	not used
	output wire						MODE1,			// C4	set to high
	output wire						Phase,			// C5	for direction
	inout wire						OW_ID,			// C6	in/out for one wire ID
	output wire						Enable,			// C7 	used for PWM
	input wire						quad_a,			// C8	in
	input wire						quad_b,			// C9	in
	
	// SPI
	// used for sdi
	input wire 	[15:0]				spi_cmd_r,			
	input wire 	[7:0]				spi_addr_r,			
	input wire 	[39:0]				spi_data_r,			
	input wire 						spi_data_valid_r,	
	
	// between modules
	output wire 					quad_a_out,
	output wire 					quad_b_out,
	output wire 					ul_out,
	output wire 					ll_out,
	input wire	 					pwm_in,
	
	// uart for one wire
	input wire [UART_ADDRESS_WIDTH-1:0]	uart_slot_en,		
	output wire							rx_slot,
	input wire 							tx_slot,

	input wire	 					EM_STOP	
);

reg [2:0] mode;			// card mode or disabled or enabled
reg Phase_r;
reg Enable_r;

// All output must be tristated when system is reset and any output must be set is
// the card is enabled in a particular slot
// setup outputs to enable if module is enabled otherwise tristate
assign MODE = 		(mode > `ONE_WIRE_MODE) ?  1 : 1'bz;
assign MODE1 = 		(mode > `ONE_WIRE_MODE) ?  1 : 1'bz;
assign Phase = 		(mode > `ONE_WIRE_MODE) ?  Phase_r : 1'bz;
assign Enable = 	(mode > `ONE_WIRE_MODE) ?  pwm_in : 1'bz;
assign quad_a_out = (mode > `ONE_WIRE_MODE) ?  quad_a : 1'bz;
assign quad_b_out = (mode > `ONE_WIRE_MODE) ?  quad_b : 1'bz;
assign ul_out = 	(mode > `ONE_WIRE_MODE) ?  ul : 1'bz;
assign ll_out = 	(mode > `ONE_WIRE_MODE) ?  ll : 1'bz;

// Cable detection address starts on slot + 7 so we can address other uart on slot
assign OW_ID = 	 (mode > `DISABLED_MODE && uart_slot_en == (DEV_ID + 7) && tx_slot == 0) ?  tx_slot : 1'bz;
assign rx_slot = (mode > `DISABLED_MODE  && uart_slot_en == (DEV_ID + 7)) ?  OW_ID : 1'bz;

// ********** spi sdi sets **********
// Enable/disable servo card
always @ (posedge clk) begin
	if (!resetn) begin 
		mode <= `DISABLED_MODE;
	end else if (spi_cmd_r == `C_SET_ENABLE_SERVO_CARD && spi_addr_r == DEV_ID && spi_data_valid_r) begin
		mode <= spi_data_r[2:0];
	end else begin
		mode <= mode;
	end	
end

// phase
always @ (posedge clk) begin
	if (!resetn) begin
		Phase_r <= 0;
	end else if (spi_cmd_r == `C_SET_SERVO_CARD_PHASE && spi_addr_r == DEV_ID && spi_data_valid_r) begin
		Phase_r <= spi_data_r[0];
	end else begin
		Phase_r <= Phase_r;
	end	
end

endmodule
