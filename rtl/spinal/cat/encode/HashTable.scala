package cat.encode

import spinal.core._
import spinal.lib._
import spinal.lib.fsm._
import cat.utils.MemoryPort

case class HashTable(
    val keyWidth: Int = 32,
    val valueWidth: Int = 32,
    val addressWidth: Int = 16
) extends Component {
  val io = new Bundle {
    val read = new Bundle {
      val key   = in UInt (keyWidth bits)
      val value = out UInt (valueWidth bits)
    }

    val update = new Bundle {
      val enable = in Bool ()
      val key    = in UInt (keyWidth bits)
      val value  = in UInt (valueWidth bits)
    }

    val hashMemoryPort = master(
      MemoryPort(
        readOnly = false,
        dataWidth = valueWidth,
        addressWidth = addressWidth
      )
    )
  }

  private val hashSeed      = U("32'h9e3779b9").resize(keyWidth bits)
  def hash(key: UInt): UInt = {
    (key ^ hashSeed)(31 downto (32 - addressWidth)).resize(addressWidth bits)
  }

  val depth = 1 << addressWidth

  val readArea = new Area {
    io.read.value                  := io.hashMemoryPort.read.data.asUInt
    io.hashMemoryPort.read.address := hash(io.read.key)
    io.hashMemoryPort.read.enable  := True
  }

  val updateArea = new Area {
    io.hashMemoryPort.write.data    := io.update.value.asBits
    io.hashMemoryPort.write.address := hash(io.update.key)
    io.hashMemoryPort.write.mask    := B"1111"
  }
}
