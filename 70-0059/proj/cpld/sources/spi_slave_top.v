`include "commands.v"
//  Project:           SPI slave with EFB
//  File:              spi_slave_top.v
//  Title:             spi_slave_top
//  Description:       Top level design file of SPI slave with EFB reference design
//  Engineer:		   PR
`timescale 1ns/ 1ps

module spi_slave_top #(
	parameter QUAD_CNTR_WIDTH 			= 32,
	parameter NUM_INTRPTS_PER_SLOT		= 3,
	parameter NUM_SLOTS 				= 7,
	parameter SPI_DATA_WIDTH 			= 48,
	parameter NUM_DIG_OUT_PER_SLOT		= 2,
	parameter SLOT_TYPE_CONFIG_WIDTH    = 8	
)
(
	input  wire                      					clk,       		// System clock
	input  wire                      					resetn,     	// System asynchronouse reset (active low)
																
	// spi
	input  wire                      					spi_clk,      	// Hard SPI serial clock
	input  wire                      					spi_scsn,      	// Hard SPI chip-select (active low)
	input  wire                      					spi_mosi,     	// Hard SPI serial input data
	output wire                      					spi_miso,       // Hard SPI serial output data
   
 	// used for sdi
	output reg 	[15:0]									spi_cmd_r,
	output reg	[7:0]									spi_addr_r,
	output reg	[39:0]									spi_data_r,
	output reg											spi_data_valid_r,
   
 	// used for sd0
	output wire [15:0]									spi_cmd,
	output wire	[7:0]									spi_addr,
	input wire	[39:0]									spi_data_out_r
   );
   
   localparam SPI_PACKET_WIDTH = 8;   // Memory addrss width
   localparam MAX_MEM_BURST_NUM = 6; // Maximum memory burst number
   
   wire                                   	wb_clk_i;
   wire                                   	wb_rst_i;
   wire                                   	wb_cyc_i;
   wire                                   	wb_stb_i;
   wire                                   	wb_we_i;
   wire [7:0]                             	wb_adr_i;
   wire [7:0]                             	wb_dat_i;
   wire [7:0]                             	wb_dat_o;
   wire                                   	wb_ack_o; 
											
   wire [7:0]                             	address;                       
   wire                                   	wr_en;                               
   wire [7:0]                             	wr_data;                       
   wire                                   	rd_en;                               
   wire [7:0]                             	rd_data;                       
   wire                                   	wb_xfer_done;                           
   wire                                   	wb_xfer_req;                            
   wire                                   	spi_irq;  

   wire                                   	mem_wr;                              
   wire [7:0]				            	mem_addr;       
   wire [7:0]                             	mem_wdata;                              
   wire [7:0]             					mem_rdata;    // Memory read data bus
   wire              						mem_rdata_update;    // Memory read data bus
   reg  [39:0]								spi_sdo_r;
   
   wire										spi_cmd_valid;
   wire										spi_addr_valid;
   wire	[39:0]								spi_data;
   wire										spi_data_valid;
   reg										spi_scsn_dly;

   reg [39:0] 								spi_sdo;
   reg										spi_sdo_valid; 
   
   reg	[39:0]								spi_sdi;	
   reg										spi_addr_valid_r; 
   wire										spi_done;
   
   // Use system clock/reset as the WISHBONE's clock/reset
   assign wb_clk_i = clk;
   assign wb_rst_i = ~resetn;
   
	// ********** SDO **********
	// spi construction
	//  CMD1 | CMD0 | ADDR | DATA_BYTE_4 | DATA_BYTE_3 | DATA_BYTE_2 | DATA_BYTE_1 | DATA_BYTE_0 
	
	// sdo construction
	// DATA_BYTE_4 |  DATA_BYTE_3 | DATA_BYTE_2 | DATA_BYTE_1 | DATA_BYTE_0 
	always @ (posedge clk) begin
		if (!resetn) begin
			spi_sdo 		<= 'b0;
			spi_sdo_valid 	<= 1'b0;
		end	
		else if (spi_addr_valid_r && spi_cmd == `C_READ_CPLD_REV && spi_addr_r == `REVISION_ADDRESS) begin
			spi_sdo <= (`CPLD_REV + (`CPLD_REV_MINOR << 8));
			spi_sdo_valid <= 1'b1;
		end
		else if (spi_addr_valid_r) begin
			spi_sdo <= spi_data_out_r;
			spi_sdo_valid 	<= 1'b1;
		end
		else begin
			spi_sdo		 	<= spi_sdo;	
			spi_sdo_valid 	<= 1'b0;
		end	
	end
   
   // shift spi_sdo at mem_rdata_update
   always @ (posedge clk) begin
		if (!resetn)
			spi_sdo_r <= 'b0;
		else if (spi_sdo_valid)	
			spi_sdo_r <= spi_sdo;
		else if (mem_rdata_update)
			spi_sdo_r <= spi_sdo_r << 8;
		else
			spi_sdo_r <= spi_sdo_r;
   end
   assign mem_rdata = spi_sdo_r[39:32];
  
   always @ (posedge clk) begin
		if (!resetn || spi_done) begin
			spi_cmd_r 			<= 'b0;
