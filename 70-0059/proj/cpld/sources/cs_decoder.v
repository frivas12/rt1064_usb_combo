// Company: 		Thorlabs Imaging, Sterling, VA
// Engineer: 		ZS
// Create Date: 	01/23/2018
// Module Name: 	cs_decoder


module cs_decoder#(
	parameter NUM_SLOTS				= 7,
	parameter NUM_CS_PER_SLOT		= 2,
	parameter CS_IN_WIDTH			= 5
)
(
	input wire 											clk,					// 100 MHz
	input wire 											resetn,
	input wire 											CS_READY,	
	input wire 	[CS_IN_WIDTH-1:0]						cs,							
	output reg  [NUM_SLOTS*NUM_CS_PER_SLOT-1:0]   	cs_decoded,
	output reg  								   		FLASH_CS,
	output reg											MAX3421_CS
);





always @ (posedge CS_READY or negedge resetn) begin
	if (!resetn) begin
		cs_decoded 		<= 14'h3fff;
		FLASH_CS		<= 1'b1;
		MAX3421_CS		<= 1'b1;
	end	else begin
				case (cs)
			// cs all high
			5'h00:	begin 
						cs_decoded 		<= 14'h3fff;
						FLASH_CS		<= 1'b1;
						MAX3421_CS		<= 1'b1;
					end	
			// enable slot 1a
			5'h01:	cs_decoded[0] 	<= 0;
			// enable slot 1b		
			5'h02:	cs_decoded[1] 	<= 0;
			// enable slot 2a		
			5'h03:	cs_decoded[2] 	<= 0;
			// enable slot 2b		
			5'h04:	cs_decoded[3] 	<= 0;
			// enable slot 3a
			5'h05: 	cs_decoded[4] 	<= 0;
			// enable slot 3b
			5'h06:	cs_decoded[5] 	<= 0;
			// enable slot 4a
			5'h07:	cs_decoded[6] 	<= 0;
			// enable slot 4b
			5'h08:	cs_decoded[7] 	<= 0;
			// enable slot 5a
			5'h09:	cs_decoded[8] 	<= 0;
			// enable slot 5b
			5'h0a:	cs_decoded[9] 	<= 0;
			// enable slot 6a
			5'h0b:	cs_decoded[10] 	<= 0;
			// enable slot 6b
			5'h0c:	cs_decoded[11] 	<= 0;
			// enable slot 7a
			5'h0d:	cs_decoded[12] 	<= 0;
			// enable slot 7b
			5'h0e:	cs_decoded[13] 	<= 0;
			// enable flash
			5'h1d:	FLASH_CS	 	<= 0;
			// enable max3421
			5'h1e:	MAX3421_CS	 	<= 0;
			
			// default
			default:cs_decoded 		<= 14'h3fff;
		endcase
	end	
end

// cs decoding
// code below cause run away
/*
always @ (posedge clk) begin
	if (!resetn) begin
		cs_decoded 		<= 14'h3fff;
		FLASH_CS		<= 1'b1;
		MAX3421_CS		<= 1'b1;

	end	
	else if (CS_READY) begin
		case (cs)
			// cs all high
			5'h00:	begin 
						cs_decoded 		<= 14'h3fff;
						FLASH_CS		<= 1'b1;
						MAX3421_CS		<= 1'b1;
					end	
			// enable slot 1a
			5'h01:	cs_decoded[0] 	<= 0;
			// enable slot 1b		
			5'h02:	cs_decoded[1] 	<= 0;
			// enable slot 2a		
			5'h03:	cs_decoded[2] 	<= 0;
			// enable slot 2b		
			5'h04:	cs_decoded[3] 	<= 0;
			// enable slot 3a
			5'h05: 	cs_decoded[4] 	<= 0;
			// enable slot 3b
			5'h06:	cs_decoded[5] 	<= 0;
			// enable slot 4a
			5'h07:	cs_decoded[6] 	<= 0;
			// enable slot 4b
			5'h08:	cs_decoded[7] 	<= 0;
			// enable slot 5a
			5'h09:	cs_decoded[8] 	<= 0;
			// enable slot 5b
			5'h0a:	cs_decoded[9] 	<= 0;
			// enable slot 6a
			5'h0b:	cs_decoded[10] 	<= 0;
			// enable slot 6b
			5'h0c:	cs_decoded[11] 	<= 0;
			// enable slot 7a
			5'h0d:	cs_decoded[12] 	<= 0;
			// enable slot 7b
			5'h0e:	cs_decoded[13] 	<= 0;
			// enable flash
			5'h1d:	FLASH_CS	 	<= 0;
			// enable max3421
			5'h1e:	MAX3421_CS	 	<= 0;
			
			// default
			default:cs_decoded 		<= 14'h3fff;
		endcase
	end
	else
		cs_decoded <= cs_decoded;
end
*/
endmodule