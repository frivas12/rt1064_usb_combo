`include "commands.v"

module Mag_Sensor#(
	parameter DEV_ID				= 0,
	parameter UART_ADDRESS_WIDTH	= 0
)
(
	input wire 						clk,				
	input wire 						resetn,
	input wire 						clk_1MHz,	

	output wire						CS,				// C0	Used for magnetic encoder on PCB 41-0113 and BISS encoders
	output wire						RESET,			// C1	out L6470HTR stepper drive reset (low shuts down the drive)
	input wire						ul,				// C2	in (limit is high when on the limit)
	input wire						ll,				// C3	in (limit is high when on the limit)
	input wire						index,			// C4	in	
	inout wire						OW_ID,			// C5	in/out for one wire ID
	output wire						DRV_CS,			// C6 	(CS 1 on slot)
	output wire						SSI_SEL,		// C7	Used for magnetic encoder on PCB 41-0113
	//input wire						SSI_SEL, 		// C7   Used temprory for Fagor input data
	//input wire						quad_a,			// C8	in
	//input wire						quad_b,			// C9	in
	//output wire						quad_b,			// C9  Used temprory for Fagor output NSL
	input wire						quad_a,			// C8  Used temprory for Fagor input data
	output wire						NSL_test_tb,	
	
	// SPI
	// used for sdi
	input wire 	[15:0]				spi_cmd_r,			
	input wire 	[7:0]				spi_addr_r,			
	input wire 	[31:0]				spi_data_r,			
	input wire 						spi_data_valid_r,	
	
	// used for sd0
	input wire 	[15:0]				spi_cmd,			
	input wire [7:0]				spi_addr,			
	output reg 	[31:0]				spi_data_out_r,			
		
	// between modules
	input wire 							cs_decoded_in,    //removed to connect countinuse 1MHz clock for Biss Encoder
	input wire 							cs_decoded_in2,
	output wire 						quad_a_out,
	output wire 						quad_b_out,
	output wire 						ul_out,
	output wire 						ll_out,
	output wire 						index_out,

	// uart for one wire
	input wire [UART_ADDRESS_WIDTH-1:0]	uart_slot_en,		
	output wire							rx_slot,
	input wire 							tx_slot,

	input wire	 						EM_STOP
);

reg [1:0] mode;			// card mode or disabled or enabled
reg reset_r;
reg ssi_sel_r = 1'b1;
reg digital_output_r;
reg MA_Temp;
reg NSL;
wire MA;
wire NSL_test;
reg [11:0] Cnt_NSL;
reg [7:0] Cnt;
reg [20:0] SLO;
reg [20:0] SLO_buf;

// All output must be tristated when system is reset and any output must be set is
// the card is enabled in a particular slot
// setup outputs to enable if module is enabled otherwise tristate

//assign CS = 		(mode > `DISABLED_MODE) ?  cs_decoded_in2 : 1'bz;            //removed to connect countinuse 1MHz clock for Biss Encoder
//assign CS = 		(mode > `DISABLED_MODE) ?   MA : 1'bz;
assign RESET = 		(mode > `DISABLED_MODE) ?  reset_r : 1'bz;
assign ul_out = 	(mode > `DISABLED_MODE) ?  ul : 1'bz;
assign ll_out = 	(mode > `DISABLED_MODE) ?  ll : 1'bz;
assign index_out = 	(mode > `DISABLED_MODE) ?  index : 1'bz;
assign DRV_CS = 	(mode > `DISABLED_MODE) ?  cs_decoded_in : 1'bz;             
assign SSI_SEL = 	(mode > `DISABLED_MODE) ?  NSL_test : 1'bz;                //Disconnected SSI_SEL to use for Fagor input data
//assign SSI_SEL = ssi_sel_r; 
assign quad_a_out = (mode > `DISABLED_MODE) ?  quad_a : 1'bz;
assign quad_b_out = (mode > `DISABLED_MODE) ?  MA : 1'bz;
//assign NSL_test_tb = (mode > `DISABLED_MODE) ?  NSL_test : 1'bz;					 //Disconnected quad_b to use for Meg sensor input data


// Cable detection address starts on slot + 7 so we can address other uart on slot
// 4:1 multiplexer using conditional logic
/*
assign OW_ID = (mode == `ENABLED_MODE && uart_slot_en == (DEV_ID + 7) && tx_slot == 0) ?  tx_slot : 1'bz;


assign OW_ID = 	(mode == `ENABLED_MODE && uart_slot_en == (DEV_ID + 7) && tx_slot == 0) ?  tx_slot : 
				(mode == `DIGITAL_OUTPUT) ?  0 :
*/

assign OW_ID = (mode == `ENABLED_MODE && uart_slot_en == (DEV_ID + 7) && tx_slot == 0) ?  tx_slot :			   (mode == `DIGITAL_OUTPUT) ?  digital_output_r : 1'bz ;	  
assign rx_slot = (mode > `DISABLED_MODE  && uart_slot_en == (DEV_ID + 7)) ?  OW_ID : 1'bz;





/*
wire W_OW_ID;
assign W_OW_ID = (mode == `ENABLED_MODE && uart_slot_en == (DEV_ID + 7) && tx_slot == 0) ?  tx_slot : 1'bz;

