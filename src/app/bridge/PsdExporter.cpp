#include "app/bridge/PsdExporter.h"
#include "app/bridge/PsdUtils.h"

#include <algorithm>
#include <fstream>
#include <cstring>

#include "core/document/Document.h"
#include "core/layer/Layer.h"
#include "core/render/Renderer.h"

namespace app::psd {

// ─────────────────────────────────────────────────────────────────────────────
// レイヤーチャンネルデータを PackBits で圧縮して書き出すヘルパー
// PSD チャンネル順: R G B A (各チャンネルを行ごとに)
// ─────────────────────────────────────────────────────────────────────────────
static void writeLayerChannelData(std::vector<uint8_t>& buf,
                                   const core::PixelBuffer& pixels,
                                   int layerX, int layerY,
                                   int layerW, int layerH,
                                   int canvasW, int canvasH) {
  // チャンネル順: [0]=R [1]=G [2]=B [-1]=A
  // PSD 内部では Alpha が先、次に RGB の場合もあるが layerinfo では -1,0,1,2 の順に記述
  const int channels = 4;
  const int chanOrder[4] = {3, 0, 1, 2}; // A, R, G, B インデックス

  for (int ch = 0; ch < channels; ++ch) {
    const int chanIdx = chanOrder[ch]; // 0=R,1=G,2=B,3=A

    // 行ごとの RLE サイズを格納するテーブル (2 bytes/row)
    const size_t rleTableOffset = buf.size();
    for (int row = 0; row < layerH; ++row) {
      writeU16BE(buf, 0); // 後で埋める
    }

    for (int row = 0; row < layerH; ++row) {
      const int canvasY = layerY + row;
      std::vector<uint8_t> rowData(layerW);
      for (int col = 0; col < layerW; ++col) {
        const int canvasX = layerX + col;
        if (canvasX < 0 || canvasY < 0 || canvasX >= canvasW || canvasY >= canvasH) {
          rowData[col] = 0;
          continue;
        }
        const core::Color c = pixels.pixel(canvasX, canvasY);
        switch (chanIdx) {
          case 0: rowData[col] = c.r; break;
          case 1: rowData[col] = c.g; break;
          case 2: rowData[col] = c.b; break;
          case 3: rowData[col] = c.a; break;
        }
      }
      const auto rle = packBitsEncode(rowData.data(), layerW);
      // RLE サイズを書き戻す
      const uint16_t rleSize = static_cast<uint16_t>(rle.size());
      buf[rleTableOffset + row * 2]     = rleSize >> 8;
      buf[rleTableOffset + row * 2 + 1] = rleSize & 0xFF;
      // RLE データを追加
      writeBytes(buf, rle.data(), rle.size());
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// PSD エクスポート本体
// ─────────────────────────────────────────────────────────────────────────────
ExportResult exportPsd(const core::Document& doc, const std::string& path) {
  const int W = doc.canvasSize().width;
  const int H = doc.canvasSize().height;
  const int numLayers = static_cast<int>(doc.layerCount());

  std::vector<uint8_t> buf;
  buf.reserve(W * H * 4 + numLayers * 256 * 1024);

  // ── 1. File Header ────────────────────────────────────────────────────────
  writeBytes(buf, "8BPS", 4);   // signature
  writeU16BE(buf, 1);            // version (1 = PSD, 2 = PSB)
  for (int i = 0; i < 6; ++i) buf.push_back(0); // reserved
  writeU16BE(buf, 4);            // channels (RGBA)
  writeU32BE(buf, H);
  writeU32BE(buf, W);
  writeU16BE(buf, 8);            // bits per channel
  writeU16BE(buf, 3);            // color mode (3 = RGB)

  // ── 2. Color Mode Data (empty for RGB) ───────────────────────────────────
  writeU32BE(buf, 0);

  // ── 3. Image Resources (minimal) ─────────────────────────────────────────
  writeU32BE(buf, 0);

  // ── 4. Layer and Mask Information ────────────────────────────────────────
  // レイヤーセクションのサイズは後で書き戻す
  const size_t layerSectionSizeOffset = buf.size();
  writeU32BE(buf, 0); // placeholder

  const size_t layerSectionStart = buf.size();

  // Layer info セクション
  const size_t layerInfoSizeOffset = buf.size();
  writeU32BE(buf, 0); // placeholder
  const size_t layerInfoStart = buf.size();

  // レイヤー数 (負 = 最初のアルファが透明度)
  writeI32BE(buf, -numLayers);

  // 各レイヤーのレコード
  for (int li = numLayers - 1; li >= 0; --li) { // 下から順に
    const core::Layer& layer = doc.layerAt(li);

    // レイヤー境界 (top, left, bottom, right)
    writeI32BE(buf, 0);
    writeI32BE(buf, 0);
    writeI32BE(buf, H);
    writeI32BE(buf, W);

    // チャンネル情報 4 チャンネル: -1 (alpha), 0 (R), 1 (G), 2 (B)
    writeU16BE(buf, 4);
    const int16_t chanIds[4] = {-1, 0, 1, 2};
    for (int ch = 0; ch < 4; ++ch) {
      const int16_t cid = chanIds[ch];
      buf.push_back(uint8_t(cid >> 8)); buf.push_back(uint8_t(cid));
      writeU32BE(buf, 0); // channel data length placeholder (filled later)
    }

    // ブレンドモードシグネチャ
    writeBytes(buf, "8BIM", 4);
    const std::string bmKey = blendModeToKey(static_cast<int>(layer.blendMode()));
    writeBytes(buf, bmKey.c_str(), 4);

    // 不透明度・クリッピング・フラグ
    buf.push_back(static_cast<uint8_t>(std::clamp(int(layer.opacity() * 255.f), 0, 255)));
    buf.push_back(0); // clipping (0=base)
    uint8_t flags = 0;
    if (!layer.visible()) flags |= 0x02;
    buf.push_back(flags);
    buf.push_back(0); // filler

    // Extra data: name + padding
    const size_t extraSizeOffset = buf.size();
    writeU32BE(buf, 0); // placeholder
    const size_t extraStart = buf.size();

    // Layer mask data (empty)
    writeU32BE(buf, 0);
    // Layer blending ranges (empty)
    writeU32BE(buf, 0);
    // Pascal string (layer name)
    writePaddedStr(buf, layer.name());
    writePadded4(buf);

    // Extra data size を書き戻す
    const size_t extraEnd = buf.size();
    const uint32_t extraSize = static_cast<uint32_t>(extraEnd - extraStart);
    buf[extraSizeOffset+0] = extraSize >> 24;
    buf[extraSizeOffset+1] = extraSize >> 16;
    buf[extraSizeOffset+2] = extraSize >> 8;
    buf[extraSizeOffset+3] = extraSize;
  }

  // Channel image data for each layer (圧縮: PackBits = 1)
  for (int li = numLayers - 1; li >= 0; --li) {
    const core::Layer& layer = doc.layerAt(li);
    // 圧縮方式
    writeU16BE(buf, 1); // PackBits

    writeLayerChannelData(buf, layer.buffer(),
                          layer.offsetX(), layer.offsetY(), W, H, W, H);
  }

  // Layer info セクションサイズを書き戻す
  {
    const uint32_t sz = static_cast<uint32_t>(buf.size() - layerInfoStart);
    buf[layerInfoSizeOffset+0] = sz >> 24;
    buf[layerInfoSizeOffset+1] = sz >> 16;
    buf[layerInfoSizeOffset+2] = sz >> 8;
    buf[layerInfoSizeOffset+3] = sz;
  }

  // Global layer mask info (empty)
  writeU32BE(buf, 0);

  // Layer section サイズを書き戻す
  {
    const uint32_t sz = static_cast<uint32_t>(buf.size() - layerSectionStart);
    buf[layerSectionSizeOffset+0] = sz >> 24;
    buf[layerSectionSizeOffset+1] = sz >> 16;
    buf[layerSectionSizeOffset+2] = sz >> 8;
    buf[layerSectionSizeOffset+3] = sz;
  }

  // ── 5. 合成済み画像データ (Raw = 0 圧縮) ─────────────────────────────────
  writeU16BE(buf, 0); // 圧縮方式: Raw

  // 合成レンダリング
  core::Renderer renderer;
  core::PixelBuffer composite = renderer.composite(doc);
  // チャンネル順: R G B A (各チャンネル全行)
  const int chanOrder[4] = {0, 1, 2, 3};
  for (int ch = 0; ch < 4; ++ch) {
    for (int y = 0; y < H; ++y) {
      for (int x = 0; x < W; ++x) {
        const core::Color c = composite.pixel(x, y);
        switch (chanOrder[ch]) {
          case 0: buf.push_back(c.r); break;
          case 1: buf.push_back(c.g); break;
          case 2: buf.push_back(c.b); break;
          case 3: buf.push_back(c.a); break;
        }
      }
    }
  }

  // ── ファイル書き出し ──────────────────────────────────────────────────────
  std::ofstream ofs(path, std::ios::binary);
  if (!ofs) return {false, "Cannot open file: " + path};
  ofs.write(reinterpret_cast<const char*>(buf.data()),
            static_cast<std::streamsize>(buf.size()));
  if (!ofs) return {false, "Write error"};
  return {true, {}};
}

} // namespace app::psd
