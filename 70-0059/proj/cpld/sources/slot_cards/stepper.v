
`include "commands.v"

module stepper#(
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
	output wire						SSI_SEL,		// C7	NSL Used for magnetic encoder on PCB 41-0113
	input wire						ENC_I,			// C8	Quad CHA encoder (input), Data for BISS & SSI (input) 
	inout wire						ENC_O,			// C9	Quad CHB encoder (input), Clock for BISS & SSI (output) 
	
	// SPI
	// used for sdi
	input wire 	[15:0]				spi_cmd_r,			
	input wire 	[7:0]				spi_addr_r,			
	input wire 	[39:0]				spi_data_r,			
	input wire 						spi_data_valid_r,	
	
	// used for sd0
	input wire 	[15:0]				spi_cmd,			
	input wire  [7:0]				spi_addr,			
	output reg 	[39:0]				spi_data_out_r,			
		
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

reg [2:0] mode;			// card mode or disabled or enabled
reg reset_r;
reg OPTO_PWR = 1'b1;
reg digital_output_r;
reg MA_Temp;
reg NSL = 0;
wire MA;	//Clock for Biss and SSI 
wire NSL_test;
reg [11:0] Cnt_NSL;
reg [7:0] Cnt;
reg [51:0] SLO; //Data for Biss and SSI 
reg [51:0] SLO_buf ;
//reg reset_nsl;

// All output must be tristated when system is reset and any output must be set is
// the card is enabled in a particular slot
// setup outputs to enable if module is enabled otherwise tristate

assign CS = 		(mode == `STEP_MODE_MAG) ?   OPTO_PWR : 1'bz;
assign RESET = 		(mode > `ONE_WIRE_MODE) ?  reset_r : 1'bz;
assign ul_out = 	(mode > `ONE_WIRE_MODE) ?  ul : 1'bz;
assign ll_out = 	(mode > `ONE_WIRE_MODE) ?  ll : 1'bz;
assign index_out = 	(mode > `ONE_WIRE_MODE) ?  index : 1'bz;
assign DRV_CS = 	(mode > `ONE_WIRE_MODE) ?  cs_decoded_in : 1'bz;             
assign SSI_SEL =   (mode == `STEP_MODE_MAG) ?  NSL_test : 1'bz;                //Disconnected SSI_SEL to use for Fagor input data

assign quad_a_out = (mode == `ENABLED_MODE) ?  ENC_I : 1'bz;
assign quad_b_out = (mode == `ENABLED_MODE) ?  ENC_O : 1'bz;
assign ENC_O =  	(mode > `DIGITAL_OUTPUT) ?  MA : 1'bz ;

// Cable detection address starts on slot + 7 so we can address other uart on slot
// 4:1 multiplexer using conditional logic
/*
assign OW_ID = (mode == `ENABLED_MODE && uart_slot_en == (DEV_ID + 7) && tx_slot == 0) ?  tx_slot : 1'bz;
assign OW_ID = 	(mode == `ENABLED_MODE && uart_slot_en == (DEV_ID + 7) && tx_slot == 0) ?  tx_slot : 
				(mode == `DIGITAL_OUTPUT) ?  0 :
*/

// assign OW_ID = (mode == `ENABLED_MODE && uart_slot_en == (DEV_ID + 7) && tx_slot == 0) ?  tx_slot :
//			   (mode == `DIGITAL_OUTPUT) ?  digital_output_r : 1'bz ;	  
assign OW_ID = (mode == `DIGITAL_OUTPUT) ?  digital_output_r :
				(mode > `DISABLED_MODE && uart_slot_en == (DEV_ID + 7) && tx_slot == 0) ? tx_slot : 1'bz;
