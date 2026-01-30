`include "commands.v"

module otm_dac#(
	parameter DEV_ID			= 0
)
(
	input wire 						clk,				
	input wire 						resetn,

	output wire						CS,				// C0	SPI chip select
	output wire						LASER_CNTRL,	// C7	
	input wire						IPG_B_REFL,		// C9	
	
	// SPI
	// used for sdi
	input wire 	[15:0]				spi_cmd_r,			
	input wire 	[7:0]				spi_addr_r,			
	input wire 	[39:0]				spi_data_r,			
	input wire 						spi_data_valid_r,	
	
	// between modules
	input wire 						cs_decoded_in
);

reg mode;			// card mode or disabled or enabled
reg LASER_CNTRL_r;

// All output must be tristated when system is reset and any output must be set is
// the card is enabled in a particular slot
// setup outputs to enable if module is enabled otherwise tristate
assign CS = 			(mode > `DISABLED_MODE) ?  cs_decoded_in : 1'bz;
assign LASER_CNTRL = 	(mode > `DISABLED_MODE) ?  LASER_CNTRL_r : 1'bz;

// ********** spi sdi sets **********
// Enable/disable stepper card
always @ (posedge clk) begin
	if (!resetn) begin
		mode <= `DISABLED_MODE;
	end else if (spi_cmd_r == `C_SET_ENABLE_OTM_DAC_CARD && spi_addr_r == DEV_ID && spi_data_valid_r) begin
		mode <= spi_data_r[0];
	end else begin
		mode <= mode;
	end	
end

// laser control on/off
always @ (posedge clk) begin
	if (!resetn) begin
		LASER_CNTRL_r <= 0;
	end else if (spi_cmd_r == `C_SET_OTM_DAC_LASER_EN && spi_addr_r == DEV_ID && spi_data_valid_r) begin
		LASER_CNTRL_r <= spi_data_r[0];
	end else begin
		LASER_CNTRL_r <= LASER_CNTRL_r;
	end	
end





endmodule