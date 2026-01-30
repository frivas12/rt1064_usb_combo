`include "commands.v"
// Company: 		Thorlabs Imaging, Sterling, VA
// Engineer: 		ZS
// Create Date: 	03/13/2018
// Module Name: 	set_pins


module set_pins#(
	parameter SLOT_TYPE_CONFIG_WIDTH  	= 8,
	parameter NUM_SLOTS					= 7,
	parameter NUM_IO_PINS_PER_SLOT		= 10,
	parameter SPI_DATA_WIDTH			= 48,
	parameter NUM_CS_PER_SLOT			= 2,
	parameter NUM_DIG_OUT_PER_SLOT		= 2,
	parameter NUM_PINS_PER_SLOT			= 10,
	parameter NUM_INTRPTS_PER_SLOT		= 3
)
(
	input wire 											clk,					// 100 MHz
	input wire 											resetn,
	input wire [NUM_SLOTS*SLOT_TYPE_CONFIG_WIDTH-1:0]	slot_type_config,
	input wire 											CS_READY,
	input wire [NUM_SLOTS*NUM_CS_PER_SLOT-1:0]			cs_decoded,
	input wire [NUM_SLOTS*NUM_DIG_OUT_PER_SLOT-1:0]		dig_out,
	input wire [NUM_SLOTS-1:0]							pwm_out,
	input wire [NUM_SLOTS-1:0]							servo_phase,
	
	output reg [NUM_SLOTS*NUM_IO_PINS_PER_SLOT-1:0]		pin_io_dir,
	output reg [NUM_SLOTS*NUM_IO_PINS_PER_SLOT-1:0]		pin_out,
	output reg [NUM_SLOTS*NUM_IO_PINS_PER_SLOT-1:0]		pin_in,
	output reg [NUM_SLOTS*NUM_INTRPTS_PER_SLOT-1:0]		pin_intrpt,
	input wire 											UC_TXD0,
	output wire 										UC_RXD0
);
	
	wire [SLOT_TYPE_CONFIG_WIDTH-1:0] 					slot_1_type_config;
	wire [SLOT_TYPE_CONFIG_WIDTH-1:0] 					slot_2_type_config;
	wire [SLOT_TYPE_CONFIG_WIDTH-1:0] 					slot_3_type_config;
	wire [SLOT_TYPE_CONFIG_WIDTH-1:0] 					slot_4_type_config;
	wire [SLOT_TYPE_CONFIG_WIDTH-1:0] 					slot_5_type_config;
	wire [SLOT_TYPE_CONFIG_WIDTH-1:0] 					slot_6_type_config;
	wire [SLOT_TYPE_CONFIG_WIDTH-1:0] 					slot_7_type_config;


reg[6:0] UC_RXD0_r;assign UC_RXD0 = UC_RXD0_r[4];

genvar slot_conf_id;
generate for (slot_conf_id = 0; slot_conf_id < NUM_SLOTS; slot_conf_id = slot_conf_id + 1)
	begin
		always @ (posedge clk) begin
			if (!resetn) begin
				pin_io_dir[((slot_conf_id+1)*NUM_IO_PINS_PER_SLOT)-1:slot_conf_id*NUM_IO_PINS_PER_SLOT]	 	<= `M_DIR_IN;;	// set all input
				pin_out[((slot_conf_id+1)*NUM_IO_PINS_PER_SLOT)-1:slot_conf_id*NUM_IO_PINS_PER_SLOT]		<= 'bz;	// set all pin to trisate
			end	
			else begin
				if (slot_type_config[(slot_conf_id+1)*SLOT_TYPE_CONFIG_WIDTH-1:slot_conf_id*SLOT_TYPE_CONFIG_WIDTH] == `DEV_TYPE_STEPPER) begin
					// *** Stepper ***
					// C0 => NOT USED
					// C1 => RESET : output
					// C2 => UL : input
					// C3 => LL : input
					// C4 => Index : input
					// C5 => OW_ID : bi-directional
					// C6 => CS (chip select) : output
					// C7 => STCK (NOT USED)
					// C8 => ENC_A : input
					// C9 => ENC_B : input

					// Setup pin direction
					pin_io_dir[slot_conf_id*NUM_IO_PINS_PER_SLOT + 1]	 	<= `M_DIR_OUT;
					pin_io_dir[slot_conf_id*NUM_IO_PINS_PER_SLOT + 6]	 	<= `M_DIR_OUT;
					pin_io_dir[slot_conf_id*NUM_IO_PINS_PER_SLOT + 5]		<= `M_DIR_OUT;	// C5 OW_ID
					
					pin_intrpt[slot_conf_id*NUM_INTRPTS_PER_SLOT + 0] 	<= pin_in[slot_conf_id*NUM_IO_PINS_PER_SLOT + 2];
					pin_intrpt[slot_conf_id*NUM_INTRPTS_PER_SLOT + 1] 	<= pin_in[slot_conf_id*NUM_IO_PINS_PER_SLOT + 3];
					pin_intrpt[slot_conf_id*NUM_INTRPTS_PER_SLOT + 2]	<= pin_in[slot_conf_id*NUM_IO_PINS_PER_SLOT + 4];

					// make input connections 
					UC_RXD0_r[slot_conf_id] <= pin_in[slot_conf_id*NUM_IO_PINS_PER_SLOT + 5]; 	// C5 OW_ID

					// make output connections
					pin_out[slot_conf_id*NUM_IO_PINS_PER_SLOT + 1] 		<= dig_out[slot_conf_id*NUM_DIG_OUT_PER_SLOT + 0];
					pin_out[slot_conf_id*NUM_IO_PINS_PER_SLOT + 6] 		<= cs_decoded[slot_conf_id*NUM_CS_PER_SLOT + 0];	
					pin_out[slot_conf_id*NUM_IO_PINS_PER_SLOT + 0] 		<= UC_TXD0; 	// C0 UC_TXD0
				end
				
			else if (slot_type_config[(slot_conf_id+1)*SLOT_TYPE_CONFIG_WIDTH-1:slot_conf_id*SLOT_TYPE_CONFIG_WIDTH] == `DEV_TYPE_SERVO) begin
					// Setup pin direction
					pin_io_dir[slot_conf_id*NUM_IO_PINS_PER_SLOT + 0]	<= `M_DIR_OUT;	// C0 Mode2
					pin_io_dir[slot_conf_id*NUM_IO_PINS_PER_SLOT + 4]	<= `M_DIR_OUT;	// C4 Mode1
					pin_io_dir[slot_conf_id*NUM_IO_PINS_PER_SLOT + 5]	<= `M_DIR_OUT;	// C5 Phase
					pin_io_dir[slot_conf_id*NUM_IO_PINS_PER_SLOT + 7]	<= `M_DIR_OUT;	// C7 Enable

					// make input connections
					// C0 output Mode2
					pin_out[slot_conf_id*NUM_IO_PINS_PER_SLOT + 0] 		<= dig_out[slot_conf_id*NUM_DIG_OUT_PER_SLOT + 0];
					// C1 input UL
					pin_intrpt[slot_conf_id*NUM_INTRPTS_PER_SLOT + 0] 	<= pin_in[(slot_conf_id*NUM_IO_PINS_PER_SLOT) + 1];
					// C2 not used in shutter so set hi
					pin_intrpt[slot_conf_id*NUM_INTRPTS_PER_SLOT + 1] 	<= 1;
					// C3 not used in shutter so set hi
					pin_intrpt[slot_conf_id*NUM_INTRPTS_PER_SLOT + 2] 	<= 1;
					
					// make output connections
					// C0 Mode1
					pin_out[slot_conf_id*NUM_IO_PINS_PER_SLOT + 0] 		<= dig_out[slot_conf_id*NUM_DIG_OUT_PER_SLOT + 0];	
					// C4 Mode1
					pin_out[slot_conf_id*NUM_IO_PINS_PER_SLOT + 4] 		<= dig_out[slot_conf_id*NUM_DIG_OUT_PER_SLOT + 1];	
					// C5 Phase
					pin_out[slot_conf_id*NUM_IO_PINS_PER_SLOT + 5] 		<= servo_phase[slot_conf_id];
					// C7 Enable
					pin_out[slot_conf_id*NUM_IO_PINS_PER_SLOT + 7] 		<= pwm_out[slot_conf_id];	
				end
			else if (slot_type_config[(slot_conf_id+1)*SLOT_TYPE_CONFIG_WIDTH-1:slot_conf_id*SLOT_TYPE_CONFIG_WIDTH] == `DEV_TYPE_OTM_DAC) begin
					// CO => CS (chip select) : output
					// C1 => NOT USED
					// C2 => NOT USED
					// C3 => NOT USED
					// C4 => NOT USED
					// C5 => NOT USED
					// C6 => NOT USED
					// C7 => LASER_CNTRL : output
					// C8 => NOT USED
					// C9 => IPG_B_REFL : input
					
					
					// Setup pin direction
					pin_io_dir[slot_conf_id*NUM_IO_PINS_PER_SLOT + 0]	<= `M_DIR_OUT;	// C0 CS
					pin_io_dir[slot_conf_id*NUM_IO_PINS_PER_SLOT + 7]	<= `M_DIR_OUT;	// C7 LASER_CNTRL

					// make input connections

					// make output connections
					// C0 CS
					pin_out[slot_conf_id*NUM_IO_PINS_PER_SLOT + 0] 		<= cs_decoded[slot_conf_id*NUM_CS_PER_SLOT + 0];	
					// C7 LASER_CNTRL
					pin_out[slot_conf_id*NUM_IO_PINS_PER_SLOT + 7] 		<= dig_out[slot_conf_id*NUM_DIG_OUT_PER_SLOT + 0];	

				end
			else if (slot_type_config[(slot_conf_id+1)*SLOT_TYPE_CONFIG_WIDTH-1:slot_conf_id*SLOT_TYPE_CONFIG_WIDTH] == `DEV_TYPE_RS232) begin
					// CO => TX_IN : output to RS232 IC
					// C1 => RX_OUT : input from RS232 IC
					// C2 => NOT USED
					// C3 => NOT USED
					// C4 => NOT USED
					// C5 => NOT USED
					// C6 => NOT USED
					// C7 => NOT USED
					// C8 => NOT USED
					// C9 => NOT USED
					
					
					// Setup pin direction
					pin_io_dir[slot_conf_id*NUM_IO_PINS_PER_SLOT + 0]	<= `M_DIR_OUT;	// C0 TX_IN

					// make input connections 
					UC_RXD0_r[slot_conf_id] <= pin_in[slot_conf_id*NUM_IO_PINS_PER_SLOT + 1]; 	// C1 RX_OUT
					
					// make output connections
					pin_out[slot_conf_id*NUM_IO_PINS_PER_SLOT + 0] <=  UC_TXD0; 	// C0 UC_TXD0

				end
/*			else if (slot_type_config[(slot_conf_id+1)*SLOT_TYPE_CONFIG_WIDTH-1:slot_conf_id*SLOT_TYPE_CONFIG_WIDTH] == `DEV_TYPE_GALVO_DRIVER) begin
					// (internal SPI not part of SPI on main board)
					
					// CO => LDAC 		: output  DAC Update Pin
					// C1 => USED
					// C2 => DAC_SCK 	: output  	SPI CLK 
					// C3 => DAC_SDO 	: input  	SPI MISO 
					// C4 => DAC_SDI 	: output  	SPI MOSI 
					// C5 => TGP 		: output  	Toggle Pin
					// C6 => NOT USED
					// C7 => CS_LD 		: output(chip select internal not part of SPI on main board) 
					// C8 => OVRTMP		: input 	Thermal Protection Interrupt
					// C9 => NOT USED
					
					// Setup pin direction
					pin_io_dir[slot_conf_id*NUM_IO_PINS_PER_SLOT + 0]	<= `M_DIR_OUT;
					pin_io_dir[slot_conf_id*NUM_IO_PINS_PER_SLOT + 2]	<= `M_DIR_OUT;
					pin_io_dir[slot_conf_id*NUM_IO_PINS_PER_SLOT + 4]	<= `M_DIR_OUT;
					pin_io_dir[slot_conf_id*NUM_IO_PINS_PER_SLOT + 5]	<= `M_DIR_OUT;
					pin_io_dir[slot_conf_id*NUM_IO_PINS_PER_SLOT + 7]	<= `M_DIR_OUT;

					// make input connections
					//UC_RXD0_r[slot_conf_id] <= pin_in[slot_conf_id*NUM_IO_PINS_PER_SLOT + 1]; 	// C1 RX_OUT
					
					// make output connections
					//pin_out[slot_conf_id*NUM_IO_PINS_PER_SLOT + 0] <=  UC_TXD0; 	// C0 UC_TXD0

				end */
			else begin
				pin_io_dir[((slot_conf_id+1)*NUM_IO_PINS_PER_SLOT)-1:slot_conf_id*NUM_IO_PINS_PER_SLOT]	 	<= `M_DIR_IN;;	// set all input
				pin_out[((slot_conf_id+1)*NUM_IO_PINS_PER_SLOT)-1:slot_conf_id*NUM_IO_PINS_PER_SLOT]		<= 'bz;	// set all pin to trisate
			end
			end	
		end	
	end	
endgenerate
	
endmodule