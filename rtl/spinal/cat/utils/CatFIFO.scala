package cat.utils

import spinal.core._
import spinal.lib._

/*
 * CatFIFO
 *
 * CatFIFO is a special FIFO designed for Cat Encoder/Decoder, it can push 0 or 4 bytes of data per cycle,
 * and pop 0 to 4 bytes of data. push and pop can happen at the same time.
 *
 */
case class CatFIFO(
    val depth: Int = 8
) extends Component {
  require(depth > 4 && isPow2(depth))
  val fifoAddrWidth = log2Up(depth)

  val io = new Bundle {
    val push = new Bundle {
      val data  = in Vec (Bits(8 bits), 4)
      val valid = in Bool ()
      val ready = out Bool ()

      def fire = valid && ready
    }

    val pop = new Bundle {
      val data   = out Vec (Bits(8 bits), 4)
      val valid  = out Bool ()
      val ready  = in Bool ()
      val number = in UInt (3 bits)

      def fire = valid && ready
    }

    val invalidateAll = in Bool ()

    val occupancy = out UInt (log2Up(depth + 1) bits)
  }

  val fifoMemory = Vec(Reg(Bits(8 bits)) init (0), depth)

  // The popAddress is the address of the next byte to be popped.
  // The pushAddress is similar.
  val popAddress  = Reg(UInt(fifoAddrWidth bits)) init (0)
  val pushAddress = Reg(UInt(fifoAddrWidth bits)) init (0)

  val occupancy = Reg(UInt(log2Up(depth + 1) bits)) init (0)

  val pushBytesNumber = (io.push.fire ? U(4) | U(0)).resized
  val popBytesNumber  = (io.pop.fire ? io.pop.number | U(0)).resized

  val occupancyNext = occupancy + pushBytesNumber - popBytesNumber

  occupancy := occupancyNext

  // Update FIFO memory when push happens
  when(io.push.fire) {
    for (i <- 0 until 4) {
      fifoMemory(pushAddress + i) := io.push.data(i)
    }
  }

  // Update address when push or pop happens
  when(io.push.fire) {
    pushAddress := pushAddress + pushBytesNumber.resized
  }

  when(io.pop.fire) {
    popAddress := popAddress + popBytesNumber.resized
  }

  when(io.invalidateAll) {
    popAddress    := 0
    pushAddress   := 0
    occupancyNext := 0
  }

  // IO Assignment
  io.push.ready := occupancy < (depth - 4) && !io.invalidateAll
  io.pop.valid  := occupancy > io.pop.number

  for (i <- 0 until 4) {
    io.pop.data(i) := fifoMemory(popAddress + i)
  }
  io.occupancy := occupancy
}
