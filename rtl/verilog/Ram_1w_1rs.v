`ifndef __RAM_1W_1RS_V
`define __RAM_1W_1RS_V

`ifdef VERILATOR
`include "common.svh"
`else

`endif
module Ram_1w_1rs #(
    parameter wordCount = 512,
    parameter wordWidth = 32,
    parameter clockCrossing = 1'b0,
    parameter technology = "auto",  // MEM_TYPE = 0
    parameter readUnderWrite = "writeFirst",
    parameter wrAddressWidth = 9,
    parameter wrDataWidth = 32,
    parameter wrMaskWidth = 1,
    parameter wrMaskEnable = 1'b0,
    parameter rdAddressWidth = 9,
    parameter rdDataWidth = 32,

    localparam byteWidth = wordWidth / wrMaskWidth
)(
    input  wire                      wr_clk,
    input  wire                      wr_en,
    input  wire [   wrMaskWidth-1:0] wr_mask,
    input  wire [wrAddressWidth-1:0] wr_addr,
    input  wire [   wrDataWidth-1:0] wr_data,
    input  wire                      rd_clk,
    input  wire                      rd_en,
    input  wire [rdAddressWidth-1:0] rd_addr,
    output wire [   rdDataWidth-1:0] rd_data
);

    RAM_SimpleDualPort #(
        .ADDR_WIDTH(wrAddressWidth),
        .DATA_WIDTH(wordWidth),
        .BYTE_WIDTH(byteWidth),
        .READ_LATENCY(1)
    ) RAM_SimpleDualPort_inst (
        .clk    (wr_clk),
        .en     (wr_en),
        .raddr  (rd_addr),
        .waddr  (wr_addr),
        .strobe (wr_mask),
        .wdata  (wr_data),
        .rdata  (rd_data)
    );

endmodule
`endif 