assign rx_slot = (mode > `DISABLED_MODE  && uart_slot_en == (DEV_ID + 7)) ?  OW_ID : 1'bz;

always @ (posedge clk_1MHz) begin
	if (!resetn) begin
			Cnt <= 0;
			MA_Temp <= 0;
			Cnt_NSL <= 0;
	end else if (Cnt_NSL < 3200) begin                        //NSL hold high for at least 3 ms after power up
			NSL <= 1'b1;
			Cnt_NSL <= Cnt_NSL + 1;
	end else if (Cnt <= 18 && mode == `STEP_MODE_MAG) begin   //SSI Protocol Counter for 21 Pulses for magnetic encoder 
			Cnt <= Cnt+1;
			if (Cnt == 18) begin 
					MA_Temp <= ~MA_Temp;
					NSL <= ~NSL;
			end
	end else if (Cnt < 51 && mode == `STEP_MODE_ENC_BISS) begin  //Biss Protocol Counter for 50 Pulses
			Cnt <= Cnt+1;
			if (Cnt == 50) begin
			        MA_Temp <= ~MA_Temp;
			end
	end else begin
			Cnt <= 0;
	end
end

// amir
assign MA = !(MA_Temp & clk_1MHz); //Clock for Biss and SSI 
assign NSL_test = NSL;
always @ (negedge MA_Temp) begin
		    SLO_buf <= SLO;////24'h3c2b1a; //
end

always @ (posedge MA) begin
	//if (Cnt && mode == `STEP_MODE_MAG) begin
	if (mode == `STEP_MODE_MAG) begin
		SLO <= { SLO[50:0],ENC_I } & 32'h000fffff;//24'h3c2b1a; //24'h1ffff0{ SLO[50:0],ENC_I };
	end else if (Cnt && mode == `STEP_MODE_ENC_BISS)  begin
	    SLO <= { SLO[50:0],ENC_I };
	end
end

/* eric
assign MA = !(MA_Temp & clk_1MHz); //Clock for Biss and SSI 
assign NSL_test = NSL;
reg prev_MA_Temp;
always @(posedge clk) begin
	if(prev_MA_Temp == 1 && MA_Temp == 0) begin //it's a falling edge of MA_Temp
		SLO_buf <= SLO;////24'h3c2b1a; //
	end
	prev_MA_Temp <= MA_Temp;
end

reg prev_MA;
always @ (posedge clk) begin
	if(prev_MA == 0 && MA == 1) begin  //rising edge of MA
		if (mode == `STEP_MODE_MAG) begin
			SLO <= { SLO[50:0],ENC_I } & 32'h000fffff;//24'h3c2b1a; //24'h1ffff0{ SLO[50:0],ENC_I };
		end else if (Cnt && mode == `STEP_MODE_ENC_BISS)  begin
			SLO <= { SLO[50:0],ENC_I };
		end
	end
	prev_MA <= MA;
end
*/

// ********** spi sdi sets **********
// Enable/disable stepper card
always @ (posedge clk) begin
	if (!resetn) begin
		mode <= `DISABLED_MODE;
		reset_r <= 0;
		digital_output_r <= 0;
	//	reset_nsl <= 0;
	end else if (spi_cmd_r == `C_SET_ENABLE_STEPPER_CARD && spi_addr_r == DEV_ID && spi_data_valid_r) begin 
		mode <= spi_data_r[2:0];
		reset_r <= 1;
		digital_output_r <= digital_output_r;
	//	reset_nsl <= reset_nsl;
	end else if (spi_cmd_r == `C_SET_STEPPER_DIGITAL_OUTPUT && spi_addr_r == DEV_ID && spi_data_valid_r) begin
		mode <= mode;
		digital_output_r <= spi_data_r[0];
		reset_r <= reset_r;
	//	reset_nsl <= reset_nsl;
//	end else if (spi_cmd_r == `C_SET_STEPPER_ENC_NSL_DI && spi_addr_r == DEV_ID && spi_data_valid_r) begin
//		mode <= mode;
//		digital_output_r <= spi_data_r[0];
//		reset_r <= reset_r;
//		reset_nsl <= spi_data_r[0];
	end else if (EM_STOP) begin
		mode <= mode;
		reset_r <= 0;
		digital_output_r <= digital_output_r;
	//	reset_nsl <= reset_nsl;
	end else begin
		mode <= mode;
		reset_r <= reset_r;
		digital_output_r <= digital_output_r;
	//	reset_nsl <= reset_nsl;
	end	
end


// ********** spi sdo sets **********
always @ (posedge clk) begin
	if (!resetn) begin 
		spi_data_out_r	 <= 'bz;
	end else if (spi_cmd == `C_READ_BISS_ENC && spi_addr == DEV_ID) begin// && mode == `STEP_MODE_ENC_BISS) 
		spi_data_out_r 	<= {SLO_buf[13:6],SLO_buf[45:14]}; //40'hf0aaaaaaaa
	end else if (spi_cmd == `C_READ_SSI_MAG && spi_addr == DEV_ID ) begin//&& mode == `STEP_MODE_MAG) 
		spi_data_out_r	<= {4'b0,SLO_buf[3:0],16'b0,SLO_buf[19:4]}; //40'h3c2b1a; //
	end else begin   
		spi_data_out_r	<= 'bz;
	end
end


endmodule

