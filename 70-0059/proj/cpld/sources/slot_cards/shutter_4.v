`include "commands.v"

module shutter#(
	parameter DEV_ID				= 0,
	parameter UART_ADDRESS_WIDTH	= 0
)
(
	input wire 						clk,				
	input wire 						resetn,

	// CPLD lines
	output wire						CS,				// C0	Chip select for MCP23S09
	output wire						IN_2_2,			// C1	Shuuter 3 input 2
	output wire						IN_2_3,			// C2	Shuuter 4 input 1
	output wire						IN_1_2,			// C3	Shuuter 1 input 2
	output wire						IN_1_3,			// C4	Shuuter 2 input 1
	inout wire						OW_ID,			// C5	in/out for one wire ID
	inout wire						IN_2_1,			// C6	Shuuter 3 input 1
	output wire						IN_2_4,			// C7 	Shuuter 4 input 2
	output wire						IN_1_1,			// C8	Shuuter 1 input 1
	output wire						IN_1_4,			// C9	Shuuter 2 input 2
	
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
	
	input wire	 						EM_STOP
);
localparam		SHUTTER_PWM_CNTR_WIDTH			= 12;
localparam      SHUTTER_INIT_PERIOD	= 1900;


reg [2:0] mode;			// card mode or disabled or enabled


reg Phase_1_r;
reg Phase_2_r;
reg Phase_3_r;
reg Phase_4_r;

reg pwm_out_1;
reg pwm_out_2;
reg pwm_out_3;
reg pwm_out_4;

reg [SHUTTER_PWM_CNTR_WIDTH-1:0] 				pwm_duty_1;
reg [SHUTTER_PWM_CNTR_WIDTH-1:0] 				pwm_duty_2;
reg [SHUTTER_PWM_CNTR_WIDTH-1:0] 				pwm_duty_3;
reg [SHUTTER_PWM_CNTR_WIDTH-1:0] 				pwm_duty_4;

reg [SHUTTER_PWM_CNTR_WIDTH-1:0] 				pwm_freq_cntr;
reg [SHUTTER_PWM_CNTR_WIDTH-1:0]               pwm_period;
reg [SHUTTER_PWM_CNTR_WIDTH-1:0]               pwm_pending_period;



// All output must be tristated when system is reset and any output must be set is
// the card is enabled in a particular slot
// setup outputs to enable if module is enabled otherwise tristate
assign CS = 		(mode > `ONE_WIRE_MODE) ?  cs_decoded_in : 1'bz;
assign IN_2_2 = 	(mode > `ONE_WIRE_MODE) ?  (Phase_3_r ? 0 : pwm_out_3) : 1'bz;
assign IN_2_3 = 	(mode > `ONE_WIRE_MODE) ?  (Phase_4_r ? pwm_out_4 : 0) : 1'bz;
assign IN_1_2 = 	(mode > `ONE_WIRE_MODE) ?  (Phase_1_r ? 0 : pwm_out_1) : 1'bz;
assign IN_1_3 = 	(mode > `ONE_WIRE_MODE) ?  (Phase_2_r ? pwm_out_2 : 0) : 1'bz;
assign IN_2_1 = 	(mode > `ONE_WIRE_MODE) ?  (Phase_3_r ? pwm_out_3 : 0) : 1'bz;
assign IN_2_4 = 	(mode > `ONE_WIRE_MODE) ?  (Phase_4_r ? 0 : pwm_out_4) : 1'bz;
assign IN_1_1 = 	(mode > `ONE_WIRE_MODE) ?  (Phase_1_r ? pwm_out_1 : 0) : 1'bz;
assign IN_1_4 = 	(mode > `ONE_WIRE_MODE) ?  (Phase_2_r ? 0 : pwm_out_2) : 1'bz;

assign OW_ID = 	(mode > `DISABLED_MODE && uart_slot_en == (DEV_ID + 7) && tx_slot == 0) ?  tx_slot : 1'bz;
assign rx_slot = (mode > `DISABLED_MODE  && uart_slot_en == (DEV_ID + 7)) ?  OW_ID : 1'bz;

assign has_command = spi_addr_r == DEV_ID && spi_data_valid_r;

// ********** spi sdi sets **********
// Enable/disable servo card
always @ (posedge clk) begin
	if (!resetn) begin
		mode <= `DISABLED_MODE;
	end else if (has_command && spi_cmd_r == `C_SET_ENABLE_SHUTTER_CARD) begin
		mode <= spi_data_r[2:0];
	end else begin
		mode <= mode;
	end	
end