//			spi_cmd_valid_r 	<= 1'b0;
			spi_addr_r 			<= 'b0;
			spi_addr_valid_r 	<= 1'b0;
			spi_data_r 			<= 'b0;
			spi_data_valid_r 	<= 1'b0;
		end	
		else if (spi_cmd_valid) begin
			spi_cmd_r 			<= spi_cmd;
	//		spi_cmd_valid_r 	<= spi_cmd_valid; 
			spi_data_valid_r 	<= spi_data_valid;
		end	
		else if (spi_addr_valid) begin 
			spi_addr_r 			<= spi_addr;
			spi_addr_valid_r 	<= spi_addr_valid;
		end	
		else if (spi_data_valid) begin
			spi_data_r 			<= spi_data;	
			spi_data_valid_r 	<= spi_data_valid;
		end	
		else begin
//			spi_cmd_valid_r 	<= 1'b0;
			spi_addr_valid_r 	<= 1'b0;
			spi_data_valid_r 	<= 1'b0;
		end
	end
	
	// spi_done is rising edge of spi_scsn
	always @ (posedge clk) begin
		if (!resetn)
			spi_scsn_dly <= 1'b0;
		else
			spi_scsn_dly <= spi_scsn;
	end
	assign spi_done = spi_scsn & ~spi_scsn_dly;
   
   // SPI slave EFB module instantiation 
   spi_slave_efb spi_slave_efb_inst (
             .wb_clk_i 	(wb_clk_i			), 
             .wb_rst_i 	(wb_rst_i			), 
             .wb_cyc_i 	(wb_cyc_i			), 
             .wb_stb_i 	(wb_stb_i			), 
             .wb_we_i  	(wb_we_i 			),  
             .wb_adr_i 	(wb_adr_i			), 
             .wb_dat_i 	(wb_dat_i			), 
			 .spi_scsn 	(spi_scsn			),
             .wb_dat_o 	(wb_dat_o			), 
             .wb_ack_o 	(wb_ack_o			), 
             .spi_clk  	(spi_clk 			),  
             .spi_miso 	(spi_miso			), 
             .spi_mosi 	(spi_mosi			) 
             );
   
   // WISHBONE control module instantiation           
   wb_ctrl wb_ctrl_inst (
             .wb_clk_i  (wb_clk_i    		),
             .wb_rst_i  (wb_rst_i    		),
             .wb_cyc_i  (wb_cyc_i    		),
             .wb_stb_i  (wb_stb_i    		),
             .wb_we_i   (wb_we_i     		),
             .wb_adr_i  (wb_adr_i    		),
             .wb_dat_i  (wb_dat_i    		),
             .wb_dat_o  (wb_dat_o    		),
             .wb_ack_o  (wb_ack_o    		), 
             .address   (address     		), 
             .wr_en     (wr_en       		),
             .wr_data   (wr_data     		),
             .rd_en     (rd_en       		),    
             .rd_data   (rd_data     		),
             .xfer_done (wb_xfer_done		),
             .xfer_req  (wb_xfer_req 		)
             );   
    
   // spi_ctrl module instantiation        
   spi_ctrl #(
               .SPI_PACKET_WIDTH    (8 			   ),  
               .MAX_MEM_BURST_NUM   (5				   )
               )
   spi_ctrl_inst (
                   .clk            	(clk          		),
                   .rst_n          	(resetn        		),
                   .spi_csn        	(spi_scsn         	),
                   .address        	(address      		), 
                   .wr_en          	(wr_en        		),
                   .wr_data        	(wr_data      		),
                   .rd_en          	(rd_en        		),    
                   .rd_data        	(rd_data      		),
                   .wb_xfer_done   	(wb_xfer_done 		),
                   .wb_xfer_req    	(wb_xfer_req  		),
                   .mem_wr         	(mem_wr     		),
                   .mem_addr       	(mem_addr   		),
                   .mem_wdata      	(mem_wdata  		),
                   .mem_rdata      	(mem_rdata  		),
				   .mem_rdata_update(mem_rdata_update	),
				   .spi_cmd			(spi_cmd			),
				   .spi_cmd_valid	(spi_cmd_valid		),
				   .spi_addr		(spi_addr			),
				   .spi_addr_valid	(spi_addr_valid		),
				   .spi_data		(spi_data			),
				   .spi_data_valid	(spi_data_valid		)
                   );
    
   
endmodule                 