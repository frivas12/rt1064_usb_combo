`timescale 1ns / 1ns

`include "../sources/commands.v"
// Company: 		Thorlabs Imaging, Sterling, VA
// Engineer: 		ZS
// Create Date: 	02/27/2018
// Module Name: 	tb_mcm_top



module tb_mcm_top
(
);

parameter NUM_SLOTS 				= 7;
parameter NUM_INTRPTS				= 4;
parameter CS_WIDTH 					= 5;
parameter QUAD_CNTR_WIDTH   		= 32;
parameter NUM_IO_PINS_PER_SLOT 		= 10;
parameter SPI_PACKET_WIDTH 			= 8;
parameter SPI_DATA_BYTES 			= 6;
parameter SPI_DATA_WIDTH			= SPI_DATA_BYTES*SPI_PACKET_WIDTH;

parameter SPI_CLK_TIME				= 100;

// global
reg     										clk;					// PLL to 100 MHz	
reg     										resetn;

								
// SPI 								
reg     										spi_clk;				// 5 MHz
reg     										spi_mosi;
wire     										spi_miso;
reg     										spi_scsn;

reg  [SPI_PACKET_WIDTH-1:0]						spi_sdi_command, spi_sdi_address;
reg  [SPI_DATA_WIDTH-1:0]						spi_sdi_data; 		

wire 	[7:0]									spi_cmd;
wire											spi_cmd_valid;
wire	[7:0]									spi_addr;
wire											spi_addr_valid;
wire	[47:0]									spi_data;
wire											spi_data_valid;	
reg   [47:0]									spi_sdo;
reg												spi_sdo_valid;	


reg   [31:0]									pwm_freq;
reg   [31:0]									pwm_duty;
wire											pwm_out;
//reg	 [3:0]									i,j,slot;
reg	 [6:0]										i,j,q;
reg	 [3:0]										slot;
reg	 [5:0]										index;

reg		[NUM_SLOTS*NUM_IO_PINS_PER_SLOT-1:0]	pin_in;
wire 	[NUM_SLOTS*NUM_IO_PINS_PER_SLOT-1:0]	pin_out;
wire 	[NUM_SLOTS*NUM_IO_PINS_PER_SLOT-1:0]	pin_io;
wire											led_sw;	
reg 											debug_pin;

