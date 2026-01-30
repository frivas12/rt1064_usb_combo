`timescale 1ns / 1ns

`include "commands.v"
// Company: 		Thorlabs Imaging, Sterling, VA
// Engineer: 		ZS
// Create Date: 	02/27/2018
// Module Name: 	tb_mcm_top



module tb_spi
(
);

parameter NUM_SLOTS 				= 7;
parameter NUM_INTRPTS				= 4;
parameter CS_WIDTH 					= 5;
parameter QUAD_CNTR_WIDTH   		= 32;
parameter NUM_IO_PINS_PER_SLOT 		= 10;
parameter SPI_PACKET_WIDTH 			= 8;
parameter SPI_DATA_BYTES 			= 5;
parameter SPI_DATA_WIDTH			= SPI_DATA_BYTES*SPI_PACKET_WIDTH;

parameter SPI_CLK_TIME				= 100;

// global
//reg     										clk;					// PLL to 100 MHz	
reg     										resetn;

								
// SPI 								
reg     										spi_clk;				// 5 MHz
reg     										spi_mosi;
wire     										spi_miso;
reg     										spi_scsn;

reg  [2*SPI_PACKET_WIDTH-1:0]					spi_sdi_command; 
reg  [SPI_PACKET_WIDTH-1:0]						spi_sdi_address;
reg  [SPI_DATA_WIDTH-1:0]						spi_sdi_data; 		

wire 	[15:0]									spi_cmd;
wire											spi_cmd_valid;
wire	[7:0]									spi_addr;
wire											spi_addr_valid;
wire	[39:0]									spi_data;
wire											spi_data_valid;	
reg   [39:0]									spi_sdo;
reg												spi_sdo_valid;	


reg	 [6:0]										i,j,q;
reg	 [3:0]										slot;
reg	 [5:0]										index;


