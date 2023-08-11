package cat

object CatConfig {
  val fifoDepth = 16

  // This indicates the maximum length per tile.
  val maximumTileLength      = 16
  // 4 bytes per pixel (ARGB 8888)
  def maximumTileSizeInBytes = maximumTileLength * maximumTileLength * 4

  val hashTableSize = 4096

  def maximumMatchLength = 264
}
