package cat.encode

import spinal.core._
import spinal.lib._
import spinal.lib.fsm._
import cat.utils.MemoryPort

case class HashTable(
    val keyWidth: Int = 12,
    val valueWidth: Int = 24,
    val addressWidth: Int = 12
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
        addressWidth = addressWidth,
        useByteMask = false
      )
    )
  }

  private val hashSeed      = U("32'h9e3779b9").resize(keyWidth bits)
  def hash(key: UInt): UInt = {
    if (key.getWidth == 24 && addressWidth == 12) {
      (key(21 downto 14) ^ key(9 downto 2)) @@ (key(23 downto 22) ^ key(
        13 downto 12
      )) @@ (key(1 downto 0) ^ key(11 downto 10))
    } else {
      (key ^ hashSeed).resize(addressWidth bits)
    }
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
    io.hashMemoryPort.write.enable  := io.update.enable
  }
}