GSR GSR_INST(.GSR(1'b1));     
PUR PUR_INST(.PUR(1'b1));




spi_slave_top dut_spi_slave_top(
	//.clk					(					),					// PLL to 100 MHz	
	.resetn					(resetn				),			
						
	.spi_clk				(spi_clk			),				// 5 MHz
	.spi_mosi				(spi_mosi			),
	.spi_miso				(spi_miso			),
	.spi_scsn				(spi_scsn			),
													
	.spi_cmd_r				(spi_cmd			),
    .spi_cmd_valid_r		(spi_cmd_valid		),
    .spi_addr_r				(spi_addr			),
    .spi_addr_valid_r		(spi_addr_valid		),
    .spi_data_r				(spi_data			),
    .spi_data_valid_r		(spi_data_valid		),
    .spi_done				(spi_done			)

);



// spi mosi task
task SPI_WRITE_OUT;
	input [2*SPI_PACKET_WIDTH-1:0] 	SPI_CMD;
	input [SPI_PACKET_WIDTH-1:0] 	SPI_ADDR;
	input [SPI_DATA_WIDTH-1:0] 		SPI_DATA;
	//integer	i, j, q;
	begin
	spi_mosi 						= 1'b1;
	spi_scsn						= 1'b0;
	
	// CMD1
	for (i=0; i<SPI_PACKET_WIDTH; i=i+1) begin
		spi_mosi   	= SPI_CMD[2*SPI_PACKET_WIDTH-i-1]; 
		#SPI_CLK_TIME; 		
		spi_clk 	= ~spi_clk;
		#SPI_CLK_TIME;
		spi_clk 	= ~spi_clk;
	end	
	spi_mosi 						= 1'b1;
	#200;
	
	// CMD0
	for (i=0; i<SPI_PACKET_WIDTH; i=i+1) begin
		spi_mosi   	= SPI_CMD[SPI_PACKET_WIDTH-i-1]; 
		#SPI_CLK_TIME; 		
		spi_clk 	= ~spi_clk;
		#SPI_CLK_TIME;
		spi_clk 	= ~spi_clk;
	end	
	spi_mosi 						= 1'b1;
	#200;
	
	// ADDR
	for (i=0; i<SPI_PACKET_WIDTH; i=i+1) begin
		spi_mosi   	= SPI_ADDR[SPI_PACKET_WIDTH-i-1];  
		#SPI_CLK_TIME; 		
		spi_clk 	= ~spi_clk;
		#SPI_CLK_TIME;
		spi_clk 	= ~spi_clk;
	end
	spi_mosi 						= 1'b1;
	#100;
	spi_sdo							= 40'h96_bd_30_a6_47;
	spi_sdo_valid					= 1'b1;
	#10
	spi_sdo_valid					= 1'b0;
	#90
	
	// DATA
	for (i=0; i<SPI_DATA_BYTES; i=i+1) begin
		for (j=0; j<SPI_PACKET_WIDTH; j=j+1) begin
			//spi_mosi   	= SPI_DATA[(NUM_SLOTS-i-1)*SPI_DATA_WIDTH-j -1];  
			q 			= (SPI_DATA_BYTES - i)*SPI_PACKET_WIDTH - j - 1;
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



//always begin
//	#5 clk <= ~clk;
//end


initial begin
//	clk 						= 1'b0;
	resetn 						= 1'b0;
	spi_scsn					= 1'b1;
	spi_mosi    				= 1'b1;
	spi_clk						= 1'b0;
	spi_sdo						= 'b0;
	spi_sdo_valid				= 1'b0;
	
	#200			
	resetn 						= 1'b1;
	#1000		

	
	spi_sdi_command 			= `C_READ_INTERUPTS;
	spi_sdi_address 			= `SLOT_7_ADDRESS;					
	spi_sdi_data 				= 40'hf0f0f0f0f0;	// 
	spi_scsn					= 1'b0;
	SPI_WRITE_OUT(spi_sdi_command, spi_sdi_address, spi_sdi_data);
	#100;
	spi_scsn					= 1'b1;
	#900;			
		
	//end	
	$display("read interrupt done");
	#2500;
	
	
	
	
	
	////////////////////////////////////////////////////////////////
	// SET_PWM_FREQ, 7 times
	//for (slot=0; slot<NUM_SLOTS; slot=slot+1) begin
		spi_sdi_command 			= `C_SET_PWM_FREQ;
		spi_sdi_address 			= `LED_ADDRESS;					
		spi_sdi_data 				= 40'h01234503e8;
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
		spi_sdi_data 				= 40'h00000001f4;
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
		spi_sdi_data 				= 40'h0000000001;	// 
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
		spi_sdi_data 				= 40'h0000000001;	// 
		spi_scsn					= 1'b0;
		SPI_WRITE_OUT(spi_sdi_command, spi_sdi_address, spi_sdi_data);
		#100;
		spi_scsn					= 1'b1;
		#900;			
		
	//end	
	$display("read interrupt done");

	#500;
	//for (slot=0; slot<NUM_SLOTS; slot=slot+1) begin
		spi_sdi_command 			= `C_READ_INTERUPTS;
		spi_sdi_address 			= `SLOT_1_ADDRESS;					
		spi_sdi_data 				= 40'h0000000000;	// 
		spi_scsn					= 1'b0;
		SPI_WRITE_OUT(spi_sdi_command, spi_sdi_address, spi_sdi_data);
		#100;
		spi_scsn					= 1'b1;
		#900;			
		
	//end	
	$display("read interrupt done");
	#2000;

	#500;	
	//for (slot=0; slot<NUM_SLOTS; slot=slot+1) begin
		spi_sdi_command 			= `C_READ_INTERUPTS;
		spi_sdi_address 			= `SLOT_1_ADDRESS;					
		spi_sdi_data 				= 40'h0000000000;	// 
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
		spi_sdi_data 				= 40'h0000000000;	// 
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
		spi_sdi_data 				= 40'h0000000000;	// 
		spi_scsn					= 1'b0;
		SPI_WRITE_OUT(spi_sdi_command, spi_sdi_address, spi_sdi_data);
		#100;
		spi_scsn					= 1'b1;
		#900;			
		
	//end	
	$display("read interrupt done");
	#500;
	

end



endmodule