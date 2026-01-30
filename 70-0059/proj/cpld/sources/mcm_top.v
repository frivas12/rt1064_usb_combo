`include "config.v"
`include "commands.v"

// Company: 		Thorlabs Imaging, Sterling, VA
// Engineer: 		PR
// Create Date: 	06/22/2018
// Module Name: 	mcm_top

module mcm_top #(
	parameter NUM_SLOTS 				= `IMPLEMENTED_SLOTS,
	parameter CS_WIDTH					= 5,
	parameter NUM_IO_PINS_PER_SLOT		= 10
)
(
	// global
	input  wire     									clk_in,					// PLL to 100 MHz	
	input  wire     									resetn,
	output wire											led_sw,
	input  wire [CS_WIDTH-1:0]   						cs,
	output wire [NUM_SLOTS-1:0]							intrpt_out,
	output wire											FLASH_CS,
	output wire 										MAX3421_CS,
	input wire 											CS_READY,				
	
	// SPI 							
	input  wire     									spi_clk,				// 5 MHz
	input  wire     									spi_mosi,
	output wire     									spi_miso,
	input  wire     									spi_scsn,
	
	// UART
	input  wire     									UC_TXD0,
	output wire     									UC_RXD0,

	// 7x10 IO pins
	inout wire 	[NUM_SLOTS*NUM_IO_PINS_PER_SLOT-1:0]	pin_io,
	
	// extra pin header
	inout wire 											C_1,
	inout wire 											C_2,
	inout wire 											C_3,
	inout wire 											C_4,
	inout wire 											C_5,
	inout wire 											C_6,
	inout wire 											C_7,
	inout wire 											C_8
);

// test pins on header JP2
//assign C_1 = clk_1MHz;
//assign C_2 = (uart_slot_en == 12) ? 1:0;
//assign C_3 = (uart_slot_en == 13) ? 1:0;
//assign C_4 = EM_STOP;
//assign C_5 = UC_TXD0;
//assign C_6 = UC_RXD0;


//test pins for testing slot cards.
assign C_1 = uart_slot_en[0];
assign C_2 = uart_slot_en[1];
assign C_3 = uart_slot_en[2];
assign C_4 = uart_slot_en[3];
assign C_5 = UC_TXD0;
assign C_6 = UC_RXD0;

localparam									NUM_PINS_PER_SLOT			= 10;			
localparam 									SPI_PACKET_WIDTH 			= 8;
localparam 									SPI_DATA_BYTES 				= 6;
localparam 									SPI_DATA_WIDTH				= SPI_DATA_BYTES*SPI_PACKET_WIDTH;
localparam 									PWM_CNTR_WIDTH 				= 12;
localparam 									QUAD_CNTR_WIDTH  			= 32;
localparam 									NUM_CS_PER_SLOT				= 2;
localparam 									NUM_INTRPTS_PER_SLOT		= 3;
localparam									NUM_DIG_OUT_PER_SLOT		= 2;
localparam 									SLOT_TYPE_CONFIG_WIDTH  	= 8;
localparam 									UART_ADDRESS_WIDTH		 	= 4;

// global
wire													clk;
		
// ********** spi signals **********			
// used for sdi
wire 		[15:0]         				spi_cmd_r;
wire  		[7:0]         					spi_addr_r;
wire  		[39:0] 						spi_data_r;
wire										spi_data_valid_r;
// used for sdo
wire 		[15:0]         				spi_cmd;
wire  		[7:0]         					spi_addr;
wire  		[39:0] 						spi_data_out_r;	

// ********** SPI cs decoder **********			
wire 		[NUM_SLOTS*NUM_CS_PER_SLOT-1:0]				cs_decoded;

// ********** between modules **********			
// quand encoder to any slot
wire 		[NUM_SLOTS-1:0]								quad_clr;
wire 		[NUM_SLOTS-1:0]								quad_a;
wire 		[NUM_SLOTS-1:0]								quad_b;

// interrupts
wire 		[NUM_SLOTS*NUM_INTRPTS_PER_SLOT-1:0]		pin_intrpt;

// pwm to any slot
wire 		[NUM_SLOTS-1:0]								pwm_out;

// uart to any slot, Cable detection address starts on slot + 7 so we can address other uart on slot
wire 		[UART_ADDRESS_WIDTH-1:0]					uart_slot_en;

// ********** wishbone signals **********			
wire	 			wb_clk_i;
wire  				wb_rst_i;
wire 				wb_cyc_i;
wire 				wb_stb_i;
wire 				wb_we_i;
wire [7:0] 			wb_adr_i;
wire [7:0] 			wb_dat_i;
wire [7:0] 			wb_dat_o;
wire 				wb_ack_o;

wire [7:0]          address;                       
wire                wr_en;                               
wire [7:0]          wr_data;                       
wire                rd_en;                               
wire [7:0]          rd_data;                       
wire                wb_xfer_done;                           
wire                wb_xfer_req; 

wire 				EM_STOP;

// ********** Internal Oscillator **********
// defparam OSCH_inst.NOM_FREQ = "2.08";// This is the default frequency
//defparam OSCH_inst.NOM_FREQ = "66.5";
defparam OSCH_inst.NOM_FREQ = "38.00";
OSCH OSCH_inst( .STDBY(1'b0), // 0=Enabled, 1=Disabled
				// also Disabled with Bandgap=OFF
				.OSC(clk),
				.SEDSTDBY()); // this signal is not required if not
				// using SED

// ********** PLL from uC clock out pint **********
wire clk_96M; 
wire clk_100k; 
wire clk_20k; 
wire clk_1MHz; 

// pll 12mhz to 96mhz, 100kHz and  20kHz
//pll __ (.CLKI( clk_in), .CLKOP(clk_96M), .CLKOS(clk_100k), .CLKOS2(clk_20k), .CLKOS3(clk_1MHz));
pll __ (.CLKI( clk_in), .CLKOP(clk_96M ), .CLKOS( clk_100k), .CLKOS2( clk_20k), .CLKOS3( clk_1MHz));

//***********************************************************************
// 100Hz Clock
//***********************************************************************
/*
reg clk_100Hz;
reg [7:0] counter_clk;
// 20,000/100 =  200 counts 

always @(posedge clk_20k) begin
	if (!resetn) begin
		counter_clk <= 0;
		clk_100Hz <= 0;
	end else begin
		counter_clk <= counter_clk + 1;
		if (counter_clk < 100) begin
		  clk_100Hz <= 1; // CLK high signal
		end
		else if (counter_clk < 200) begin
		  clk_100Hz <= 0; // CLK low signal
		end
		else if (counter_clk == 200) begin
		  counter_clk <= 0; // Reset counter for next period
		  clk_100Hz <= clk_100Hz;
		end
	end
end
*/
// ********** spi_slave_top instantiation **********
spi_slave_top#(
	.QUAD_CNTR_WIDTH 		( QUAD_CNTR_WIDTH			),
	.NUM_INTRPTS_PER_SLOT	( NUM_INTRPTS_PER_SLOT		),
	.NUM_SLOTS 				( NUM_SLOTS					),
	.SPI_DATA_WIDTH			( SPI_DATA_WIDTH			),
	.NUM_DIG_OUT_PER_SLOT	( NUM_DIG_OUT_PER_SLOT		),
	.SLOT_TYPE_CONFIG_WIDTH ( SLOT_TYPE_CONFIG_WIDTH	)
) spi_slave_top_inst(
	.clk					( clk						),					
	.resetn					( resetn					),
	
	// spi
	.spi_clk				( spi_clk					),				
	.spi_scsn				( spi_scsn					),
	.spi_mosi				( spi_mosi					),
	.spi_miso				( spi_miso					),
	
	// used for sdi
	.spi_cmd_r				( spi_cmd_r					),
	.spi_addr_r				( spi_addr_r				),
	.spi_data_r				( spi_data_r				),
	.spi_data_valid_r		( spi_data_valid_r			),

	// used for sd0
	.spi_cmd				( spi_cmd					),
	.spi_addr				( spi_addr					),
	.spi_data_out_r			( spi_data_out_r			)
);

// ********** cs decoding **********
cs_decoder #(
	.NUM_SLOTS			( NUM_SLOTS					),	
    .NUM_CS_PER_SLOT	( NUM_CS_PER_SLOT			),
    .CS_IN_WIDTH		( 5							)
) u_cs_decoder(		
	.clk				( clk						),				
	.resetn				( resetn					),
	.CS_READY			( CS_READY					),
	.cs					( cs						),
	.cs_decoded			( cs_decoded				),
	.FLASH_CS			( FLASH_CS					),
	.MAX3421_CS			( MAX3421_CS				)
);

// ********** led instantiation **********

status_led#(
		.DEV_ID				( `LED_ADDRESS				),
		.PWM_CNTR_WIDTH 	( PWM_CNTR_WIDTH			)
) u_status_led(		
		.clk				( clk						),			
		.resetn				( resetn					),						
		.pwm_out			( led_sw					),

		// SPI
		.spi_cmd_r			( spi_cmd_r					),
		.spi_addr_r			( spi_addr_r				),
		.spi_data_r			( spi_data_r				),
		.spi_data_valid_r	( spi_data_valid_r			),

		// used for sdo
		.spi_cmd			( spi_cmd					),
		.spi_addr			( spi_addr					),
		.spi_data_out_r		( spi_data_out_r			),
		
		.EM_STOP			( EM_STOP					)
);

