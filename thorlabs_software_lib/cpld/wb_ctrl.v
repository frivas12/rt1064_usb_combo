// --------------------------------------------------------------------
// >>>>>>>>>>>>>>>>>>>>>>>>> COPYRIGHT NOTICE <<<<<<<<<<<<<<<<<<<<<<<<<
// --------------------------------------------------------------------
// Copyright (c) 2010 by Lattice Semiconductor Corporation
// --------------------------------------------------------------------
//
// Permission:
//
//   Lattice Semiconductor grants permission to use this code for use
//   in synthesis for any Lattice programmable logic product.  Other
//   use of this code, including the selling or duplication of any
//   portion is strictly prohibited.
//
// Disclaimer:
//
//   This VHDL or Verilog source code is intended as a design reference
//   which illustrates how these types of functions can be implemented.
//   It is the user's responsibility to verify their design for
//   consistency and functionality through the use of formal
//   verification methods.  Lattice Semiconductor provides no warranty
//   regarding the use or functionality of this code.
//
// --------------------------------------------------------------------
//           
//                     Lattice Semiconductor Corporation
//                     5555 NE Moore Court
//                     Hillsboro, OR 97214
//                     U.S.A
//
//                     TEL: 1-800-Lattice (USA and Canada)
//                          503-268-8001 (other locations)
//
//                     web: http://www.latticesemi.com/
//                     email: techsupport@latticesemi.com
//
// --------------------------------------------------------------------
//
//  Project:           SPI slave with EFB
//  File:              wb_ctrl.v
//  Title:             wb_ctrl
//  Description:       Wishbone control module of this reference design for XO2 architecture
//
// --------------------------------------------------------------------
// Code Revision History :
// --------------------------------------------------------------------
// Ver: | Author   | Mod. Date  | Changes Made:
// V1.0 | H.C.     | 2012-03-09 | Initial Release
//
// --------------------------------------------------------------------

`timescale 1 ns/ 1 ps

module wb_ctrl (
    input  wire       wb_clk_i, // WISHBONE clock 
    input  wire       wb_rst_i, // WISHBONE reset
    output reg        wb_cyc_i, // WISHBONE bus cycle
    output reg        wb_stb_i, // WISHBONE strobe
    output reg        wb_we_i,  // WISHBONE write/read control
    output reg  [7:0] wb_adr_i, // WISHBONE address
    output reg  [7:0] wb_dat_i, // WISHBONE input data
    input  wire [7:0] wb_dat_o, // WISHBONE output data
    input  wire       wb_ack_o, // WISHBONE transfer acknowledge
    input  wire [7:0] address,  // Local address
    input  wire       wr_en,    // Local write enable
    input  wire [7:0] wr_data,  // Local write data
    input  wire       rd_en,    // Local read enable
    output wire [7:0] rd_data,  // Local read data
    output wire       xfer_done,// WISHBONE transfer done
    output wire       xfer_req  // WISHBONE transfer request
    );
    
    // Defines states for WISHBONE state machine   
    `define IDLE  1'b0
    `define WORK  1'b1
    
    reg wb_sm;  // The state register for WISHBONE state machine
    
    // WISHBONE control state machine 
    always @(posedge wb_clk_i or posedge wb_rst_i)
       if (wb_rst_i)
          wb_sm <= `IDLE;
       else
          case (wb_sm)
          // No transfer
          `IDLE:  if (wr_en | rd_en)
                     wb_sm <= `WORK;  // Go to `WORK state when the WISHBONE write or read transaction is enabled
          // WISHBONE transfer                                
          `WORK:  if (wb_ack_o)       // Go to `IDLE state when the WISHBONE transfer is acknowledgeed
                     wb_sm <= `IDLE;
          endcase              
    
    // Assign WISHBONE control signals
    always @(posedge wb_clk_i or posedge wb_rst_i)
       if (wb_rst_i) begin
          wb_cyc_i <= 1'b0;
          wb_stb_i <= 1'b0;
          wb_we_i  <= 1'b0;
          wb_adr_i <= 8'd0;
          wb_dat_i <= 8'd0;
       end else 
          case (wb_sm)
          `IDLE:   if (wr_en) begin
                      wb_cyc_i <= #1 1'b1;   // delay 1 ns to avoid simulation/hardware mismatch
                      wb_stb_i <= #1 1'b1;   // delay 1 ns to avoid simulation/hardware mismatch
                      wb_we_i  <= 1'b1;
                      wb_adr_i <= address;
                      wb_dat_i <= wr_data;
                   end else if (rd_en) begin
                      wb_cyc_i <= #1 1'b1;   // delay 1 ns to avoid simulation/hardware mismatch
                      wb_stb_i <= #1 1'b1;   // delay 1 ns to avoid simulation/hardware mismatch
                      wb_we_i  <= 1'b0;
                      wb_adr_i <= address;
                   end else begin
                      wb_cyc_i <= 1'b0;
                      wb_stb_i <= 1'b0;
                      wb_we_i  <= 1'b0;
                   end 
                      
          `WORK:   if (wb_ack_o) begin
                      wb_cyc_i <= 1'b0;
                      wb_stb_i <= 1'b0;
                      wb_we_i  <= 1'b0;
                   end 
                     
          endcase              
              
    assign xfer_done = wb_ack_o;
    assign rd_data = wb_dat_o;
    assign xfer_req = ((wb_sm == `IDLE) && !wr_en && !rd_en) || ((wb_sm == `WORK) && wb_ack_o) ? 1'b1 : 1'b0;
 
endmodule     
    
    
    
