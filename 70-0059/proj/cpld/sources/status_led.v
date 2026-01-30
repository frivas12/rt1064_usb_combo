`include "commands.v"

// Emergency stop is activated when the uC stops running and/or the supervisior task doesn't 
// checkin with this module by setting PWM here.  The EM_STOP is passed to all modules and
// can be used to shut things down while EM_STOP is high.

module status_led#(
	parameter DEV_ID				= 0,
	parameter PWM_CNTR_WIDTH 		= 12
)
(
	input wire 							clk,					// 38 MHz
	input wire 							resetn,							
	output reg	 						pwm_out,
		
	// SPI
	// used for sdi
	input wire 	[15:0]					spi_cmd_r,			
	input wire 	[7:0]					spi_addr_r,			
	input wire 	[39:0]					spi_data_r,			
	input wire 							spi_data_valid_r,

	// used for sd0
	input wire 	[15:0]						spi_cmd,			
	input wire 	[7:0]						spi_addr,			
	output reg 	[39:0]						spi_data_out_r,			
		
	output reg	 						EM_STOP
);

localparam		BLINK_CNTR_WIDTH			= 13;			
localparam		BLINK_RATE					= 4000;		// 4000 = 5Hz		
localparam		EM_STOP_TIMEOUT_COUNTS		= 2000;

reg [PWM_CNTR_WIDTH-1:0] 				pwm_duty;
reg [PWM_CNTR_WIDTH-1:0] 				pwm_freq_cntr;		
reg [BLINK_CNTR_WIDTH-1:0] 				status_cntr;	
reg pwm;

always @ (posedge clk) begin
	if (!resetn) begin
		pwm_duty 			<= 'b0;
		pwm_out 			<= 'b0;
		EM_STOP 			<= 'b0;
	end	
	else begin
		// SPI set command
		if (spi_cmd_r == `C_SET_PWM_DUTY && spi_addr_r == DEV_ID && spi_data_valid_r) begin
			pwm_duty 		<= spi_data_r[PWM_CNTR_WIDTH-1:0];
			status_cntr <= 0;
		end	
		else begin	
			pwm_duty 		<= pwm_duty;
		end
			
		// pwm dim while no blink
		if (pwm_freq_cntr == 1900 - 1) begin
			pwm_freq_cntr 	<= 'b0;
			status_cntr <= status_cntr + 1;

			if (pwm_duty == 'b0)
				pwm			<= 1'b0;
			else
				pwm			<= 1'b1;
		end
		else if (pwm_freq_cntr == pwm_duty - 1) begin
			pwm_freq_cntr 	<= pwm_freq_cntr + 1'b1;
			pwm			<= 1'b0;	
		end
		else begin
			pwm_freq_cntr 	<= pwm_freq_cntr + 1'b1;
			pwm			<= pwm;	
		end	
		
		// blink because no set command recieved from uC  and set EM_STOP high
		if(status_cntr >= EM_STOP_TIMEOUT_COUNTS) begin
			if(status_cntr < (BLINK_RATE/2 + EM_STOP_TIMEOUT_COUNTS))
				pwm_out <= 1;
			else if(status_cntr < (BLINK_RATE + EM_STOP_TIMEOUT_COUNTS))
				pwm_out <= 0;
			else begin
				pwm_out <= pwm_out;
				status_cntr <= EM_STOP_TIMEOUT_COUNTS;
			end
			EM_STOP <= 1;
		end	
		else begin	// status ok just do solid color pwm
			pwm_out <= pwm;
			EM_STOP <= 0;
		end	
	end
end

reg em_stop_flag;

// ********** spi sdo sets **********
always @ (posedge clk) begin
	if (!resetn || EM_STOP) begin
//		reset_r <= 0;
		spi_data_out_r 			<= 'bz;
		em_stop_flag <= 1;
	end else if (spi_cmd == `C_READ_EM_STOP_FLAG && spi_addr == DEV_ID) begin
//		reset_r <= 1;
		spi_data_out_r 			<= em_stop_flag;
		em_stop_flag <= 0;
	end else begin
//		reset_r <= reset_r;
		spi_data_out_r 			<= 'bz;
		em_stop_flag			<= em_stop_flag;
	end	
end


endmodule