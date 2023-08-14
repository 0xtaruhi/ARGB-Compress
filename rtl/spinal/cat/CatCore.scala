package cat

import spinal.core._
import spinal.lib._
import spinal.lib.fsm._
import cat.utils.MemoryPort

case class CatCore() extends Component {

  val addressWidth = log2Up(CatConfig.maximumTileSizeInBytes + 1)

  val io = new Bundle {
    // PS/PL Data Register Interface
    val dataReg = new Bundle {
      val readData    = in Bits (32 bits)
      val writeData   = out Bits (32 bits)
      val writeEnable = out Bool ()
      val regIndex    = out UInt (3 bits)
    }

    // PS/PL CS Register Interface
    val status  = out Bits (8 bits)
    val control = in Bits (8 bits)
    val info    = new Bundle {
      val readData    = in Bits (16 bits)
      val writeData   = out Bits (16 bits)
      val writeEnable = out Bool ()
    }

    // Memory Interface
    val unencodedMemory = Vec(
      master(
        MemoryPort(
          readOnly = false,
          dataWidth = 32,
          addressWidth = addressWidth
        )
      ),
      2
    )

    val undecodedMemory = master(
      MemoryPort(
        readOnly = false,
        dataWidth = 32,
        addressWidth = addressWidth
      )
    )

    val hashMemory = master(
      MemoryPort(
        readOnly = false,
        dataWidth = addressWidth,
        addressWidth = log2Up(CatConfig.hashTableSize),
        useByteMask = false
      )
    )
  }

  val decodeUnit = new decode.DecodeUnit()
  val encodeUnit = new encode.EncodeUnit()

  defaultAssign()

  def defaultAssign(): Unit = {
    io.status := Status.Busy

    io.dataReg.writeData   := 0
    io.dataReg.writeEnable := False
    io.dataReg.regIndex    := 0

    io.info.writeData   := 0
    io.info.writeEnable := False

    def defaultMemoryAssign(port: MemoryPort, useByteMask: Boolean = true) {
      port.read.enable   := False
      port.read.address  := 0
      port.write.address := 0
      port.write.data    := 0
      if (useByteMask) {
        port.write.mask := 0
      } else {
        port.write.enable := False
      }
    }

    defaultMemoryAssign(io.undecodedMemory)
    io.unencodedMemory.foreach(defaultMemoryAssign(_))
    io.hashMemory <> encodeUnit.io.hashMemory

    encodeUnit.io.control.setResetInactive()
    decodeUnit.io.control.setResetInactive()
    encodeUnit.io.control.start := False
    decodeUnit.io.control.start := False

    encodeUnit.io.encodeLength := io.info.readData.asUInt.resized
    decodeUnit.io.decodeLength := io.info.readData.asUInt.resized

    encodeUnit.io.unencodedMemory(0).read.data := io
      .unencodedMemory(0)
      .read
      .data
    encodeUnit.io.unencodedMemory(1).read.data := io
      .unencodedMemory(1)
      .read
      .data
    encodeUnit.io.encodedMemory.read.data      := io.undecodedMemory.read.data

    decodeUnit.io.undecodedMemory.read.data := io.undecodedMemory.read.data
    decodeUnit.io.decodedMemory.read.data   := io.unencodedMemory(0).read.data
  }

  def writeInfo(data: Bits) = {
    io.info.writeData   := data.resized
    io.info.writeEnable := True
  }

  def writeDataReg(data: Bits, index: UInt) {
    io.dataReg.regIndex  := index
    io.dataReg.writeData := data
  }

  def setStatus(status: Bits) = {
    io.status := status.resized
  }

  val decodedLength = RegInit(U(0, addressWidth bits))
  val encodedLength = RegInit(U(0, addressWidth bits))

  val infoData = io.info.readData.asUInt

  val mainFsm = new StateMachine {
    val sIdle: State = new State with EntryPoint {
      whenIsActive {
        setStatus(Status.Idle)

        switch(io.control) {
          is(Control.ReadUndecodedMemory) {
            goto(sReadUndecodedMemory)
          }
          is(Control.ReadUnencodedMemory) {
            goto(sReadUnencodedMemory)
          }
          is(Control.WriteUndecodedMemory) {
            goto(sWriteUndecodedMemory)
          }
          is(Control.WriteUnencodedMemory) {
            goto(sWriteUnencodedMemory)
          }
          is(Control.Decode) {
            goto(sDecode)
          }
          is(Control.Encode) {
            goto(sEncode)
          }
        }
      }
    }

    val sReadUndecodedMemory: State = new StateFsm(fsm =
      readMemoryFsm(
        readReq = (address: UInt) => {
          io.undecodedMemory.read.address := address
          io.undecodedMemory.read.enable  := True
        },
        readResp = () => io.undecodedMemory.read.data
      )
    ) {
      whenCompleted {
        goto(sDone)
      }
    }

    val sReadUnencodedMemory: State = new StateFsm(fsm =
      readMemoryFsm(
        readReq = (address: UInt) => {
          io.unencodedMemory(0).read.address := address
          io.unencodedMemory(0).read.enable  := True
        },
        readResp = () => io.unencodedMemory(0).read.data
      )
    ) {
      whenCompleted {
        goto(sDone)
      }
    }

    val sWriteUndecodedMemory: State = new StateFsm(fsm =
      writeMemoryFsm(
        writeReq = (address: UInt, data: Bits) => {
          io.undecodedMemory.write.address := address
          io.undecodedMemory.write.data    := data
          io.undecodedMemory.write.mask    := B"1111"
        }
      )
    ) {
      whenCompleted {
        goto(sDone)
      }
    }

    val sWriteUnencodedMemory: State = new StateFsm(fsm =
      writeMemoryFsm(
        writeReq = (address: UInt, data: Bits) => {
          for (port <- io.unencodedMemory) {
            port.write.address := address
            port.write.data    := data
            port.write.mask    := B"1111"
          }
        }
      )
    ) {
      whenCompleted {
        goto(sDone)
      }
    }

    val sDecode = new StateFsm(decodeFsm()) {
      whenIsActive {
        io.unencodedMemory(0).write <> decodeUnit.io.decodedMemory.write
        io.undecodedMemory.read.address := decodeUnit.io.undecodedMemory.read.address
        io.undecodedMemory.read.enable := decodeUnit.io.undecodedMemory.read.enable
        io.unencodedMemory(0)
          .read
          .address := decodeUnit.io.decodedMemory.read.address
        io.unencodedMemory(0)
          .read
          .enable  := decodeUnit.io.decodedMemory.read.enable
      }
      whenCompleted {
        writeInfo(decodeUnit.io.decodedLength.asBits)
        decodeUnit.io.control.setResetActive()
        goto(sDone)
      }
    }

    val sEncode = new StateFsm(encodeFsm()) {
      whenIsActive {
        io.undecodedMemory.write <> encodeUnit.io.encodedMemory.write
        io.unencodedMemory(0).read.address := encodeUnit.io
          .unencodedMemory(0)
          .read
          .address
        io.unencodedMemory(1).read.address := encodeUnit.io
          .unencodedMemory(1)
          .read
          .address
        io.unencodedMemory(0).read.enable  := encodeUnit.io
          .unencodedMemory(0)
          .read
          .enable
        io.unencodedMemory(1).read.enable  := encodeUnit.io
          .unencodedMemory(1)
          .read
          .enable
      }
      whenCompleted {
        writeInfo(encodeUnit.io.encodedLength.asBits)
        encodeUnit.io.control.setResetActive()
        goto(sDone)
      }
    }

    val sDone = new State {
      whenIsActive {
        when(io.control === Control.ReturnToIdle) {
          goto(sIdle)
        }
        setStatus(Status.Done)
      }
    }

    def decodeFsm(): StateMachine = new StateMachine {
      val sReset: State = new State with EntryPoint {
        whenIsActive {
          decodeUnit.io.control.setResetActive()
          decodeUnit.io.control.start := False
          goto(sStart)
        }
      }

      val sStart: State = new State {
        whenIsActive {
          decodeUnit.io.control.setResetInactive()
          decodeUnit.io.control.start := True
          goto(sRunning)
        }
      }

      val sRunning: State = new State {
        whenIsActive {
          decodeUnit.io.control.start := False
          when(decodeUnit.io.control.done) {
            exit()
          }
        }
      }
    }

    def encodeFsm(): StateMachine = new StateMachine {
      val sReset: State = new State with EntryPoint {
        whenIsActive {
          encodeUnit.io.control.start := False
          goto(sStart)
        }
      }

      val sStart: State = new State {
        whenIsActive {
          encodeUnit.io.control.start := True
          goto(sRunning)
        }
      }

      val sRunning: State = new State {
        whenIsActive {
          encodeUnit.io.control.start := False
          when(encodeUnit.io.control.done) {
            exit()
          }
        }
      }
    }

    def readMemoryFsm(
        readReq: UInt => Unit,
        readResp: () => Bits
    ): StateMachine = new StateMachine {
      val address       = RegInit(U(0, addressWidth bits))
      val sEntry: State = new State with EntryPoint {
        whenIsActive {
          address := (infoData + 4).resized
          readReq(infoData.resized)
          goto(sRunning)
        }
      }

      val sRunning = new State {
        val dataRegIndex = RegInit(U(0, 3 bits))
        onEntry {
          dataRegIndex := 0
        }
        whenIsActive {
          when(dataRegIndex === 7) {
            exit()
          }
          writeDataReg(readResp(), dataRegIndex)
          readReq(address)
          address      := address + 4
          dataRegIndex := dataRegIndex + 1
        }
      }
    }

    def writeMemoryFsm(
        writeReq: (UInt, Bits) => Unit
    ): StateMachine = new StateMachine {
      val address      = RegInit(U(0, addressWidth bits))
      val dataRegIndex = RegInit(U(0, 3 bits))

      val sEntry: State = new State with EntryPoint {
        whenIsActive {
          address      := infoData.resized
          dataRegIndex := 0
          goto(sRunning)
        }
      }

      val sRunning: State = new State {
        whenIsActive {
          when(dataRegIndex === 7) {
            exit()
          }
          io.dataReg.regIndex := dataRegIndex
          writeReq(address, io.dataReg.readData)
          address             := address + 4
          dataRegIndex        := dataRegIndex + 1
        }
      }
    }
  }
}