// ********** uart controller instantiation **********

uart_controller#(
		.DEV_ID				( `UART_CONTROLLER_ADDRESS	),
		.UART_ADDRESS_WIDTH	( UART_ADDRESS_WIDTH	)
) u_uart_controller(		
		.clk				( clk						),			
		.resetn				( resetn					),						

		// SPI
		.spi_cmd_r			( spi_cmd_r					),
		.spi_addr_r			( spi_addr_r				),
		.spi_data_r			( spi_data_r				),
		.spi_data_valid_r	( spi_data_valid_r			),
		
		.uart_slot_en		(uart_slot_en				)						
);

// ********** instantiate quad_decoder **********
genvar quad_id;
generate for (quad_id = 0; quad_id < NUM_SLOTS; quad_id = quad_id + 1)
	begin: quad_ins
quad_decoder  #(
				.DEV_ID				( quad_id							),
				.QUAD_CNTR_WIDTH	( QUAD_CNTR_WIDTH					)
) u_quad_decoder(												
		.clk				( clk										),			
		.clk_1MHz			( clk_1MHz									),			
		.resetn				( resetn									),			

		// SPI
		// used for sdi
		.spi_cmd_r			( spi_cmd_r									),
		.spi_addr_r			( spi_addr_r								),
		.spi_data_r			( spi_data_r								),
		.spi_data_valid_r	( spi_data_valid_r							),
		
		// used for sdo
		.spi_cmd			( spi_cmd									),
		.spi_addr			( spi_addr									),
		.spi_data_out_r		( spi_data_out_r							),

		// between modules
		.quad_index			( pin_intrpt[quad_id*NUM_INTRPTS_PER_SLOT + 2]							),							
		.quad_a				( quad_a[quad_id]							),			
		.quad_b				( quad_b[quad_id]							)			
);
	end
endgenerate	

// ********** instantiate interrupts for each slot **********
genvar intrpt_id;
generate for (intrpt_id = 0; intrpt_id < NUM_SLOTS; intrpt_id = intrpt_id + 1)
	begin: intrpt_ins
intrpt_ctrl  #(
		.DEV_ID					( intrpt_id											),
		.NUM_INTRPTS_PER_SLOT	( NUM_INTRPTS_PER_SLOT								)
) u_intrpt_ctrl(												
		.clk					( clk												),			
		.resetn					( resetn											),	
				
		// SPI
		// used for sdi
		.spi_cmd_r				( spi_cmd_r											),
		.spi_addr_r				( spi_addr_r										),
		.spi_data_r				( spi_data_r										),
		.spi_data_valid_r		( spi_data_valid_r									),
		
		// used for sdo
		.spi_cmd				( spi_cmd											),
		.spi_addr				( spi_addr											),
		.spi_data_out_r			( spi_data_out_r									),
		
		// between modules
		.intrpt_in_0			( pin_intrpt[intrpt_id*NUM_INTRPTS_PER_SLOT + 0]	),			
		.intrpt_in_1			( pin_intrpt[intrpt_id*NUM_INTRPTS_PER_SLOT + 1]	),		
		.intrpt_in_2			( pin_intrpt[intrpt_id*NUM_INTRPTS_PER_SLOT + 2]	),
		.intrpt_out				( intrpt_out[intrpt_id]								)							
);
	end
endgenerate	

// ********** instantiate pwm_controller for each slot **********
genvar pwm_id;
generate for (pwm_id = 0; pwm_id < NUM_SLOTS; pwm_id = pwm_id + 1)
	begin: pwm_ins
		// 
		pwm_controller#(
				.DEV_ID				( pwm_id					),
				.PWM_CNTR_WIDTH 	( PWM_CNTR_WIDTH			)
		) pwm_controller_ins(		
				.clk				( clk						),			
				.resetn				( resetn					),						
				.pwm_out			( pwm_out[pwm_id]			),

		// SPI
		// used for sdi
		.spi_cmd_r				( spi_cmd_r						),
		.spi_addr_r				( spi_addr_r					),
		.spi_data_r				( spi_data_r					),
		.spi_data_valid_r		( spi_data_valid_r				)
		
		);
	end
endgenerate	

// ****************************************
// ********** Card instantiation **********
// ****************************************

// ********** stepper instantiation **********
genvar stepper_id;
generate for (stepper_id = (`START_SLOT_STEPPER - 1); stepper_id < `STOP_SLOT_STEPPER; stepper_id = stepper_id + 1)
	begin: stepper_ins
stepper#(
		.DEV_ID				( stepper_id								),
		.UART_ADDRESS_WIDTH	( UART_ADDRESS_WIDTH						)
) u_stepper(		
		.clk				( clk										),			
		.resetn				( resetn									),						
		.clk_1MHz 			(clk_1MHz									),
		// Slot pins
		.CS					( pin_io[(stepper_id * 10) + 0]				),						
		.RESET				( pin_io[(stepper_id * 10) + 1]				),						
		.ul					( pin_io[(stepper_id * 10) + 2]				),						
		.ll					( pin_io[(stepper_id * 10) + 3]				),						
		.index				( pin_io[(stepper_id * 10) + 4]				),						
		.OW_ID				( pin_io[(stepper_id * 10) + 5]				),						
		.DRV_CS				( pin_io[(stepper_id * 10) + 6]				),						
		.SSI_SEL			( pin_io[(stepper_id * 10) + 7]				),						
		.ENC_I				( pin_io[(stepper_id * 10) + 8]				),						
		.ENC_O				( pin_io[(stepper_id * 10) + 9]				),

		// SPI
		// used for sdi
		.spi_cmd_r			( spi_cmd_r					),
		.spi_addr_r			( spi_addr_r				),
		.spi_data_r			( spi_data_r				),
		.spi_data_valid_r	( spi_data_valid_r			),

		// used for sdo
		.spi_cmd			( spi_cmd									),
		.spi_addr			( spi_addr									),
		.spi_data_out_r		( spi_data_out_r							),
		
		// between modules
		.cs_decoded_in		(cs_decoded[stepper_id*NUM_CS_PER_SLOT + 0]			),
		.cs_decoded_in2		(cs_decoded[stepper_id*NUM_CS_PER_SLOT + 1]			),
		.quad_a_out			(quad_a[stepper_id]									),
		.quad_b_out			(quad_b[stepper_id]									),
		.ul_out				(pin_intrpt[stepper_id*NUM_INTRPTS_PER_SLOT + 0]	),
		.ll_out				(pin_intrpt[stepper_id*NUM_INTRPTS_PER_SLOT + 1]	),
		.index_out			(pin_intrpt[stepper_id*NUM_INTRPTS_PER_SLOT + 2]	),
		
		// uart for one wire
		.uart_slot_en		(uart_slot_en										),						
		.rx_slot			(UC_RXD0											),						
		.tx_slot			(UC_TXD0											),

		.EM_STOP			(EM_STOP											)
);
	end
endgenerate	

// ********** servo instantiation **********
genvar servo_id;
generate for (servo_id = (`START_SLOT_SERVO - 1); servo_id < `STOP_SLOT_SERVO; servo_id = servo_id + 1)
	begin: servo_ins
servo#(
		.DEV_ID				( servo_id									),
		.UART_ADDRESS_WIDTH	( UART_ADDRESS_WIDTH						)
) u_servo(		
		.clk				( clk										),			
		.resetn				( resetn									),						
	
		// Slot pins
		.MODE				( pin_io[(servo_id * 10) + 0]				),						
		.ul					( pin_io[(servo_id * 10) + 1]				),						
		.ll					( pin_io[(servo_id * 10) + 2]				),						
		.MODE1				( pin_io[(servo_id * 10) + 4]				),						
		.Phase				( pin_io[(servo_id * 10) + 5]				),						
		.OW_ID				( pin_io[(servo_id * 10) + 6]				),						
		.Enable				( pin_io[(servo_id * 10) + 7]				),						
		.quad_a				( pin_io[(servo_id * 10) + 8]				),						
		.quad_b				( pin_io[(servo_id * 10) + 9]				),						

		// SPI
		// used for sdi
		.spi_cmd_r			( spi_cmd_r									),
		.spi_addr_r			( spi_addr_r								),
		.spi_data_r			( spi_data_r								),
		.spi_data_valid_r	( spi_data_valid_r							),

		// between modules
		.quad_a_out			(quad_a[servo_id]								),
		.quad_b_out			(quad_b[servo_id]								),
		.ul_out				(pin_intrpt[servo_id*NUM_INTRPTS_PER_SLOT + 0]	),
		.ll_out				(pin_intrpt[servo_id*NUM_INTRPTS_PER_SLOT + 1]	),
		.pwm_in				(pwm_out[servo_id]								),

		// uart for one wire
		.uart_slot_en		(uart_slot_en										),						
		.rx_slot			(UC_RXD0											),						
		.tx_slot			(UC_TXD0											),

		.EM_STOP			(EM_STOP										)
);
	end
endgenerate	

// ********** shutter card instantiation **********

genvar shutter_id;
generate for (shutter_id = (`START_SLOT_SHUTTER - 1); shutter_id < `STOP_SLOT_SHUTTER; shutter_id = shutter_id + 1)
	begin: shutter_ins
shutter#(
		.DEV_ID				( shutter_id								),
		.UART_ADDRESS_WIDTH	( UART_ADDRESS_WIDTH						)
) u_shutter(		
		.clk				( clk										),			
		.resetn				( resetn									),						
	
		// Slot pins
		.CS					( pin_io[(shutter_id * 10) + 0]				),						
		.IN_2_2				( pin_io[(shutter_id * 10) + 1]				),						
		.IN_2_3				( pin_io[(shutter_id * 10) + 2]				),						
		.IN_1_2				( pin_io[(shutter_id * 10) + 3]				),						
		.IN_1_3				( pin_io[(shutter_id * 10) + 4]				),						
		.OW_ID				( pin_io[(shutter_id * 10) + 5]				),						
		.IN_2_1				( pin_io[(shutter_id * 10) + 6]				),						
		.IN_2_4				( pin_io[(shutter_id * 10) + 7]				),						
		.IN_1_1				( pin_io[(shutter_id * 10) + 8]				),						
		.IN_1_4				( pin_io[(shutter_id * 10) + 9]				), // temprary change to test 					

		// SPI
		// used for sdi
		.spi_cmd_r			( spi_cmd_r									),
		.spi_addr_r			( spi_addr_r								),
		.spi_data_r			( spi_data_r								),
		.spi_data_valid_r	( spi_data_valid_r							),

		// between modules
		.cs_decoded_in		(cs_decoded[shutter_id*NUM_CS_PER_SLOT + 0]	),
		
		// uart for one wire
		.uart_slot_en		(uart_slot_en								),						
		.rx_slot			(UC_RXD0									),						
		.tx_slot			(UC_TXD0									),

		.EM_STOP			(EM_STOP									)
);
	end
endgenerate	

// ********** Piezo card instantiation **********

// genvar piezo_id;
// generate for (piezo_id = (`START_SLOT_PIEZO - 1); piezo_id < `STOP_SLOT_PIEZO; piezo_id = piezo_id + 1)
// 	begin: piezo_ins
// piezo#(
// 		.DEV_ID				( piezo_id									),
// 		.UART_ADDRESS_WIDTH	( UART_ADDRESS_WIDTH						)
// ) u_piezo(		
// 		.clk				( clk										),			
// 		.resetn				( resetn									),						
// 	
// 		// Slot pins
// 		.CS_DAC				( pin_io[(piezo_id * 10) + 0]				),						
// //		.C_SCLK				( pin_io[(piezo_id * 10) + 1]				),						
// //		.C_MOSI				( pin_io[(piezo_id * 10) + 2]				),						
// 		.CS_ADC				( pin_io[(piezo_id * 10) + 3]				),						
// 		.TRIGGER			( pin_io[(piezo_id * 10) + 4]				),						
// 		.OW_ID				( pin_io[(piezo_id * 10) + 5]				),						
// 
// 		// SPI
// 		// used for sdi
// 		.spi_cmd_r			( spi_cmd_r									),
// 		.spi_addr_r			( spi_addr_r								),
// 		.spi_data_r			( spi_data_r								),
// 		.spi_data_valid_r	( spi_data_valid_r							),
// 
// 		// between modules
// 		.cs_decoded_in		(cs_decoded[piezo_id*NUM_CS_PER_SLOT + 0]	),
// 		.cs_decoded_in2		(cs_decoded[piezo_id*NUM_CS_PER_SLOT + 1]	),
// 		
// 		// uart for one wire
// 		.uart_slot_en		(uart_slot_en								),						
// 		.rx_slot			(UC_RXD0									),						
// 		.tx_slot			(UC_TXD0									),
// 
// 		.EM_STOP			(EM_STOP									)
// );
// 
// end
// endgenerate	

endmodule