GSR GSR_INST(.GSR(1'b1));     
PUR PUR_INST(.PUR(1'b1));


mcm_top #(
	.NUM_SLOTS 				(7					),
	.CS_WIDTH				(5					),
	.NUM_IO_PINS_PER_SLOT	(10					)
)dut_mcm_top(
	
	.clk_in					(					),					// PLL to 100 MHz	
	.resetn					(resetn				),
	.led_sw					(led_sw				),
	.cs						(					),
	.intrpt_out				(					),
	.FLASH_CS				(					),
	.MAX3421_CS				(					),
	.CM_1					(					),				
						
	.spi_clk				(spi_clk			),				// 5 MHz
	.spi_mosi				(spi_mosi			),
	.spi_miso				(spi_miso			),
	.spi_scsn				(spi_scsn			),
													
	// 7x10 IO pins	
	.pin_io					(pin_io				)
	//.debug_pin				(debug_pin			)
	//.pin_in					(pin_in				)
	//.pin_out				(pin_out			),
	//.pin_io_dir				(pin_io_dir			)
	
	
	
);



// spi mosi task
task SPI_WRITE_OUT;
	input [SPI_PACKET_WIDTH-1:0] 	SPI_CMD;
	input [SPI_PACKET_WIDTH-1:0] 	SPI_ADDR;
	input [SPI_DATA_WIDTH-1:0] 		SPI_DATA;
	//integer	i, j, q;
	begin
	spi_mosi 						= 1'b1;
	spi_scsn						= 1'b0;
	for (i=0; i<SPI_PACKET_WIDTH; i=i+1) begin
		spi_mosi   	= SPI_CMD[SPI_PACKET_WIDTH-i-1]; 
		#SPI_CLK_TIME; 		
		spi_clk 	= ~spi_clk;
		#SPI_CLK_TIME;
		spi_clk 	= ~spi_clk;
	end	
	spi_mosi 						= 1'b1;
	#200;
	
	for (i=0; i<SPI_PACKET_WIDTH; i=i+1) begin
		spi_mosi   	= SPI_ADDR[SPI_PACKET_WIDTH-i-1];  
		#SPI_CLK_TIME; 		
		spi_clk 	= ~spi_clk;
		#SPI_CLK_TIME;
		spi_clk 	= ~spi_clk;
	end
	spi_mosi 						= 1'b1;
	#100;
	spi_sdo							= 48'h5c_96_bd_30_a6_47;
	spi_sdo_valid					= 1'b1;
	#10
	spi_sdo_valid					= 1'b0;
	#90
	
	for (i=0; i<SPI_DATA_BYTES; i=i+1) begin
		for (j=0; j<SPI_PACKET_WIDTH; j=j+1) begin
			//spi_mosi   	= SPI_DATA[(NUM_SLOTS-i-1)*SPI_DATA_WIDTH-j -1];  
			q 			= (6-i)*8-j-1;
			spi_mosi   	= SPI_DATA[q];  
			#SPI_CLK_TIME; 		
			spi_clk 	= ~spi_clk;
			#SPI_CLK_TIME;
			spi_clk 	= ~spi_clk;
		end
		spi_mosi 						= 1'b1;		
		#200;
	end
	
	spi_scsn						= 1'b1;
	spi_mosi 						= 1'b1;
	end
endtask



always begin
	#5 clk <= ~clk;
end


// test quadrature 
initial begin
	pin_in[8]					= 1'b0;

	repeat (6) begin
		repeat (200) begin
			#50
			pin_in[8]					= 1'b1;
			#50
			pin_in[8]					= 1'b0;
		end
	end
end

initial begin
	pin_in[9]					= 1'b0;

	#25
	repeat (6) begin
		repeat (100) begin
			#50
			pin_in[9]					= 1'b1;
			#50
			pin_in[9]					= 1'b0;
		end
		repeat (100) begin
			#50
			pin_in[9]					= 1'b0;
			#50
			pin_in[9]					= 1'b1;
		end
	end	
end


initial begin
	clk 						= 1'b0;
	resetn 						= 1'b0;
	spi_scsn					= 1'b1;
	spi_mosi    				= 1'b1;
	spi_clk						= 1'b0;
	spi_sdo						= 'b0;
	spi_sdo_valid				= 1'b0;
	debug_pin					= 1'b0;
	pin_in[2]					= 1'b0;
	pin_in[3]					= 1'b0;
	pin_in[4]					= 1'b0;
	#200			
	resetn 						= 1'b1;
	#1000		

	
	////////////////////////////////////////////////////////////////
	// SET_PWM_FREQ, 7 times
	//for (slot=0; slot<NUM_SLOTS; slot=slot+1) begin
		spi_sdi_command 			= `C_SET_PWM_FREQ;
		spi_sdi_address 			= `LED_ADDRESS;					
		spi_sdi_data 				= 48'h0000_0000_03e8;
		spi_scsn					= 1'b0;
		SPI_WRITE_OUT(spi_sdi_command, spi_sdi_address, spi_sdi_data);
		#100;
		spi_scsn					= 1'b1;
		#900;			
		
	//end	
	$display("set pwm freq for all 7 slots done");
	////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////
	// SET_PWM_DUTY, 7 times
	//for (slot=0; slot<NUM_SLOTS; slot=slot+1) begin
		spi_sdi_command 			= `C_SET_PWM_DUTY;
		spi_sdi_address 			= `LED_ADDRESS;					
		spi_sdi_data 				= 48'h0000_0000_01f4;
		spi_scsn					= 1'b0;
		SPI_WRITE_OUT(spi_sdi_command, spi_sdi_address, spi_sdi_data);
		#100;
		spi_scsn					= 1'b1;
		#900;			
		
	//end	
	$display("set pwm duty for all 7 slots done");
	////////////////////////////////////////////////////////////////
	#500;
	//for (slot=0; slot<NUM_SLOTS; slot=slot+1) begin
		spi_sdi_command 			= `C_SET_DIG_OUT;
		spi_sdi_address 			= `SLOT_1_ADDRESS;					
		spi_sdi_data 				= 48'h0000_0000_0001;	// 
		spi_scsn					= 1'b0;
		SPI_WRITE_OUT(spi_sdi_command, spi_sdi_address, spi_sdi_data);
		#100;
		spi_scsn					= 1'b1;
		#900;			
		
	//end	
	$display("set digital out done");
	#500;
	//for (slot=0; slot<NUM_SLOTS; slot=slot+1) begin
		spi_sdi_command 			= `C_SET_SLOT_TYPE_CONFIG;
		spi_sdi_address 			= `SLOT_1_ADDRESS;					
		spi_sdi_data 				= 48'h0000_0000_0001;	// 
		spi_scsn					= 1'b0;
		SPI_WRITE_OUT(spi_sdi_command, spi_sdi_address, spi_sdi_data);
		#100;
		spi_scsn					= 1'b1;
		#900;			
		
	//end	
	$display("read interrupt done");
	debug_pin	= 1'b1;
	#500;
	//for (slot=0; slot<NUM_SLOTS; slot=slot+1) begin
		spi_sdi_command 			= `C_READ_INTERUPTS;
		spi_sdi_address 			= `SLOT_1_ADDRESS;					
		spi_sdi_data 				= 48'h0000_0000_0000;	// 
		spi_scsn					= 1'b0;
		SPI_WRITE_OUT(spi_sdi_command, spi_sdi_address, spi_sdi_data);
		#100;
		spi_scsn					= 1'b1;
		#900;			
		
	//end	
	$display("read interrupt done");
	#2000;
	debug_pin	= 1'b0;
	#500;	
	//for (slot=0; slot<NUM_SLOTS; slot=slot+1) begin
		spi_sdi_command 			= `C_READ_INTERUPTS;
		spi_sdi_address 			= `SLOT_1_ADDRESS;					
		spi_sdi_data 				= 48'h0000_0000_0000;	// 
		spi_scsn					= 1'b0;
		SPI_WRITE_OUT(spi_sdi_command, spi_sdi_address, spi_sdi_data);
		#100;
		spi_scsn					= 1'b1;
		#900;			
		
	//end	
	$display("read interrupt done");
	#500;	
	//for (slot=0; slot<NUM_SLOTS; slot=slot+1) begin
		spi_sdi_command 			= `C_SET_QUAD_COUNTS;
		spi_sdi_address 			= `SLOT_1_ADDRESS;					
		spi_sdi_data 				= 48'h0000_0000_0000;	// 
		spi_scsn					= 1'b0;
		SPI_WRITE_OUT(spi_sdi_command, spi_sdi_address, spi_sdi_data);
		#100;
		spi_scsn					= 1'b1;
		#900;			
		
	//end	
	$display("read interrupt done");
	#2500;
	//for (slot=0; slot<NUM_SLOTS; slot=slot+1) begin
		spi_sdi_command 			= `C_SET_QUAD_COUNTS;
		spi_sdi_address 			= `SLOT_1_ADDRESS;					
		spi_sdi_data 				= 48'h0000_0000_0000;	// 
		spi_scsn					= 1'b0;
		SPI_WRITE_OUT(spi_sdi_command, spi_sdi_address, spi_sdi_data);
		#100;
		spi_scsn					= 1'b1;
		#900;			
		
	//end	
	$display("read interrupt done");
	#500;
	
	// test interrupt
	pin_in[2]					= 1'b1;
	#100
	pin_in[2]					= 1'b0;
	#300
	
	pin_in[3]					= 1'b1;
	#100
	pin_in[3]					= 1'b0;
	#300
	
	pin_in[4]					= 1'b1;
	#100
	pin_in[4]					= 1'b0;
end



endmodule