always @ (posedge clk) begin
	if (!resetn) begin
		pwm_duty_1 			<= 'b0;
		pwm_duty_2 			<= 'b0;
		pwm_duty_3 			<= 'b0;
		pwm_duty_4 			<= 'b0;
	end else if (has_command && spi_cmd_r == `C_SET_SUTTER_PWM_DUTY) begin
		if(spi_data_r[17:16] == 0)
			pwm_duty_1 			<= spi_data_r[SHUTTER_PWM_CNTR_WIDTH-1:0];
		else if(spi_data_r[17:16] == 1)
			pwm_duty_2 			<= spi_data_r[SHUTTER_PWM_CNTR_WIDTH-1:0];
		else if(spi_data_r[17:16] == 2)
			pwm_duty_3 			<= spi_data_r[SHUTTER_PWM_CNTR_WIDTH-1:0];
		else
			pwm_duty_4 			<= spi_data_r[SHUTTER_PWM_CNTR_WIDTH-1:0];
	end else begin
		pwm_duty_1 			<= pwm_duty_1;
		pwm_duty_2 			<= pwm_duty_2;
		pwm_duty_3 			<= pwm_duty_3;
		pwm_duty_4 			<= pwm_duty_4;
	end	
end

// phase
always @ (posedge clk) begin
	if (!resetn) begin
		Phase_1_r <= 0;
		Phase_2_r <= 0;
		Phase_3_r <= 0;
		Phase_4_r <= 0;
	end else if (has_command && spi_cmd_r == `C_SET_SUTTER_PHASE_DUTY) begin
		if(spi_data_r[2:1] == 0)
			Phase_1_r <= spi_data_r[0];
		else if(spi_data_r[2:1] == 1)
			Phase_2_r <= spi_data_r[0];
		else if(spi_data_r[2:1] == 2)
			Phase_3_r <= spi_data_r[0];
		else
			Phase_4_r <= spi_data_r[0];
	end else begin
		Phase_1_r <= Phase_1_r;
		Phase_2_r <= Phase_2_r;
		Phase_3_r <= Phase_3_r;
		Phase_4_r <= Phase_4_r;
	end
end

// 38000000 cycles/second
// 38000000 / 1900 => 20 kHz
always @ (posedge clk) begin
	if (!resetn  || EM_STOP)
		pwm_pending_period <= SHUTTER_INIT_PERIOD;
	else if (spi_cmd_r == `C_SET_SUTTER_PWM_PERIOD && spi_addr_r == DEV_ID && spi_data_valid_r)
        pwm_pending_period <= spi_data_r[SHUTTER_PWM_CNTR_WIDTH-1:0];
    else
        pwm_pending_period <= pwm_pending_period;
end

// pwm_freq_cntr
always @ (posedge clk) begin
	if (!resetn || EM_STOP) begin 
		pwm_freq_cntr 	<= 'b0;
		
		// Only reset this on reset.
		// Otherwise, debugging can reset this.
		if (!resetn)
			pwm_period 		<= SHUTTER_INIT_PERIOD;
		else
			pwm_period		<= pwm_period;
		pwm_out_1		<= 1'b0;
		pwm_out_2		<= 1'b0;
		pwm_out_3		<= 1'b0;
		pwm_out_4		<= 1'b0;
	end	
	else begin
		if (pwm_freq_cntr == 0 || pwm_freq_cntr == pwm_period) begin	// 1900  => 20kHz
			pwm_freq_cntr 	<= 'b1;
			pwm_period <= pwm_pending_period;
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
				
			if (pwm_duty_4 == 'b0)
				pwm_out_4			<= 1'b0;
			else
				pwm_out_4			<= 1'b1;
				
		end	
		else begin
			if (pwm_freq_cntr == pwm_duty_1) begin
				pwm_out_1			<= 1'b0;	
			end else begin
				pwm_out_1			<= pwm_out_1;	
			end
			if (pwm_freq_cntr == pwm_duty_2) begin
				pwm_out_2			<= 1'b0;	
			end else begin
				pwm_out_2			<= pwm_out_2;	
			end
			if (pwm_freq_cntr == pwm_duty_3) begin
				pwm_out_3			<= 1'b0;	
			end else begin
				pwm_out_3			<= pwm_out_3;	
			end
			if (pwm_freq_cntr == pwm_duty_4) begin
				pwm_out_4			<= 1'b0;	
			end else begin
				pwm_out_4			<= pwm_out_4;	
			end
			pwm_freq_cntr 	<= pwm_freq_cntr + 1'b1;
			pwm_period <= pwm_period;
		end	
	end	
end	

endmodule
