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

    val hashMemory = master(
      MemoryPort(
        readOnly = false,
        dataWidth = 32,
        addressWidth = log2Up(CatConfig.hashTableSize)
      )
    )
  }

  val hashTable =
    new HashTable(
      keyWidth = 32,
      valueWidth = 32,
      addressWidth = log2Up(CatConfig.hashTableSize)
    )

  def hashTableRead(key: UInt) {
    hashTable.io.read.key := key
  }

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

  hashTable.io.update.enable := False
  hashTable.io.update.key    := 0
  hashTable.io.update.value  := 0
  hashTable.io.read.key      := 0
  hashTable.io.hashMemoryPort <> io.hashMemory

  def updateHashTable(key: UInt, value: UInt) {
    hashTable.io.update.enable := True
    hashTable.io.update.key    := key.resized
    hashTable.io.update.value  := value.resized
  }

  val encodeFsm = new StateMachine {
    def exceedsLimit = ip >= limit

    val distance = RegInit(U(0, addressWidth bits))
    val ref      = RegInit(U(0, addressWidth bits))

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

          val thisPacketDumpNumberNext = Mux(
            totalDumpLiteralsNumber > 32,
            U(32),
            totalDumpLiteralsNumber
          ).resize(thisPacketDumpNumber.getWidth)

          thisPacketDumpNumber := thisPacketDumpNumberNext

          encodedMemoryWrite(
            data = (thisPacketDumpNumberNext - 1).asBits.resize(8 bits)
          )
          goto(sRunning)

          val totalDumpLiteralsNumberNext =
            totalDumpLiteralsNumber - thisPacketDumpNumberNext
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

          val dumpPosNext = dumpPos + thisCycleDumpNumber
          dumpPos := dumpPosNext
          unencodedMemoryRead(dumpPosNext)

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
            matchedLength := 0
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
            } elsewhen (bytesMatch(3) === False) {
              wordMatchedBytesNumber := 3
            } otherwise {
              wordMatchedBytesNumber := 4
            }

            def wordNotSame = bytesMatch.andR === False

            val comparePos1Next = comparePos1 + 4
            val comparePos2Next = comparePos2 + 4

            val thisCycleMatchedBytesNumber = UInt(3 bits)
            val inBoundBytesNumber          =
              Min(U(4), compareBound - comparePos2).resize(3)

            thisCycleMatchedBytesNumber := Min(
              wordMatchedBytesNumber,
              inBoundBytesNumber
            )

            val matchedLengthNext = matchedLength + thisCycleMatchedBytesNumber
            matchedLength := matchedLengthNext

            when(wordNotSame) {
              matchedLength := matchedLengthNext + 1
              exit()
            }

            unencodedMemoryRead(comparePos1Next, 0)
            unencodedMemoryRead(comparePos2Next, 1)

            comparePos1 := comparePos1Next
            comparePos2 := comparePos2Next

            when(comparePos2Next >= compareBound) {
              exit()
            }
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
            val dumpMatchDistanceHigh =
              dumpMatchDistance(dumpMatchDistance.getWidth - 1 downto 8)

            when(matchedLength > CatConfig.maximumMatchLength - 2) {
              // Long Match (exceed the maximum match length)

              val byte0 = B"111" ## dumpMatchDistanceHigh.resize(5 bits).asBits
              val byte1 = B(CatConfig.maximumMatchLength - 2 - 7 - 2, 8 bits)
              val byte2 = dumpMatchDistance(7 downto 0).asBits
              encodedMemoryWrite(data = (byte2 ## byte1 ## byte0))

              matchedLength := matchedLength - (CatConfig.maximumMatchLength - 2)
            } elsewhen (matchedLength < 7) {
              // Short Match
              val byte0 =
                matchedLength.resize(3).asBits ## dumpMatchDistanceHigh
                  .resize(5 bits)
                  .asBits
              val byte1 = dumpMatchDistance(7 downto 0).asBits
              encodedMemoryWrite(data = (byte1 ## byte0))

              exit()
            } otherwise {
              // Long Match (within the maximum match length)
              val byte0 = B"111" ## dumpMatchDistanceHigh.resize(5 bits).asBits
              val byte1 = B(matchedLength - 7, 8 bits)
              val byte2 = dumpMatchDistance(7 downto 0).asBits
              encodedMemoryWrite(data = (byte2 ## byte1 ## byte0))

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
            val seqNext =
              (B(0, 8 bits) ## io
                .unencodedMemory(0)
                .read
                .data(23 downto 0)).asUInt
            seq := seqNext
            hashTableRead(seqNext)
            goto(sStage3)
          }
        }

        val sStage3: State = new State {
          whenIsActive {
            val refNext = hashTable.io.read.value.resize(ref.getWidth)
            distance := ip - refNext
            ref      := refNext

            updateHashTable(key = seq, value = ip.resized)
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
            val refWord = io.unencodedMemory(0).read.data.asUInt
            when(seq(23 downto 0) =/= refWord(23 downto 0)) {
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
              totalDumpLiteralsNumber := ipNext - anchor
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
          goto(sDumpMatch)
          comparePos1  := (ref + 3).resized
          comparePos2  := (ip + 3).resized
          compareBound := totalEncodeLength - 4
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
          ip := ipNext
          unencodedMemoryRead(ipNext)
          goto(sStage4)
        }
      }

      val sStage4: State = new State {
        whenIsActive {
          val seqNext = io.unencodedMemory(0).read.data.asUInt
          seq := seqNext
          updateHashTable(key = seqNext(23 downto 0), value = ip)
          ip  := ip + 1
          goto(sStage5)
        }
      }

      val sStage5: State = new State {
        whenIsActive {
          updateHashTable(key = seq(31 downto 8), value = ip)
          val ipNext = ip + 1
          ip     := ipNext
          anchor := ipNext
          goto(sConditionCheck)
        }
      }
    }

    val sLoopOuterWhile: State = new StateFsm(fsm = loopOuterWhileFsm) {
      whenCompleted {
        when(totalEncodeLength > anchor) {
          goto(sDumpLeft)
          totalDumpLiteralsNumber := totalEncodeLength - anchor
        } otherwise {
          exit()
        }
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