//assign OW_ID = (mode > `ENABLED_MODE) ? 1 :
 //     (mode > `DISABLED_MODE) ? W_OW_ID : 1'bz ;


assign OW_ID = (mode==`DISABLED_MODE) ? 1'bz :
      ((mode==2'b01 && uart_slot_en == (DEV_ID + 7) && tx_slot == 0)) ? tx_slot :
     // (mode==`ENABLED_MODE) ? W_OW_ID :
      (mode==`DIGITAL_OUTPUT) ? 1 :
      1'bz ;	  

// was original assign OW_ID = 	(mode == `ENABLED_MODE && uart_slot_en == (DEV_ID + 7) && tx_slot == 0) ?  tx_slot : 1'bz;
//assign OW_ID = 	(mode == `DIGITAL_OUTPUT) ?  digital_output_r : 1'bz;
//assign OW_ID = 	(mode > `DISABLED_MODE) ?  1 : 1'bz; // TODO  this was set for the opto switch power on need to fix
assign rx_slot = (mode > `DISABLED_MODE  && uart_slot_en == (DEV_ID + 7)) ?  OW_ID : 1'bz;
//assign rx_slot = (mode==2'b01  && uart_slot_en == (DEV_ID + 7)) ?  OW_ID : 1'bz;
*/
//********** Biss  interface **********
//always @ (posedge clk_1MHz) begin
	//if (!resetn) begin
		//Cnt <= 0;
		//MA_Temp <= 0;
	//end else if (Cnt == 50) begin
		
		//Cnt <= 0;
		//MA_Temp <= ~MA_Temp;
	//end else begin
		//Cnt <= Cnt+1;
	//end	
//end

//assign MA = !(MA_Temp & clk_1MHz);

//always @ (negedge MA_Temp) begin
	//SLO_buf <= SLO;
//end

//always @ (posedge MA) begin
	//if (Cnt==0) begin
		//SLO <= 0;
	//end else begin
	    //SLO <= { SLO[44:0],quad_b };
	//end
//end
//
//********** SSI  interface **********
always @ (posedge clk_1MHz) begin
	if (!resetn) begin
		Cnt <= 0;
		MA_Temp <= 0;
	//end else if (ssi_sel_r) begin
	end else if (Cnt_NSL < 3200) begin
		NSL <= 1'b1;
		Cnt_NSL <= Cnt_NSL + 1;
	end else if (Cnt < 24) begin
		Cnt <= Cnt+1;
		if (Cnt == 1) begin
			NSL <= 1'b0;
		end else if (Cnt > 1 & Cnt <23) begin
			MA_Temp <= 1'b1;
		end else begin 
			MA_Temp <= 1'b0;
		end
	end else begin
		NSL <= 1'b1;
		Cnt <= 0;
	end
end

assign MA = !(MA_Temp & clk_1MHz);
assign NSL_test = NSL;
//always @ (negedge MA_Temp) begin
	//SLO_buf <= SLO;
//end

always @ (posedge MA) begin
	if (Cnt==0) begin
		SLO <= 0;
	end else begin
	    SLO <= { SLO[19:0],quad_a };
	end
end

// ********** spi sdi sets **********
// Enable/disable stepper card
always @ (posedge clk) begin
	if (!resetn) begin
		mode <= `DISABLED_MODE;
		reset_r <= 0;
		digital_output_r <= 0;
	end else if (spi_cmd_r == `C_SET_ENABLE_STEPPER_CARD && spi_addr_r == DEV_ID && spi_data_valid_r) begin
		mode <= spi_data_r[1:0];
		reset_r <= 1;
		digital_output_r <= digital_output_r;
	end else if (spi_cmd_r == `C_SET_STEPPER_DIGITAL_OUTPUT && spi_addr_r == DEV_ID && spi_data_valid_r) begin
		mode <= mode;
		digital_output_r <= spi_data_r[0];
		reset_r <= reset_r;
	end else if (EM_STOP) begin
		mode <= mode;
		reset_r <= 0;
		digital_output_r <= digital_output_r;
	end else begin
		mode <= mode;
		reset_r <= reset_r;
		digital_output_r <= digital_output_r;
	end	
end

// ********** spi sdo sets **********
always @ (posedge clk) begin
	if (!resetn) 
		spi_data_out_r 			<= 'bz;
		ssi_sel_r              <=  0;
	else if (spi_cmd == `C_READ_Mag_Sensor && spi_addr == DEV_ID) 
			spi_data_out_r 		<= SLO_buf; //40'h140d0c0b0a
			ssi_sel_r <= spi_data_r[0];
	else	
		spi_data_out_r 			<= 'bz;
		ssi_sel_r <= ssi_sel_r;
end

// Inverted mag sensor card NSL_DI
//always @ (posedge clk) begin
	//if (!resetn) begin 
		//ssi_sel_r <= 0;
	//end else if (spi_cmd_r == `C_SET_STEPPER_ENC_NSL_DI && spi_addr_r == DEV_ID && spi_data_valid_r) begin
		////ssi_sel_r <= spi_data_r[0];
	//end else begin
		////ssi_sel_r <= ssi_sel_r;
	//end	
//end

endmodule