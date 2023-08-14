package cat.utils

import spinal.core._
import spinal.lib._

case class AlignedReadWriteMemory(
    val width: Int = 32,
    val depth: Int = 1024,
    val useByteMask: Boolean = false
) extends Component {
  val io = new Bundle {
    val port = slave(
      MemoryPort(
        readOnly = false,
        dataWidth = width,
        addressWidth = log2Up(depth),
        useByteMask = useByteMask
      )
    )
  }

  val mem = Mem(Bits(width bits), depth)

  if (useByteMask) {
    mem.write(
      address = io.port.write.address,
      data = io.port.write.data,
      enable = True,
      mask = io.port.write.mask
    )
  } else {
    mem.write(
      address = io.port.write.address,
      data = io.port.write.data,
      enable = io.port.write.enable
    )
  }

  val addressWidth = log2Up(depth)
  io.port.read.data := mem.readSync(address =
    io.port.read.address.takeHigh(addressWidth).asUInt
  )
}

/*
 * UnalignedReadWriteMemory
 *
 * UnalignedReadWriteMemory supports unaligned read and write operations.
 *
 */
case class UnalignedReadWriteMemory(
    val width: Int = 32,
    val depth: Int = 1024,
    val readOnly: Boolean = false
) extends Component {
  require(isPow2(depth))
  val addrWidth = log2Up(depth) + 2

  val io = new Bundle {
    val port = slave(
      MemoryPort(readOnly, dataWidth = width, addressWidth = addrWidth)
    )
  }

  val bank0 = Mem(Bits(width bits), depth / 2)
  val bank1 = Mem(Bits(width bits), depth / 2)

  val readArea = new Area {
    val baseAddress = io.port.read.address(addrWidth - 1 downto 3)
    val bankSelect  = io.port.read.address(2)
    val offset      = io.port.read.address(1 downto 0)

    val headByteInBank0 = !bankSelect
    val headByteInBank1 = bankSelect
    val crossBank       = offset =/= 0

    val bank0Address = Mux(headByteInBank1, baseAddress + 1, baseAddress)
    val bank1Address = baseAddress

    val bank0Data = bank0.readSync(
      address = bank0Address,
      enable = io.port.read.enable && (headByteInBank0 || crossBank)
    )

    val bank1Data = bank1.readSync(
      address = bank1Address,
      enable = io.port.read.enable && (headByteInBank1 || crossBank)
    )

    // Concatenate bank0Data and bank1Data to form the read data
    val concatenatedData = Mux(
      headByteInBank0,
      bank1Data ## bank0Data,
      bank0Data ## bank1Data
    )

    io.port.read.data := (concatenatedData >> (offset * 8)).resized
  }

  val writeArea = !readOnly generate new Area {
    val baseAddress = io.port.write.address(addrWidth - 1 downto 3)
    val bankSelect  = io.port.write.address(2)
    val offset      = io.port.write.address(1 downto 0)

    val headByteInBank0 = !bankSelect
    val headByteInBank1 = bankSelect
    val crossBank       = offset =/= 0

    val bank0Address = Mux(headByteInBank1, baseAddress + 1, baseAddress)
    val bank1Address = baseAddress
    val mask         = (io.port.write.mask << (offset * 8))
      .resize(width / 4)
    val writeData    = (io.port.write.data << (offset * 8))
      .resize(width * 2)

    val bank0WriteData = Bits(width bits)
    val bank1WriteData = Bits(width bits)
    val bank0WriteMask = Bits(width / 8 bits)
    val bank1WriteMask = Bits(width / 8 bits)

    when(headByteInBank1) {
      bank0WriteData := writeData(2 * width - 1 downto width)
      bank1WriteData := writeData(width - 1 downto 0)
      bank0WriteMask := mask(2 * width / 8 - 1 downto width / 8)
      bank1WriteMask := mask(width / 8 - 1 downto 0)
    } otherwise {
      bank0WriteData := writeData(width - 1 downto 0)
      bank1WriteData := writeData(2 * width - 1 downto width)
      bank0WriteMask := mask(width / 8 - 1 downto 0)
      bank1WriteMask := mask(2 * width / 8 - 1 downto width / 8)
    }

    bank0.write(
      address = bank0Address,
      data = bank0WriteData,
      enable = bank0WriteMask.orR,
      mask = bank0WriteMask
    )

    bank1.write(
      address = bank1Address,
      data = bank1WriteData,
      enable = bank1WriteMask.orR,
      mask = bank1WriteMask
    )
  }
}

object UnalignedReadOnlyMemory {
  def apply(
      width: Int = 32,
      depth: Int = 1024
  ): UnalignedReadWriteMemory = {
    UnalignedReadWriteMemory(width, depth, readOnly = true)
  }
}
