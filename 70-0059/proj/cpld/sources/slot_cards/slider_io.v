`include "commands.v"

// problem with cable detect on slot 7 

module io#(
	parameter DEV_ID				= 0,
	parameter UART_ADDRESS_WIDTH	= 0
)
(
	input wire 						clk,				
	input wire 						clk_100k,				
	input wire 						resetn,

	// CPLD lines
	output wire						CS,				// C0	Chip select for MCP23S09
	output wire						LV_OUT_5,		// C1	not used
	output wire						LV_OUT_1,		// C2	not used
	output wire						LV_OUT_3,		// C3	not used
	output wire						LV_OUT_4,		// C4	not used 
	inout wire						OW_ID,			// C5	in/out for one wire ID
	output wire						LV_OUT_2,		// C6	not used
	output wire						LV_OUT_0,		// C7 	not used
	inout wire						LV_IO_4,		// C8	not used
	inout wire						LV_IO_5,		// C9	not used
	
	// SPI
	// used for sdi
	input wire 	[15:0]				spi_cmd_r,			
	input wire 	[7:0]				spi_addr_r,			
	input wire 	[39:0]				spi_data_r,			
	input wire 						spi_data_valid_r,	
	
	// between modules
	input wire 						cs_decoded_in,
	
	// uart for one wire
	input wire [UART_ADDRESS_WIDTH-1:0]	uart_slot_en,		
	output wire							rx_slot,
	input wire 							tx_slot,

	input wire	 						EM_STOP //(not needed for now)
);
localparam		IO_PWM_CNTR_WIDTH			= 12;			

reg [2:0] mode;			// card mode or disabled or enabled

reg pwm_out_1;
reg pwm_out_2;
reg pwm_out_3;

reg [IO_PWM_CNTR_WIDTH-1:0] 				pwm_duty_1;
reg [IO_PWM_CNTR_WIDTH-1:0] 				pwm_duty_2;
reg [IO_PWM_CNTR_WIDTH-1:0] 				pwm_duty_3;

reg [IO_PWM_CNTR_WIDTH-1:0] 				pwm_freq_cntr;		

// All output must be tristated when system is reset and any output must be set is
// the card is enabled in a particular slot
// setup outputs to enable if module is enabled otherwise tristate
assign CS = 		(mode > `DISABLED_MODE) ?  cs_decoded_in : 1'bz;

//assign LV_OUT_0 = 	(mode > `DISABLED_MODE) ?  pwm_out_1 : 1'bz;
//assign LV_OUT_1 = 	(mode > `DISABLED_MODE) ?  1'd1 : 1'bz;
//assign LV_OUT_2 = 	(mode > `DISABLED_MODE) ?  1'd1 : 1'bz;
//assign LV_OUT_3 = 	(mode > `DISABLED_MODE) ?  1'd1 : 1'bz;
//assign LV_OUT_4 = 	(mode > `DISABLED_MODE) ?  1'd1 : 1'bz;
//assign LV_OUT_5 = 	(mode > `DISABLED_MODE) ?  1'd1 : 1'bz;

// bidirectional
//assign LV_IO_4 = 	(mode > `DISABLED_MODE) ?  1'd1 : 1'bz;
//assign LV_IO_5 = 	(mode > `DISABLED_MODE) ?  1'd1 : 1'bz;

// Cable detection address starts on slot + 7 so we can address other uart on slot
assign OW_ID = 	 (mode > `DISABLED_MODE && uart_slot_en == (DEV_ID + 7) && tx_slot == 0) ?  tx_slot : 1'bz;
assign rx_slot = (mode > `DISABLED_MODE  && uart_slot_en == (DEV_ID + 7)) ?  OW_ID : 1'bz;

// ********** spi sdi sets **********
// Enable/disable servo card

always @ (posedge clk) begin
	if (!resetn) begin
		mode <= `DISABLED_MODE;
	end else if (spi_cmd_r == `C_SET_ENABLE_IO_CARD && spi_addr_r == DEV_ID && spi_data_valid_r) begin
		mode <= spi_data_r[2:0];
	end else begin
		mode <= mode;
	end	
end


// comment out code below because it’s for controlling hobby servo directly but we are not using now

// PWM for hobby servo 
// period 20ms
// pulse 1 - 2 ms, 1.5ms center
always @ (posedge clk) begin
	if (!resetn) begin
		pwm_duty_1 			<= 100;//'b0;
		pwm_duty_2 			<= 'b0;
		pwm_duty_3 			<= 'b0;
	end else if (spi_cmd_r == `C_SET_IO_PWM_DUTY && spi_addr_r == DEV_ID && spi_data_valid_r) begin
		if(spi_data_r[17:16] == 0)
			pwm_duty_1 			<= spi_data_r[IO_PWM_CNTR_WIDTH-1:0];
		else if(spi_data_r[17:16] == 1)
			pwm_duty_2 			<= spi_data_r[IO_PWM_CNTR_WIDTH-1:0];
		else
			pwm_duty_3 			<= spi_data_r[IO_PWM_CNTR_WIDTH-1:0];
	end else begin
		pwm_duty_1 			<= pwm_duty_1;
		pwm_duty_2 			<= pwm_duty_2;
		pwm_duty_3 			<= pwm_duty_3;
	end	
end

// pwm_freq_cntr
always @ (posedge clk_100k) begin
	if (!resetn) begin 
		pwm_freq_cntr 	<= 'b0;
		pwm_out_1		<= 1'b0;
		pwm_out_2		<= 1'b0;
		pwm_out_3		<= 1'b0;
	end	
	else begin
		if (pwm_freq_cntr == 2000 - 1) begin	// 1900  => 20kHz
			pwm_freq_cntr 	<= 'b0;
			if (pwm_duty_1 == 'b0)
				pwm_out_1			<= 1'b0;
			else
				pwm_out_1			<= 1'b1;

			if (pwm_duty_2 == 'b0)
				pwm_out_2			<= 1'b0;
			else
				pwm_out_2			<= 1'b1;
				
			if (pwm_duty_3 == 'b0)
				pwm_out_3			<= 1'b0;
			else
				pwm_out_3			<= 1'b1;
				
		end	
		else begin
			if (pwm_freq_cntr == pwm_duty_1 - 1) begin
				pwm_freq_cntr 	<= pwm_freq_cntr + 1'b1;
				pwm_out_1			<= 1'b0;	
			end else begin
				pwm_out_1			<= pwm_out_1;	
			end
			if (pwm_freq_cntr == pwm_duty_2 - 1) begin
				pwm_freq_cntr 	<= pwm_freq_cntr + 1'b1;
				pwm_out_2			<= 1'b0;	
			end else begin
				pwm_out_2			<= pwm_out_2;	
			end
			if (pwm_freq_cntr == pwm_duty_3 - 1) begin
				pwm_freq_cntr 	<= pwm_freq_cntr + 1'b1;
				pwm_out_3			<= 1'b0;	
			end else begin
				pwm_out_3			<= pwm_out_3;	
			end
			pwm_freq_cntr 	<= pwm_freq_cntr + 1'b1;
		end	
	end	
end	

endmodule
