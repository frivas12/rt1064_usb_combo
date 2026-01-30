// Verilog netlist produced by program LSE :  version Diamond (64-bit) 3.11.0.396.4
// Netlist written on Mon Jul 27 13:33:53 2020
//
// Verilog Description of module peizo_elliptec
//

module peizo_elliptec (clk, resetn, rx, tx, spi_cmd_r, spi_addr_r, 
            spi_data_r, spi_data_valid_r, uart_slot_en, rx_slot, tx_slot) /* synthesis syn_module_defined=1 */ ;   // c:/users/elieser/documents/cpld/sources/slot_cards/peizo_elliptec.v(5[8:22])
    input clk;   // c:/users/elieser/documents/cpld/sources/slot_cards/peizo_elliptec.v(10[20:23])
    input resetn;   // c:/users/elieser/documents/cpld/sources/slot_cards/peizo_elliptec.v(11[20:26])
    input rx;   // c:/users/elieser/documents/cpld/sources/slot_cards/peizo_elliptec.v(13[19:21])
    output tx;   // c:/users/elieser/documents/cpld/sources/slot_cards/peizo_elliptec.v(14[20:22])
    input [15:0]spi_cmd_r;   // c:/users/elieser/documents/cpld/sources/slot_cards/peizo_elliptec.v(18[24:33])
    input [7:0]spi_addr_r;   // c:/users/elieser/documents/cpld/sources/slot_cards/peizo_elliptec.v(19[23:33])
    input [39:0]spi_data_r;   // c:/users/elieser/documents/cpld/sources/slot_cards/peizo_elliptec.v(20[24:34])
    input spi_data_valid_r;   // c:/users/elieser/documents/cpld/sources/slot_cards/peizo_elliptec.v(21[19:35])
    input [-1:0]uart_slot_en;   // c:/users/elieser/documents/cpld/sources/slot_cards/peizo_elliptec.v(24[38:50])
    output rx_slot;   // c:/users/elieser/documents/cpld/sources/slot_cards/peizo_elliptec.v(25[20:27])
    input tx_slot;   // c:/users/elieser/documents/cpld/sources/slot_cards/peizo_elliptec.v(26[20:27])
    
    
    wire GND_net, VCC_net;
    
    VHI i24 (.Z(VCC_net));
    TSALL TSALL_INST (.TSALL(GND_net));
    VLO i5 (.Z(GND_net));
    PUR PUR_INST (.PUR(VCC_net));
    defparam PUR_INST.RST_PULSE = 1;
    OBZ tx_pad (.I(GND_net), .T(VCC_net), .O(tx));   // c:/users/elieser/documents/cpld/sources/slot_cards/peizo_elliptec.v(30[8:10])
    OBZ rx_slot_pad (.I(GND_net), .T(VCC_net), .O(rx_slot));   // c:/users/elieser/documents/cpld/sources/slot_cards/peizo_elliptec.v(31[8:15])
    GSR GSR_INST (.GSR(VCC_net));
    
endmodule
//
// Verilog Description of module TSALL
// module not written out since it is a black-box. 
//

//
// Verilog Description of module PUR
// module not written out since it is a black-box. 
//

