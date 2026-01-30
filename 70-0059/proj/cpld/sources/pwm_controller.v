`include "commands.v"
// Company: 		Thorlabs Imaging, Sterling, VA
// Engineer: 		ZS
// Create Date: 	01/29/2018
// Module Name: 	pwm_controller
// Description:     Accepts input pwm_freq and pwm_duty as counts.
//					e.g. divide-by-2, 50% duty cycle: pwm_freq = 2, pwm_duty = 1
//					e.g. divide-by-4, 50% duty cycle: pwm_freq = 4, pwm_duty = 2
//					e.g. divide-by-10, 50% duty cycle: pwm_freq = 10, pwm_duty = 5

module pwm_controller#(
	parameter DEV_ID				= 0,
	parameter PWM_CNTR_WIDTH 		= 12
)
(
	input wire 							clk,					// 100 MHz //! Clock is actually 38 MHz.  Should I use the 100 MHz clock line?
	input wire 							resetn,							
	output reg	 						pwm_out,
		
	// SPI
	// used for sdi
	input wire 	[15:0]					spi_cmd_r,			
	input wire 	[7:0]					spi_addr_r,			
	input wire 	[39:0]					spi_data_r,			
	input wire 							spi_data_valid_r
);

reg [PWM_CNTR_WIDTH-1:0] 				pwm_freq;
reg [PWM_CNTR_WIDTH-1:0] 				pwm_duty;
reg [PWM_CNTR_WIDTH-1:0] 				pwm_freq_cntr;		

// spi sets
// ********** spi sdi sets **********
always @ (posedge clk) begin
	if (!resetn)
		pwm_freq 			<= 'b0;
	else if (spi_cmd_r == `C_SET_PWM_FREQ && spi_addr_r == DEV_ID && spi_data_valid_r) 
		pwm_freq 			<= spi_data_r[PWM_CNTR_WIDTH-1:0];
	else	
		pwm_freq 			<= pwm_freq;
end

always @ (posedge clk) begin
	if (!resetn)
		pwm_duty 			<= 'b0;
	else if (spi_cmd_r == `C_SET_PWM_DUTY && spi_addr_r == DEV_ID && spi_data_valid_r) 
		pwm_duty 			<= spi_data_r[PWM_CNTR_WIDTH-1:0];
	else	
		pwm_duty 			<= pwm_duty;
end

// pwm_freq_cntr
always @ (posedge clk) begin
	if (!resetn) begin 
		pwm_freq_cntr 	<= 'b0;
		pwm_out			<= 1'b0;
	end	
	else begin
		if (pwm_freq_cntr == pwm_freq - 1) begin
			pwm_freq_cntr 	<= 'b0;
			if (pwm_duty == 'b0)
				pwm_out			<= 1'b0;
			else
				pwm_out			<= 1'b1;
		end
		else if (pwm_freq_cntr == pwm_duty - 1) begin
			pwm_freq_cntr 	<= pwm_freq_cntr + 1'b1;
			pwm_out			<= 1'b0;	
		end
		else begin
			pwm_freq_cntr 	<= pwm_freq_cntr + 1'b1;
			pwm_out			<= pwm_out;	
		end	
	end	
end	

endmodule