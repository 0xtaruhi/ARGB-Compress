package cat.utils

import spinal.core._
import spinal.lib._

case class UnitControlSignals(
  val resetActiveLevel: Polarity = HIGH,
) extends Bundle with IMasterSlave {
  val start = Bool()
  val done  = Bool()
  val busy  = Bool()
  val reset = Bool()

  override def asMaster(): Unit = {
    out(start, reset)
    in(done, busy)
  }

  def setResetActive() = {
    reset := { if (resetActiveLevel == HIGH) True else False }
  }

  def setResetInactive() = {
    reset := { if (resetActiveLevel == HIGH) False else True }
  }

  def fire = start && !busy
}

case class MemoryPort(
    val readOnly: Boolean = false,
    val dataWidth: Int = 32,
    val addressWidth: Int = 32,
    val useByteMask: Boolean = true
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
        val mask    = useByteMask generate Bits(dataWidth / 8 bits)
        val enable  = !useByteMask generate Bool()
      }

  override def asMaster(): Unit = {
    in(read.data)
    out(read.enable, read.address)
    if (!readOnly) {
      out(write.address, write.data)
      if (useByteMask) {
        out(write.mask)
      } else {
        out(write.enable)
      }
    }
  }
}
