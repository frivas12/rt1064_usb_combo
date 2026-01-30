//  Project:           SPI slave with EFB
//  File:              main_ctrl.v
//  Title:             main_ctrl
//  Description:       Main control module of this reference design for XO2 architecture
//  Engineer:		   ZS

`timescale 1 ns/ 1 ps

module spi_ctrl #( parameter SPI_PACKET_WIDTH = 8,         // Memory addrss width           
                   parameter MAX_MEM_BURST_NUM = 5        // Maximum memory burst number
                   )                                     
   (                                                     
    input  wire                            	clk,          // System clock
    input  wire                            	rst_n,        // System reset
    input  wire                            	spi_csn,      // Hard SPI chip-select (active low)
    output reg  [7:0]                      	address,      // Local address for the WISHBONE interface
    output reg                            	 	wr_en,        // Local write enable for the WISHBONE interface
    output reg  [7:0]                      	wr_data,      // Local write data for the WISHBONE interface        
    output reg                            	 	rd_en,        // Local read enable for the WISHBONE interface          
    input  wire [7:0]                      	rd_data,      // Local read data for the WISHBONE interface          
    input  wire                            	wb_xfer_done, // WISHBONE transfer done    
    input  wire                            	wb_xfer_req,  // WISHBONE transfer request 
	
    output reg                            	 	mem_wr,       // Memory write (high) and read (low)
    output reg  [SPI_PACKET_WIDTH-1:0]     	mem_addr,     // Memory address
    output reg  [SPI_PACKET_WIDTH-1:0]      	mem_wdata,    // Memory write data bus
    input  wire [SPI_PACKET_WIDTH-1:0]      	mem_rdata,    // Memory read data bus
	output reg									mem_rdata_update,	
	
	output reg  [2*SPI_PACKET_WIDTH-1:0] 		spi_cmd,
	output reg									spi_cmd_valid,
	output 		[SPI_PACKET_WIDTH-1:0] 		spi_addr,
	output reg									spi_addr_valid,
 	output reg  [5*SPI_PACKET_WIDTH-1:0] 		spi_data,
	output reg									spi_data_valid
    );
    
    // The definitions for SPI EFB register address
    `define SPITXDR 8'h59
    `define SPISR   8'h5A
    `define SPIRXDR 8'h5B
    
    // The definitions for the state values of the main state machine
    `define S_IDLE      4'h0
    `define S_RXDR_RD   4'h1
    `define S_TXDR_WR   4'h2
    `define S_CMD_ST    4'h3
    `define S_CMD_LD    4'h4
    `define S_CMD_DEC   4'h5
    `define S_TXDR_WR1  4'h6
    `define S_ADDR_ST   4'h7
    `define S_ADDR_LD   4'h8
    `define S_WDATA_ST  4'h9
    `define S_DATA_WR   4'hA  
    `define S_DATA_RD   4'hB    
    `define S_RDATA_ST  4'hC

	
    `define REV_ID     16'h0B  
	
    reg [3:0]  main_sm;           // The state register of the main state machine
    reg        spi_csn_buf0_p;    // The postive-egde sampling of spi_csn
    reg        spi_csn_buf1_p;    // The postive-egde sampling of spi_csn_buf0_p 
    reg        spi_csn_buf2_p;    // The postive-egde sampling of spi_csn_buf1_p
    wire       spi_cmd_start;     // A new SPI command start
    reg        spi_cmd_start_reg; // The buffer of a new SPI command start
    reg        spi_idle;          // SPI IDLE signal    
    wire       spi_rx_rdy;        // SPI receive ready    
    wire       spi_tx_rdy;        // SPI transmit ready             
    wire       spi_xfer_done;     // SPI transmitting complete (1: complete, 0: in progress) 
    reg  [7:0] mem_burst_cnt;
	
	
	reg [3:0]  spi_byte_cnt;
	reg        spi_cmd_cnt;
	
	assign spi_addr = mem_addr;
	
	always @ (posedge clk) begin
		if (!rst_n) 
			mem_rdata_update <= 1'b0;
		else if (main_sm == `S_RDATA_ST)
			mem_rdata_update <= wr_en;
		else 
			mem_rdata_update <= 1'b0;
	end
	
	
    /////////////////////////////////////////////////////////////////////////////////////////////////
	
    // Bufferring spi_csn with postive edge                    
    always @(posedge clk or negedge rst_n)
       if (!rst_n)
          spi_csn_buf0_p <= 1'b1;
       else
          spi_csn_buf0_p <= spi_csn;
              
    // Bufferring spi_csn_buf0_p with postive edge                    
    always @(posedge clk or negedge rst_n)
       if (!rst_n)
          spi_csn_buf1_p <= 1'b1;
       else
          spi_csn_buf1_p <= spi_csn_buf0_p;

    // Bufferring spi_csn_buf1_p with postive edge 
    always @(posedge clk or negedge rst_n)
       if (!rst_n)
          spi_csn_buf2_p <= 1'b1;
       else
          spi_csn_buf2_p <= spi_csn_buf1_p;
       
    // Generate SPI command start buffer signal                 
    always @(posedge clk or negedge rst_n)
       if (!rst_n)
          spi_cmd_start_reg <= 1'b0;
       else
          if (spi_csn_buf2_p && !spi_csn_buf1_p)
             spi_cmd_start_reg <= 1'b1;
          else if (main_sm == `S_IDLE || main_sm == `S_RXDR_RD || (!spi_csn_buf2_p && spi_csn_buf1_p))
             spi_cmd_start_reg <= 1'b0;
    
    // Generate SPI command start signal
    assign spi_cmd_start = (spi_csn_buf2_p & ~spi_csn_buf1_p) | spi_cmd_start_reg;
    
    // spi_idle will be asserted between spi_csn de-asserted and main_sm == `S_IDLE                    
    always @(posedge clk or negedge rst_n)
       if (!rst_n)
          spi_idle <= 1'b0;
       else 
          if (spi_csn_buf1_p)
             spi_idle <= 1'b1;
          else if (main_sm == `S_IDLE)   
             spi_idle <= 1'b0;
    
    assign spi_xfer_done = (~spi_csn_buf2_p & spi_csn_buf1_p) | spi_idle;
             
    assign spi_rx_rdy = rd_data[3] ? 1'b1 : 1'b0;
    assign spi_tx_rdy = rd_data[4] ? 1'b1 : 1'b0;
    
    //The main state machine with its output registers      
    always @(posedge clk or negedge rst_n)
       if (!rst_n) begin
          main_sm <= `S_IDLE;
          spi_cmd <= `REV_ID;
          rd_en <= 1'b0;
          wr_en <= 1'b0;
          address <= 8'h59;
          wr_data <= 8'b0;
          mem_wr <= 1'b0;
          mem_addr <= 'b0;
          mem_wdata <= 'b0;
          mem_burst_cnt <= 'b0;
		  spi_cmd_valid		<= 1'b0;
		  spi_addr_valid 	<= 1'b0;
		  spi_data			<= 'b0;
		  spi_data_valid	<= 1'b0;
		  spi_byte_cnt		<= 'b0;
		  spi_cmd_cnt		<= 1'b0;
       end else begin
          rd_en <= 1'b0;
          wr_en <= 1'b0;
          mem_wr <= 1'b0;
          
          address <= 8'h59;
          
          case (main_sm)
          // IDEL state
          `S_IDLE:     begin
							spi_cmd_valid	<= 1'b0;
							spi_addr_valid 	<= 1'b0;
							spi_data_valid 	<= 1'b0;
							spi_data		<= 'b0;
						if (spi_cmd_start && wb_xfer_req) begin
                          main_sm <= `S_RXDR_RD;            // Go to `S_RSDR_RD state when a new SPI command starts and
                                                            // WISHBONE is ready to transfer
                          rd_en <= 1'b1;                        
                          address <= `SPIRXDR;
                       end
					   end
          // Read SPI EFB RXDR register first to get ready to read the SPI command next
          `S_RXDR_RD:  if (wb_xfer_done) begin
                          main_sm <= `S_TXDR_WR;            // Go to `S_TXDR_WR state when the RXDR register read is done
                          wr_en <= 1'b1; 
                          address <= `SPITXDR; 
                       end
          // Write dummy data to the SPI EFB TXDR register in order to write next data to the register correctly       
          `S_TXDR_WR:  if (wb_xfer_done) begin                
                          main_sm <= `S_CMD_ST;             // Go to `S_CMD_ST state when the TXDR register write is done
                          rd_en <= 1'b1;
                          address <= `SPISR;
                       end
          // Wait for the SPI command is ready in the RXDR register                                              
          `S_CMD_ST:   begin 
                          if (wb_xfer_done && spi_rx_rdy) begin  
                             main_sm <= `S_CMD_LD;          // Go to `S_CMD_LD state when the SPI command is ready in the RXDR register
                             rd_en <= 1'b1;                        
                             address <= `SPIRXDR;
                          end else if (wb_xfer_done && spi_xfer_done)   
                             main_sm <= `S_IDLE;            // Go to `S_IDLE state when the SPI transfer is complete 
                          else if (wb_xfer_done && spi_tx_rdy) begin
                             main_sm <= `S_TXDR_WR;         // Go to `S_TXDR_WR state to rewrite the TXDR register when SPI transmit is ready
                             wr_en <= 1'b1; 
                             address <= `SPITXDR;                              
                          end else if (wb_xfer_done) begin
                             rd_en <= 1'b1;                 // Otherwise, keep read SR register in the current state
                             address <= `SPISR;
                          end  
                       end 
          // Load the SPI command to improve the performance because the path delay from the RXDR register is very big 
          `S_CMD_LD:   if (wb_xfer_done) begin
                          main_sm <= `S_CMD_DEC;            // Go to `S_CMD_DEC state when the RXDR register read is done
                          if (!spi_cmd_cnt)
							spi_cmd[15:8] <= rd_data; 
						  else
							spi_cmd[7:0] <= rd_data; 
						  spi_cmd_valid <= (spi_cmd_cnt == 1'b1) ? 1'b1 : 1'b0;
                       end
          // Decode the SPI command              
          `S_CMD_DEC:  begin
							spi_cmd_valid <= 1'b0;
							main_sm <= (spi_cmd_cnt == 1'b1) ? `S_TXDR_WR1 : `S_TXDR_WR; // Go to `S_TXDR_WR1 state when the SPI command is Write Memory 
                            wr_en <= 1'b1; 
                            address <= `SPITXDR; 
							spi_cmd_cnt <= ~spi_cmd_cnt;
                          if (spi_xfer_done) begin        
                             main_sm <= `S_IDLE;               // Go to `S_IDLE state when the current SPI transaction is ended
                             rd_en <= 1'b0;
                             wr_en <= 1'b0;
                          end                           

                          mem_burst_cnt <= 'b0;
                                                                
                       end   
          // For GPIO/memory commands, write dummy data to the SPI EFB TXDR register in order to write next data to the register correctly.
          // For IRQ_ST/REV_ID commands, write their data to the SPI EFB TXDR register.                
          `S_TXDR_WR1: if (wb_xfer_done) begin
                          main_sm <= `S_ADDR_ST;               // Go to `S_ADDR_ST state when the TXDR register write is done
                          rd_en <= 1'b1;
                          address <= `SPISR;
                       end
          // For GPIO/memory commands, wait for the address ready in the RXDR register.
          // For IRQ_ST/REV_ID commands, wait for the data write done             
          `S_ADDR_ST:  begin 
                          if (wb_xfer_done && spi_rx_rdy) begin
                              main_sm <= `S_ADDR_LD;           // Go to `S_ADDR_LD state when the address is ready in the RXDR register
                              rd_en <= 1'b1;                        
                              address <= `SPIRXDR;
                              
                              if (spi_xfer_done) begin
                                 main_sm <= `S_IDLE;           // Go to `S_IDLE state when the current SPI transaction is complete
                                 rd_en <= 1'b0;
                              end   
                          end else if (wb_xfer_done && spi_xfer_done)   
                             main_sm <= `S_IDLE;               // Go to `S_IDLE state when the SPI transfer is complete
                          else if (wb_xfer_done && spi_tx_rdy) begin
                             main_sm <= `S_TXDR_WR1;           // Go to `S_TXDR_WR1 state to rewrite the TXDR register when SPI transmit is ready
                             wr_en <= 1'b1; 
                             address <= `SPITXDR; 
                          end else if (wb_xfer_done) begin
                             rd_en <= 1'b1;                    // Otherwise, keep read SR register in the current state
                             address <= `SPISR;
                          end  
                       end 
          // For GPIO/memory commands, load address.
          // For IRQ_ST/REV_ID commands, go to `S_IDLE state.                
          `S_ADDR_LD:  if (wb_xfer_done) begin
						  spi_addr_valid <= 1'b1;
						  spi_byte_cnt  <= 'b0;
                          case (spi_cmd[15]) 
                          1'b0:     begin 
                                          main_sm <= `S_WDATA_ST; // Go to `S_WDATA_ST state when the SPI command is Write Memory 
                                          mem_addr <= rd_data[SPI_PACKET_WIDTH-1:0];  
                                          rd_en <= 1'b1; 
                                          address <= `SPISR; 
                                       end	   
                          1'b1:     begin 
                                          main_sm <= `S_DATA_RD;  // Go to `S_DATA_RD state when the SPI command is Read Memory
                                          mem_addr <= rd_data[SPI_PACKET_WIDTH-1:0];  
                                          rd_en <= 1'b1; 
                                          address <= `SPISR; 
                                       end 
                          endcase
                       end
          // Wait for the SPI write data ready in the RXDR register       
          `S_WDATA_ST: begin
							spi_addr_valid <= 1'b0;
							spi_data_valid <= 1'b0;
                          if (wb_xfer_done && spi_rx_rdy) begin
                             main_sm <= `S_DATA_WR;            // Go to `S_DATA_WR state when the SPI write data is ready in the RXDR register
                             rd_en <= 1'b1;                        
                             address <= `SPIRXDR;
                          end else if (wb_xfer_done && spi_xfer_done)   
                             main_sm <= `S_IDLE;               // Go to `S_IDLE state when the SPI transfer is complete
                          else if (wb_xfer_done) begin
                             rd_en <= 1'b1;                    // Otherwise, keep read SR register in the current state
                             address <= `SPISR;
                          end  
                 
                          if (mem_wr) begin
                             mem_addr <= mem_addr + 1;
                             spi_byte_cnt <= spi_byte_cnt + 1;
                             if (mem_burst_cnt !== MAX_MEM_BURST_NUM)
                                mem_burst_cnt <= mem_burst_cnt + 1;
                          end      
                       end
          // Load SPI data                             
          `S_DATA_WR:  if (wb_xfer_done) begin
                          case (spi_cmd[15]) 
						   
                          1'b0:     begin 
                                          if (spi_xfer_done) begin        
                                             main_sm <= `S_IDLE;     // Go to `S_IDLE state when the SPI command is Write Memory and
                                                                     // the current SPI transaction is ended
                                             rd_en <= 1'b0;                      
                                          end else begin
                                             main_sm <= `S_WDATA_ST; // Go to `S_WDATA_ST state when the SPI command is Write Memory but
                                                                     // the current SPI transaction is not ended
                                             rd_en <= 1'b1;
                                          end
                                       
                                          address <= `SPISR; 
                                                                                                         
                                          mem_wdata <= rd_data;
										  
										  case (spi_byte_cnt)
											//4'h0: spi_data[47:40]	<= rd_data;
											4'h0: spi_data[39:32]	<= rd_data;	// dummy
											4'h1: spi_data[31:24]	<= rd_data;
											4'h2: spi_data[23:16]	<= rd_data;
											4'h3: spi_data[15:8]	<= rd_data;
											4'h4: spi_data[7:0]	<= rd_data;
											default: spi_data		<= spi_data;
										  endcase
										  
                                          
                                          if (mem_burst_cnt !== MAX_MEM_BURST_NUM)
                                             mem_wr <= 1'b1; 
                                          else
                                             mem_wr <= 1'b0;
											 spi_data_valid <= (spi_byte_cnt == MAX_MEM_BURST_NUM-1) ? 1'b1 : 1'b0;
                                       end 
                          endcase
                       end
          // Write the SPI read data to the TXDR register    
          `S_DATA_RD:  begin
							spi_addr_valid <= 1'b0;
                          if (wb_xfer_done && spi_tx_rdy) begin
                             case (spi_cmd[15])
								   
                             1'b1:  begin 
                                          main_sm <= `S_RDATA_ST; // Go to `S_RDATA_ST state when the SPI command is Read Memory
                                          wr_en <= 1'b1; 
                                          address <= `SPITXDR;
                                          
                                          if (mem_burst_cnt !== MAX_MEM_BURST_NUM)
                                             wr_data <= mem_rdata; // Write memory read data when memory burst number is no more than MAX_MEM_BURST_NUM 
                                          else
                                             wr_data <= 8'hFF;     // Write all '1's when memory burst number is more than MAX_MEM_BURST_NUM
                                       end
                             endcase  
                                                  
                             if (spi_xfer_done) begin
                                main_sm <= `S_IDLE;               // Go to `S_IDLE state when the current SPI transaction is complete
                                wr_en <= 1'b0;
                             end   
                          end else if (wb_xfer_done && spi_xfer_done)   
                             main_sm <= `S_IDLE;                  // Go to `S_IDLE state when the current SPI transaction is complete
                          else if (wb_xfer_done) begin
                             rd_en <= 1'b1;
                             address <= `SPISR;
                          end   
                       end 
          // Wait for the SPI read data to be transmitted from the TXDR register
          `S_RDATA_ST: begin 
                          if (wb_xfer_done) begin
                             main_sm <= `S_DATA_RD;               // Go to `S_DATA_RD state when the SPI read data is transmitted from in the TXDR register
                             rd_en <= 1'b1;
                             address <= `SPISR;
                          end  
                          
                          if (wr_en) begin 
                             mem_addr <= mem_addr + 1; 
                             spi_byte_cnt <= spi_byte_cnt + 1;
                             if (mem_burst_cnt !== MAX_MEM_BURST_NUM)
                                mem_burst_cnt <= mem_burst_cnt + 1; 
                          end      
                       end   
                                         
          default: main_sm <= `S_IDLE;                                
          endcase              
       end

    
endmodule     
    
    
    