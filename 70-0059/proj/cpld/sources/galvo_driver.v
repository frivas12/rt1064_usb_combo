`include "commands.v"

module galvo_driver#(
	parameter DEV_ID			= 0
)
(
	input wire 								clk,				// 100 MHz
	input wire 								resetn,
	// slot pins
	output reg	 							ldac,
	output reg	 							spi_clk,
	input wire 								spi_miso,
	output reg	 							spi_mosi,
	output reg	 							toggle,
	output reg	 							cs,
	input wire 								over_temp,

	input wire 	[7:0]						spi_cmd_r,			
	input wire 								spi_cmd_valid_r,	
	input wire 	[7:0]						spi_addr_r,			
	input wire 								spi_addr_valid_r,	
	input wire 	[47:0]						spi_data_r,			
	input wire 								spi_data_valid_r,	
	input wire 								spi_done			
);