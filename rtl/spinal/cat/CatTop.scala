package cat

import spinal.core._
import spinal.lib._
import cat.utils.UnalignedReadWriteMemory
import cat.utils.AlignedReadWriteMemory

case class CatTop() extends Component {
  val io = new Bundle {
    val dataReg = new Bundle {
      val readData    = in Bits (32 bits)
      val writeData   = out Bits (32 bits)
      val writeEnable = out Bool ()
      val regIndex    = out UInt (3 bits)
    }

    val status  = out Bits (8 bits)
    val control = in Bits (8 bits)

    val info = new Bundle {
      val readData    = in Bits (16 bits)
      val writeData   = out Bits (16 bits)
      val writeEnable = out Bool ()
    }
  }

  val catCore = CatCore()

  val addressWidth = log2Up(CatConfig.maximumTileSizeInBytes + 1)
  val memoryDepth  = (1 << (addressWidth - 2))

  val undecodedMemory = UnalignedReadWriteMemory(
    width = 32,
    depth = memoryDepth,
    readOnly = false
  )

  val unencodedMemory1 = UnalignedReadWriteMemory(
    width = 32,
    depth = memoryDepth,
    readOnly = false
  )

  val unencodedMemory2 = UnalignedReadWriteMemory(
    width = 32,
    depth = memoryDepth,
    readOnly = false
  )

  val hashMemory = AlignedReadWriteMemory(
    width = addressWidth,
    depth = CatConfig.hashTableSize,
    useByteMask = false
  )

  io.dataReg <> catCore.io.dataReg
  io.status          := catCore.io.status
  catCore.io.control := io.control

  io.info <> catCore.io.info

  catCore.io.undecodedMemory <> undecodedMemory.io.port
  catCore.io.unencodedMemory(0) <> unencodedMemory1.io.port
  catCore.io.unencodedMemory(1) <> unencodedMemory2.io.port
  catCore.io.hashMemory <> hashMemory.io.port
}
