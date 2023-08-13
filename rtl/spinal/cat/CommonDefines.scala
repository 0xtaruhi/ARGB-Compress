package cat

import spinal.core._
import spinal.lib._

object Status extends SpinalEnum {
  def Idle = 0x00
  def Busy = 0x01
  def Done = 0x02
}

object Control extends SpinalEnum {
  def Idle = 0x00
  def ReadUnencodedMemory = 0x01
  def ReadUndecodedMemory = 0x02
  def WriteUnencodedMemory = 0x03
  def WriteUndecodedMemory = 0x04
  def Decode = 0x05
  def Encode = 0x06
  def ReturnToIdle = 0x07
}
