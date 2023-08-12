package cat

import spinal.core._
import spinal.lib._

object Verilog {
  val generateConfig = SpinalConfig(
    defaultConfigForClockDomains =
      ClockDomainConfig(resetKind = SYNC, resetActiveLevel = LOW),
    targetDirectory = "rtl/verilog"
  ).addStandardMemBlackboxing(blackboxAllWhatsYouCan)

  def main(args: Array[String]): Unit = {
    generateConfig.generateVerilog(decode.DecodeUnit())
    generateConfig.generateVerilog(encode.EncodeUnit())
  }
}
