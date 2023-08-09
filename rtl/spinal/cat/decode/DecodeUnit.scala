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

  val undecodedFifo                   = CatFIFO(depth = cat.CatConfig.fifoDepth)
  val consumeUndecodedFifoBytesNumber = UInt(3 bits)
  val consumeUndecodedFifoReady       = Bool()

  val undecodedMemoryReadPtr = Reg(UInt(memoryAddressWidth bits)) init (0)
  // FIFO Control logic
  when(undecodedFifo.io.push.fire) {
    undecodedMemoryReadPtr := undecodedMemoryReadPtr + 4
  }
  undecodedFifo.io.invalidateAll := False

  val writeDecodedMemoryPtr = Reg(UInt(memoryAddressWidth bits)) init (0)

  val consumedBytesNumber =
    Reg(UInt(memoryAddressWidth bits)) init (0)
  val totalDecodeLength   =
    Reg(UInt(memoryAddressWidth bits)) init (0)

  val decodedMemoryReadAddress = UInt(memoryAddressWidth bits)
  decodedMemoryReadAddress := 0

  consumeUndecodedFifoReady       := False
  consumeUndecodedFifoBytesNumber := 0

  def doConsume(n: UInt): Unit = {
    consumeUndecodedFifoReady       := True
    consumeUndecodedFifoBytesNumber := n.resized
    consumedBytesNumber             := consumedBytesNumber + n
  }

  def numberToMask(n: UInt): Bits = {
    val mask = Bits(4 bits)
    switch(n) {
      is(U(0)) { mask := B"0001" }
      is(U(1)) { mask := B"0011" }
      is(U(2)) { mask := B"0111" }
      is(U(3)) { mask := B"1111" }
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
    val referenceDataOffset = Reg(UInt(13 bits)) init (0)

    val sCommandSelect: State = new State with EntryPoint {
      def opcode(command: Bits): Bits = command(7 downto 5)
      def fifoBytes                   = undecodedFifo.io.pop.data
      def firstByteInFifo             = fifoBytes(0)

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
              undumpedBytes := (firstByteInFifo(4 downto 0).asUInt + 1).resized
            }
            // Long Match
            is(B"111") {
              doConsume(3)
              goto(sDumpMatch)

              undumpedBytes       := (fifoBytes(1).asUInt + 9).resized
              referenceDataOffset := (fifoBytes(0)(4 downto 0) ##
                fifoBytes(2)).asUInt.resized
            }
            // Short Match
            default {
              doConsume(2)
              goto(sDumpMatch)

              undumpedBytes       := (op.asUInt + 2).resized
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

        val dumpBytes = undecodedFifo.io.pop.data
        writeToDecodedMemory(dumpBytes.reduceRight(_ ## _), dumpBytesNumber)

        when(lastRound) {
          goto(sCommandSelect)
        }
      }
    }

    val sDumpMatch: State = new State {
      val copyAddress   = Reg(UInt(memoryAddressWidth bits)) init (0)
      val doWrite       = Reg(Bool()) init (False)
      val doWriteNumber = Reg(UInt(3 bits)) init (0)

      onEntry {
        copyAddress   := (writeDecodedMemoryPtr - referenceDataOffset).resized
        doWrite       := False
        doWriteNumber := 0
      }

      whenIsActive {
        val dumpDone = undumpedBytes === 0
        decodedMemoryReadAddress := copyAddress

        val fetchBytesNumber =
          Mux(undumpedBytes <= 4, undumpedBytes, U(4)).resize(
            3 bits
          )

        copyAddress := copyAddress + fetchBytesNumber

        doWriteNumber := fetchBytesNumber
        doWrite       := !dumpDone

        when(doWrite) {
          writeToDecodedMemory(
            io.decodedMemory.read.data,
            doWriteNumber
          )
        }

        when(dumpDone) {
          goto(sCommandSelect)
        }
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
          goto(sPrefetchFifo)
        }
      }
    }

    val sPrefetchFifo: State = new State {
      whenIsActive {
        undecodedFifo.io.invalidateAll := True
        undecodedMemoryReadPtr         := 0
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

  // Child Component Assignment
  undecodedFifo.io.pop.number := consumeUndecodedFifoBytesNumber
  undecodedFifo.io.pop.ready  := consumeUndecodedFifoReady

  undecodedFifo.io.push.data  := Vec(
    io.undecodedMemory.read.data(7 downto 0),
    io.undecodedMemory.read.data(15 downto 8),
    io.undecodedMemory.read.data(23 downto 16),
    io.undecodedMemory.read.data(31 downto 24)
  )
  undecodedFifo.io.push.valid := True

  // IO Assignment
  io.undecodedMemory.read.enable  := True
  io.undecodedMemory.read.address := undecodedMemoryReadPtr

  io.decodedLength := decodedLength

  io.decodedMemory.read.enable  := True
  io.decodedMemory.read.address := decodedMemoryReadAddress

  io.decodedMemory.write.address := writeDecodedMemoryPtr

  io.control.busy := mainFsm.isActive(mainFsm.sDecoding) ||
    mainFsm.isActive(mainFsm.sPrefetchFifo)
  io.control.done := mainFsm.isActive(mainFsm.sDone)
}
