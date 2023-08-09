package cat.encode

import spinal.core._
import spinal.lib._
import spinal.lib.fsm._

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

    val update = Vec(
      new Bundle {
        val enable = in Bool ()
        val key    = in UInt (keyWidth bits)
        val value  = in UInt (valueWidth bits)
      },
      1
    )
  }

  private val hashSeed      = U(0x9b832e1).resize(keyWidth bits)
  def hash(key: UInt): UInt = {
    (key ^ hashSeed).resize(addressWidth bits)
  }

  val depth      = 1 << addressWidth
  val hashMemory = Mem(UInt(valueWidth bits), depth)

  val readArea = new Area {
    io.read.value := hashMemory.readSync(hash(io.read.key))
    hashMemory.readSync(0)
  }

  val updateArea = new Area {
    for (update <- io.update) {
      hashMemory.write(hash(update.key), update.value, update.enable)
    }
  }
}
