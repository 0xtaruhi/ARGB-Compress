package cat.encode

import spinal.core._
import spinal.lib._
import spinal.lib.fsm._

import cat.utils._
import _root_.cat.CatConfig

case class EncodeUnit() extends Component {
  val addressWidth = log2Up(cat.CatConfig.maximumTileSizeInBytes + 1)

  val io = new Bundle {
    val control = slave(UnitControlSignals())

    val encodeLength  = in UInt (addressWidth bits)
    val encodedLength = out UInt (addressWidth bits)

    val unencodedMemory = Vec(
      master(
        MemoryPort(
          readOnly = true,
          dataWidth = 32,
          addressWidth = addressWidth
        )
      ),
      2
    )

    val encodedMemory = master(
      MemoryPort(
        readOnly = false,
        dataWidth = 32,
        addressWidth = addressWidth
      )
    )
  }

  val hashTable =
    new HashTable(keyWidth = 32, valueWidth = 32, addressWidth = 12)

  val anchor            = RegInit(U(0, addressWidth bits))
  val ip                = RegInit(U(0, addressWidth bits))
  val limit             = RegInit(U(0, addressWidth bits))
  val totalEncodeLength = RegInit(U(0, addressWidth bits))
  val encodedLength     = RegInit(U(0, addressWidth bits))

  for (i <- 0 until 2) {
    io.unencodedMemory(i).read.enable  := False
    io.unencodedMemory(i).read.address := 0
  }

  def unencodedMemoryRead(address: UInt, memoryIndex: Int = 0) {
    io.unencodedMemory(memoryIndex).read.address := address.resized
    io.unencodedMemory(memoryIndex).read.enable  := True
  }

  io.encodedMemory.write.address := 0
  io.encodedMemory.write.data    := 0
  io.encodedMemory.write.mask    := 0
  io.encodedMemory.read.enable   := False
  io.encodedMemory.read.address  := 0

  def encodedMemoryWrite(
      address: UInt = encodedLength,
      data: Bits
  ) {
    io.encodedMemory.write.address := address
    io.encodedMemory.write.data    := data.resized

    val bytesNumber = data.getWidth / 8

    io.encodedMemory.write.mask := B((1 << bytesNumber) - 1, 4 bits)
    encodedLength               := encodedLength + bytesNumber
  }

  def encodedMemoryWriteWithExplicitBytesNumber(
      address: UInt = encodedLength,
      data: Bits,
      bytesNumber: UInt
  ) {
    io.encodedMemory.write.address := address
    io.encodedMemory.write.data    := data

    io.encodedMemory.write.mask := B((U(1) << bytesNumber) - 1, 4 bits)
    encodedLength               := encodedLength + bytesNumber
  }

  for (hashEntry <- hashTable.io.update) {
    hashEntry.enable := False
    hashEntry.key    := 0
    hashEntry.value  := 0
  }
  hashTable.io.read.key := 0

  def updateHashTable(key: UInt, value: UInt, entryIndex: Int = 0) {
    hashTable.io.update(entryIndex).enable := True
    hashTable.io.update(entryIndex).key    := key.resized
    hashTable.io.update(entryIndex).value  := value.resized
  }

  val encodeFsm = new StateMachine {
    def exceedsLimit = ip >= limit

    val distance = RegInit(U(0, addressWidth bits))
    val ref      = RegInit(U(0, 32 bits))

    val sConfig: State = new State with EntryPoint {
      whenIsActive {
        ip := 2
        goto(sLoopOuterWhile)
      }
    }

    val totalDumpLiteralsNumber = RegInit(U(0, addressWidth bits))
    def dumpLiteralsFsm         = new StateMachine {
      val dumpPos              = RegInit(U(0, addressWidth bits))
      val thisPacketDumpNumber = RegInit(U(0, 6 bits))

      val sConfig: State = new State with EntryPoint {
        whenIsActive {
          dumpPos := anchor
          unencodedMemoryRead(anchor)

          thisPacketDumpNumber := Mux(
            totalDumpLiteralsNumber > 32,
            U(32),
            totalDumpLiteralsNumber
          ).resized

          encodedMemoryWrite(
            data = (thisPacketDumpNumber - 1).asBits.resize(8 bits)
          )
          goto(sRunning)

          val totalDumpLiteralsNumberNext =
            totalDumpLiteralsNumber - thisPacketDumpNumber
          totalDumpLiteralsNumber := totalDumpLiteralsNumberNext
        }
      }

      val sRunning: State = new State {
        whenIsActive {
          val thisCycleDumpNumber = Mux(
            thisPacketDumpNumber > 4,
            U(4),
            thisPacketDumpNumber
          ).resize(3 bits)

          encodedMemoryWriteWithExplicitBytesNumber(
            data = io.unencodedMemory(0).read.data,
            bytesNumber = thisCycleDumpNumber
          )

          dumpPos := dumpPos + thisCycleDumpNumber

          val thisPacketDumpNumberNext =
            thisPacketDumpNumber - thisCycleDumpNumber
          when(thisPacketDumpNumberNext === 0) {
            when(totalDumpLiteralsNumber === 0) {
              exit()
            } otherwise {
              goto(sConfig)
            }
          }

          thisPacketDumpNumber := thisPacketDumpNumberNext
        }
      }
    }

    val matchedLength = RegInit(U(0, addressWidth bits))
    val comparePos1   = RegInit(U(0, addressWidth bits))
    val comparePos2   = RegInit(U(0, addressWidth bits))
    val compareBound  = RegInit(U(0, addressWidth bits))

    def compareAndDumpMatchFsm = new StateMachine {
      def compareFsm = new StateMachine {
        val compareStartPos = RegInit(U(0, addressWidth bits))

        val sConfig: State = new State with EntryPoint {
          whenIsActive {
            compareStartPos := comparePos1
            goto(sRunning)

            unencodedMemoryRead(comparePos1, 0)
            unencodedMemoryRead(comparePos2, 1)
          }
        }

        val sRunning: State = new State {
          val pos1Word = io.unencodedMemory(0).read.data
          val pos2Word = io.unencodedMemory(1).read.data

          whenIsActive {
            // Split the word into bytes
            val pos1Bytes = Vec(
              for (i <- 0 until 4)
                yield pos1Word((i + 1) * 8 - 1 downto i * 8)
            )
            val pos2Bytes = Vec(
              for (i <- 0 until 4)
                yield pos2Word((i + 1) * 8 - 1 downto i * 8)
            )

            // Compare the bytes
            val bytesMatch = Vec(
              for (i <- 0 until 4)
                yield pos1Bytes(i) === pos2Bytes(i)
            )

            // Find the first mismatching byte
            val wordMatchedBytesNumber = UInt(3 bits)
            when(bytesMatch(0) === False) {
              wordMatchedBytesNumber := 0
            } elsewhen (bytesMatch(1) === False) {
              wordMatchedBytesNumber := 1
            } elsewhen (bytesMatch(2) === False) {
              wordMatchedBytesNumber := 2
            } otherwise {
              wordMatchedBytesNumber := 3
            }

            when(bytesMatch.andR =/= True) {
              exit()
            }

            val comparePos1Next = comparePos1 + 4
            val comparePos2Next = comparePos2 + 4

            val thisCycleMatchedBytesNumber = UInt(3 bits)
            when(comparePos2Next >= compareBound) {
              val exceededBytesNumber =
                (comparePos2Next - compareBound).resize(3 bits)
              thisCycleMatchedBytesNumber := Mux(
                wordMatchedBytesNumber > exceededBytesNumber,
                exceededBytesNumber,
                wordMatchedBytesNumber
              )
              exit()
            } otherwise {
              thisCycleMatchedBytesNumber := wordMatchedBytesNumber
            }

            unencodedMemoryRead(comparePos1Next, 0)
            unencodedMemoryRead(comparePos2Next, 1)

            comparePos1 := comparePos1Next
            comparePos2 := comparePos2Next

            // Update the matched length
            matchedLength := matchedLength + thisCycleMatchedBytesNumber
          }
        }
      }

      def dumpMatchFsm = new StateMachine {
        val dumpMatchDistance = RegInit(U(0, addressWidth bits))

        val sConfig: State = new State with EntryPoint {
          whenIsActive {
            dumpMatchDistance := distance - 1
            goto(sRunning)
          }
        }

        val sRunning: State = new State {
          whenIsActive {
            val shouldExit = False

            when(matchedLength > CatConfig.maximumMatchLength - 2) {
              // Long Match (exceed the maximum match length)
              val byte0 = B"111" ## (distance >> 8).resize(5 bits).asBits
              val byte1 = B(CatConfig.maximumMatchLength - 2 - 7 - 2, 8 bits)
              val byte2 = distance(7 downto 0).asBits
              encodedMemoryWrite(data = (byte2 ## byte1 ## byte0))

              matchedLength := matchedLength - (CatConfig.maximumMatchLength - 2)
            } elsewhen (matchedLength < 7) {
              // Short Match
              val byte0 =
                matchedLength(7 downto 5).asBits ## (distance >> 8)
                  .resize(5 bits)
                  .asBits
              val byte1 = distance(7 downto 0).asBits
              encodedMemoryWrite(data = (byte1 ## byte0))

              shouldExit := True
            } otherwise {
              // Long Match (within the maximum match length)
              val byte0 = B"111" ## (distance >> 8).resize(5 bits).asBits
              val byte1 = B(matchedLength - 7, 8 bits)
              val byte2 = distance(7 downto 0).asBits
              encodedMemoryWrite(data = (byte2 ## byte1 ## byte0))

              shouldExit := True
            }

            when(shouldExit) {
              exit()
            }
          }
        }
      }

      val sCompare = new StateFsm(fsm = compareFsm) with EntryPoint {
        whenCompleted {
          goto(sDumpMatch)
        }
      }

      val sDumpMatch = new StateFsm(fsm = dumpMatchFsm) {
        whenCompleted {
          exit()
        }
      }
    }

    def loopOuterWhileFsm = new StateMachine {
      val seq = RegInit(U(0, 32 bits))

      def loopInnerWhileFsm = new StateMachine {
        val sStage1: State = new State with EntryPoint {
          whenIsActive {
            unencodedMemoryRead(ip)
            goto(sStage2)
          }
        }

        val sStage2: State = new State {
          whenIsActive {
            val refNext = hashTable.io.read.value.resize(ref.getWidth bits)
            distance := ip - refNext.resize(ip.getWidth bits)
            ref      := refNext

            val seqNext = io.unencodedMemory(0).read.data.asUInt
            seq := seqNext

            updateHashTable(key = seqNext, value = ip.resized)

            unencodedMemoryRead(refNext)

            when(exceedsLimit) {
              exit()
            } otherwise {
              ip := ip + 1
              goto(sConditionCheck)
            }
          }
        }

        val sConditionCheck: State = new State {
          whenIsActive {
            when(seq(23 downto 0) =/= ref(23 downto 0)) {
              goto(sStage1)
            } otherwise {
              exit()
            }
          }
        }
      }

      val sConditionCheck: State = new State {
        whenIsActive {
          when(exceedsLimit) {
            exit()
          } otherwise {
            goto(sStage1)
          }
        }
      }

      val sStage1: State = new StateFsm(fsm = loopInnerWhileFsm)
        with EntryPoint {
        whenCompleted {
          goto(sStage2)
        }
      }

      val sStage2: State = new State {
        whenIsActive {
          when(exceedsLimit) {
            exit()
          } otherwise {
            val ipNext = ip - 1
            ip := ipNext
            when(ipNext > anchor) {
              goto(sDumpLiterals)
            } otherwise {
              goto(sDumpMatch)
              comparePos1  := (ref + 3).resized
              comparePos2  := (ipNext + 3).resized
              compareBound := totalEncodeLength - 4
            }
          }
        }
      }

      val sDumpLiterals: State = new StateFsm(fsm = dumpLiteralsFsm) {
        whenCompleted {
          goto(sStage3)
        }
      }

      val sDumpMatch: State = new StateFsm(fsm = compareAndDumpMatchFsm) {
        whenCompleted {
          goto(sStage3)
        }
      }

      val sStage3: State = new State {
        whenIsActive {
          val ipNext = ip + matchedLength
          unencodedMemoryRead(ipNext)
        }
      }

      val sStage4: State = new State {
        whenIsActive {
          seq := io.unencodedMemory(0).read.data.asUInt
          updateHashTable(key = seq(5 downto 0), value = ip)
          ip  := ip + 1
          goto(sStage5)
        }
      }

      val sStage5: State = new State {
        whenIsActive {
          updateHashTable(key = seq(13 downto 8), value = ip)
          val ipNext = ip + 1
          ip     := ipNext
          anchor := ipNext
          goto(sConditionCheck)
        }
      }
    }

    val sLoopOuterWhile: State = new StateFsm(fsm = loopOuterWhileFsm) {
      whenCompleted {
        goto(sDumpLeft)
        totalDumpLiteralsNumber := totalEncodeLength - anchor
      }
    }

    val sDumpLeft: State = new StateFsm(fsm = dumpLiteralsFsm) {
      whenCompleted {
        exit()
      }
    }
  }

  val mainFsm = new StateMachine {
    val sIdle: State = new State with EntryPoint {
      whenIsActive {
        when(io.control.fire) {
          goto(sEncoding)
          ip                := 0
          totalEncodeLength := io.encodeLength
          limit             := io.encodeLength - 13
        }
      }
    }

    val sEncoding = new StateFsm(fsm = encodeFsm) {
      whenCompleted {
        goto(sDone)
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

  // IO Assignments
  io.control.done := mainFsm.isActive(mainFsm.sDone)
  io.control.busy := !mainFsm.isActive(mainFsm.sIdle) && !mainFsm
    .isActive(mainFsm.sDone)

  io.encodedLength := encodedLength
}
