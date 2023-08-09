package cat.utils

import spinal.core._
import spinal.lib._

case class UnitControlSignals() extends Bundle with IMasterSlave {
  val start = Bool()
  val done  = Bool()
  val busy  = Bool()
  val reset = Bool()

  override def asMaster(): Unit = {
    out(start, reset)
    in(done, busy)
  }

  def fire = start && !busy
}

case class MemoryPort(
    val readOnly: Boolean = false,
    val dataWidth: Int = 32,
    val addressWidth: Int = 32
) extends Bundle
    with IMasterSlave {
  val read = new Bundle {
    val enable  = Bool()
    val address = UInt(addressWidth bits)
    val data    = Bits(dataWidth bits)
  }

  val write =
    !readOnly generate
      new Bundle {
        val address = UInt(addressWidth bits)
        val data    = Bits(dataWidth bits)
        val mask    = Bits(dataWidth / 8 bits)
      }

  override def asMaster(): Unit = {
    in(read.data)
    out(read.enable, read.address)
    if (!readOnly) {
      out(write.address, write.data, write.mask)
    }
  }
}
