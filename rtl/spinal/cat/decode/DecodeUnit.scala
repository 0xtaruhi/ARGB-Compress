package cat.decode

import spinal.core._
import spinal.lib._
import spinal.lib.fsm._

import cat.utils._

case class DecodeUnit() extends Component {
  val memoryAddressWidth = log2Up(cat.CatConfig.maximumTileSizeInBytes + 1)

  val io = new Bundle {
    val control       = slave(UnitControlSignals())
    val decodeLength  = in UInt (memoryAddressWidth bits)
    val decodedLength = out UInt (memoryAddressWidth bits)

    val undecodedMemory = master(
      MemoryPort(
        readOnly = true,
        dataWidth = 32,
        addressWidth = memoryAddressWidth
      )
    )
    val decodedMemory   = master(
      MemoryPort(
        readOnly = false,
        dataWidth = 32,
        addressWidth = memoryAddressWidth
      )
    )
  }

  val writeDecodedMemoryPtr = Reg(UInt(memoryAddressWidth bits)) init (0)

  val consumedBytesNumber =
    Reg(UInt(memoryAddressWidth bits)) init (0)
  val totalDecodeLength   =
    Reg(UInt(memoryAddressWidth bits)) init (0)

  val decodedMemoryReadAddress = UInt(memoryAddressWidth bits)
  decodedMemoryReadAddress := 0

  val thisCycleConsumeBytesNumber = U(0, 3 bits)
  val consumedBytesNumberNext     =
    consumedBytesNumber + thisCycleConsumeBytesNumber
  consumedBytesNumber := consumedBytesNumberNext

  def doConsume(n: UInt): Unit = {
    // consumedBytesNumberNext := consumedBytesNumber + n
    thisCycleConsumeBytesNumber := n
  }

  def numberToMask(n: UInt): Bits = {
    val mask = Bits(4 bits)
    switch(n) {
      is(U(1)) { mask := B"0001" }
      is(U(2)) { mask := B"0011" }
      is(U(3)) { mask := B"0111" }
      is(U(4)) { mask := B"1111" }
      default { mask := B"0000" }
    }
    mask
  }

  def writeToDecodedMemory(data: Bits, number: UInt): Unit = {
    io.decodedMemory.write.address := writeDecodedMemoryPtr
    io.decodedMemory.write.data    := data
    io.decodedMemory.write.mask    := numberToMask(number)
    writeDecodedMemoryPtr          := writeDecodedMemoryPtr + number
  }

  io.decodedMemory.write.data := 0
  io.decodedMemory.write.mask := 0

  val decodeFsm = new StateMachine {
    val undumpedBytes       = Reg(UInt(9 bits)) init (0)
    val referenceDataOffset = U(0, 13 bits)

    val sCommandSelect: State = new State with EntryPoint {
      def opcode(command: Bits): Bits = command(7 downto 5)
      def fifoBytes       = io.undecodedMemory.read.data.subdivideIn(8 bits)
      def firstByteInFifo = fifoBytes(0)

      whenIsActive {
        when(consumedBytesNumber === totalDecodeLength) {
          exit()
        } otherwise {
          val op = opcode(firstByteInFifo)
          switch(op) {
            // Literal Run
            is(B"000") {
              doConsume(1)
              goto(sLiteralRun)
              undumpedBytes := firstByteInFifo(4 downto 0).asUInt
                .resize(undumpedBytes.getWidth) + 1
            }
            // Long Match
            is(B"111") {
              doConsume(3)
              goto(sDumpMatch)

              undumpedBytes       := (fifoBytes(1).asUInt +^ 9).resized
              referenceDataOffset := (fifoBytes(0)(4 downto 0) ##
                fifoBytes(2)).asUInt.resized
            }
            // Short Match
            default {
              doConsume(2)
              goto(sDumpMatch)

              undumpedBytes := op.resize(undumpedBytes.getWidth).asUInt + 2
              referenceDataOffset := (fifoBytes(0)(4 downto 0) ##
                fifoBytes(1)).asUInt.resized
            }
          }
        }
      }
    }

    val sLiteralRun: State = new State {
      whenIsActive {
        // Can dump at most 4 bytes
        val lastRound = undumpedBytes <= 4

        val dumpBytesNumber =
          Mux(lastRound, undumpedBytes, U(4)).resize(3 bits)
        doConsume(dumpBytesNumber)
        undumpedBytes := undumpedBytes - dumpBytesNumber

        writeToDecodedMemory(io.undecodedMemory.read.data, dumpBytesNumber)

        when(lastRound) {
          goto(sCommandSelect)
        }
      }
    }

    val sDumpMatch: State = new State {
      val copyAddress       = Reg(UInt(memoryAddressWidth bits)) init (0)
      val dataSourceAddress = RegNext(decodedMemoryReadAddress) init (0)

      onEntry {
        decodedMemoryReadAddress := (writeDecodedMemoryPtr - referenceDataOffset - 1).resized
        copyAddress := decodedMemoryReadAddress + 4
      }

      whenIsActive {
        decodedMemoryReadAddress := copyAddress

        val fetchBytesNumber =
          Mux(undumpedBytes <= 4, undumpedBytes, U(4)).resize(
            3 bits
          )

        def extendReadData(data: Bits, validBytesNumber: UInt): Bits = {
          val result = Bits(32 bits)
          switch(validBytesNumber) {
            is(U(1)) {
              result := data(7 downto 0) ## data(7 downto 0) ## data(
                7 downto 0
              ) ## data(7 downto 0)
            }
            is(U(2)) {
              result := data(15 downto 0) ## data(15 downto 0)
            }
            is(U(3)) {
              result := data(7 downto 0) ## data(23 downto 0)
            }
            default {
              result := data
            }
          }
          result
        }

        when(undumpedBytes <= 4) {
          goto(sCommandSelect)
        }

        copyAddress   := copyAddress + fetchBytesNumber
        undumpedBytes := undumpedBytes - fetchBytesNumber

        val validBytesNumber = writeDecodedMemoryPtr - dataSourceAddress

        writeToDecodedMemory(
          extendReadData(io.decodedMemory.read.data, validBytesNumber),
          fetchBytesNumber
        )
      }
    }
  }

  val decodedLength =
    Reg(UInt(memoryAddressWidth bits)) init (0)

  val mainFsm = new StateMachine {
    val sIdle: State = new State with EntryPoint {
      whenIsActive {
        consumedBytesNumber := 0
        totalDecodeLength   := io.decodeLength
        when(io.control.fire) {
          goto(sDecoding)
        }
      }
    }

    val sDecoding: State = new StateFsm(fsm = decodeFsm) {
      whenCompleted {
        goto(sDone)
        decodedLength := writeDecodedMemoryPtr
      }
    }

    val sDone: State = new State {
      whenIsActive {
        when(io.control.reset) {
          goto(sIdle)
        }
      }
    }
  }

  // IO Assignment
  io.undecodedMemory.read.enable  := True
  io.undecodedMemory.read.address := consumedBytesNumberNext

  io.decodedLength := decodedLength

  io.decodedMemory.read.enable  := True
  io.decodedMemory.read.address := decodedMemoryReadAddress

  io.decodedMemory.write.address := writeDecodedMemoryPtr

  io.control.busy := mainFsm.isActive(mainFsm.sDecoding)
  io.control.done := mainFsm.isActive(mainFsm.sDone)
}
