/**
 * this file contains basic definitions and typedefs for general designs.
 */

`ifndef __COMMON_SVH__
`define __COMMON_SVH__

// Vivado does not support string parameters.
`ifdef VERILATOR
`define STRING string
`else
`define STRING
`endif

// simple compile-time assertion
`define ASSERTS(expr, message) \
    if (!(expr)) $error(message);
`define ASSERT(expr) `ASSERTS(expr, "Assertion failed.");

// to ignore some signals
`define UNUSED_OK(list) \
    logic _unused_ok = &{1'b0, {list}, 1'b0};

// basic data types
`define BITS(x) logic[(x)-1:0]

typedef int unsigned uint;

typedef logic     i1;
typedef `BITS(2)  i2;
typedef `BITS(3)  i3;
typedef `BITS(4)  i4;
typedef `BITS(5)  i5;
typedef `BITS(6)  i6;
typedef `BITS(7)  i7;
typedef `BITS(8)  i8;
typedef `BITS(9)  i9;
typedef `BITS(16) i16;
typedef `BITS(19) i19;
typedef `BITS(26) i26;
typedef `BITS(32) i32;
typedef `BITS(33) i33;
typedef `BITS(34) i34;
typedef `BITS(35) i35;
typedef `BITS(36) i36;
typedef `BITS(37) i37;
typedef `BITS(38) i38;
typedef `BITS(39) i39;
typedef `BITS(40) i40;
typedef `BITS(41) i41;
typedef `BITS(42) i42;
typedef `BITS(64) i64;
typedef `BITS(65) i65;
typedef `BITS(66) i66;
typedef `BITS(67) i67;
typedef `BITS(68) i68;

// for arithmetic overflow detection
typedef i33 arith_t;

// all addresses and words are 32-bit
typedef i32 addr_t;
typedef i32 word_t;

// number of bytes transferred in one memory r/w
typedef enum i3 {
    MSIZE1,
    MSIZE2,
    MSIZE4
} msize_t;

// length of a burst transaction
// NOTE: WRAP mode in AXI3 only supports power-of-2 length.
typedef enum i4 {
    MLEN1  = 4'b0000,
    MLEN2  = 4'b0001,
    MLEN4  = 4'b0011,
    MLEN8  = 4'b0111,
    MLEN16 = 4'b1111
} mlen_t;

// a 4-bit mask for memory r/w, namely "write enable"
typedef i4 strobe_t;

// general-purpose register index
typedef i5 regidx_t;

/**
 * AXI-related typedefs
 */

`define include_ram `include "Ram_1w_1ra.v"  \
            `include "Ram_1w_1rs.v" \
            `include "Ram_2wrs.v" 
`endif
