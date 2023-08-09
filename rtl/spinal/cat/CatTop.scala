package cat

import spinal.core._
import spinal.lib._

case class CatTop() extends Component {

  class DataRegisterInterface extends Bundle with IMasterSlave {
    val readData    = in Bits (32 bits)
    val writeData   = out Bits (32 bits)
    val writeEnable = out Bool ()

    def asMaster(): Unit = {
      in(readData)
      out(writeData, writeEnable)
    }
  }

  val io = new Bundle {
    // PS/PL CS Register Interface
    val status  = out Bits (8 bits)
    val control = in Bits (8 bits)
    val info    = new Bundle {
      val readData    = in Bits (16 bits)
      val writeData   = out Bits (16 bits)
      val writeEnable = out Bool ()
    }
    // PS/PL Data Register Interface
    val data    = Vec(master(new DataRegisterInterface), 8)
  }

  val currentStatus = Reg(Status()).init(Status.IDLE)

  /*
   * For encoding process, the data of unencoded image is stored in the decodedMemory.
   * For decoding process, the data of undercoded image is stored in the encodedMemory.
   * Before the encoding/decoding process starts, the data should be written into the memory we declared above.
   * Other memories can be leaved uninitialized.
   */
  // val decodedMemory = Mem(Bits(32 bits), CatConfig.maximumTileSizeInBytes)
  // val encodedMemory = Mem(Bits(32 bits), CatConfig.maximumTileSizeInBytes)
  // val hashTable     = Mem(UInt(32 bits), CatConfig.hashTableSize)

  // IO Assignment
  io.status := currentStatus.asBits.resized
  io.info.writeData := 0
  io.info.writeEnable := False

  for (i <- 0 until io.data.size) {
    io.data(i).writeData := 0
    io.data(i).writeEnable := False
  }
}
