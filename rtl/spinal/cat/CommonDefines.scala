package cat

import spinal.core._
import spinal.lib._

object Status extends SpinalEnum {
  val IDLE, ENCODE, DECODE = newElement()
}
