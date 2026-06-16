#include "app/bridge/PsdImporter.h"
#include "app/bridge/PsdUtils.h"

#include <fstream>
#include <vector>
#include <algorithm>
#include <cstring>

#include "core/document/Document.h"
#include "core/layer/Layer.h"
#include "core/buffer/PixelBuffer.h"
#include "core/color/Color.h"

namespace app::psd {

namespace {

struct PsdLayerInfo {
  int32_t  top{}, left{}, bottom{}, right{};
  uint16_t numChannels{};
  struct ChanInfo { int16_t id{}; uint32_t len{}; };
  std::vector<ChanInfo> channels;
  char     blendKey[5]{};
  uint8_t  opacity{255};
  uint8_t  flags{};
  std::string name;
};

} // namespace

ImportResult importPsd(const std::string& path, core::Document& doc) {
  std::ifstream ifs(path, std::ios::binary);
  if (!ifs) return {false, "Cannot open: " + path};

  std::vector<uint8_t> raw(std::istreambuf_iterator<char>(ifs), {});
  if (raw.size() < 26) return {false, "File too small"};

  const uint8_t* p = raw.data();
  const size_t   sz = raw.size();
  size_t pos = 0;

  auto need = [&](size_t n) -> bool { return pos + n <= sz; };
  auto ru16 = [&]() -> uint16_t { if (!need(2)) return 0; uint16_t v = readU16BE(p+pos); pos+=2; return v; };
  auto ru32 = [&]() -> uint32_t { if (!need(4)) return 0; uint32_t v = readU32BE(p+pos); pos+=4; return v; };
  auto ri32 = [&]() -> int32_t  { return static_cast<int32_t>(ru32()); };
  auto skip  = [&](size_t n)    { pos = std::min(pos+n, sz); };

  // ── File Header ──────────────────────────────────────────────────────────
  if (std::memcmp(p, "8BPS", 4) != 0) return {false, "Not a PSD file"};
  skip(4);
  const uint16_t version = ru16();
  if (version != 1) return {false, "Only PSD v1 supported (not PSB)"};
  skip(6); // reserved
  ru16(); // channels (we use RGBA)
  const int H = static_cast<int>(ru32());
  const int W = static_cast<int>(ru32());
  ru16(); // bits/channel — we only support 8
  ru16(); // color mode

  // ── Color Mode Data ───────────────────────────────────────────────────────
  skip(ru32());

  // ── Image Resources ───────────────────────────────────────────────────────
  skip(ru32());

  // ── Layer and Mask Info ──────────────────────────────────────────────────
  const uint32_t layerSectionLen = ru32();
  const size_t   layerSectionEnd = pos + layerSectionLen;

  std::vector<PsdLayerInfo> layers;

  if (layerSectionLen > 0) {
    const uint32_t layerInfoLen = ru32();
    const size_t   layerInfoEnd = pos + layerInfoLen;

    int32_t layerCount = static_cast<int16_t>(ru16()); // signed
    const bool hasAlpha = (layerCount < 0);
    if (hasAlpha) layerCount = -layerCount;

    for (int li = 0; li < layerCount; ++li) {
      PsdLayerInfo info;
      info.top    = ri32();
      info.left   = ri32();
      info.bottom = ri32();
      info.right  = ri32();

      info.numChannels = ru16();
      for (uint16_t ci = 0; ci < info.numChannels; ++ci) {
        PsdLayerInfo::ChanInfo ch;
        ch.id  = static_cast<int16_t>(ru16());
        ch.len = ru32();
        info.channels.push_back(ch);
      }

      // blend mode signature + key
      skip(4); // "8BIM"
      char key[5]{};
      if (need(4)) { std::memcpy(key, p+pos, 4); skip(4); }
      std::memcpy(info.blendKey, key, 5);

      info.opacity = p[pos++];
      skip(1); // clipping
      info.flags = p[pos++];
      skip(1); // filler

      const uint32_t extraLen = ru32();
      const size_t   extraEnd = pos + extraLen;

      // mask data
      skip(ru32());
      // blending ranges
      skip(ru32());

      // Pascal string name
      if (pos < extraEnd) {
        const uint8_t nameLen = p[pos++];
        info.name.assign(reinterpret_cast<const char*>(p+pos), nameLen);
        pos += nameLen;
        // pad to even
        if ((nameLen + 1) % 2 != 0) skip(1);
      }
      pos = extraEnd;
      layers.push_back(std::move(info));
    }

    // Channel image data for each layer
    // (read into temporary buffers, decode later)
    for (auto& info : layers) {
      const int lW = info.right  - info.left;
      const int lH = info.bottom - info.top;
      if (lW <= 0 || lH <= 0) continue;

      // Map: channel id → row data buffer
      // channels: -1=A, 0=R, 1=G, 2=B
      std::vector<uint8_t> chanData[4]; // [0]=R [1]=G [2]=B [3]=A
      // index by channel id
      auto idxForChan = [](int16_t id) -> int {
        if (id == -1) return 3;
        if (id >= 0 && id <= 2) return id;
        return -1;
      };

      for (auto& ch : info.channels) {
        const uint16_t compress = ru16();
        const int idx = idxForChan(ch.id);

        if (idx < 0) {
          // skip unknown channel
          const size_t chStart = pos;
          skip(ch.len - 2);
          continue;
        }

        chanData[idx].resize(static_cast<size_t>(lW) * lH, 0);

        if (compress == 0) {
          // Raw
          for (int row = 0; row < lH; ++row)
            for (int col = 0; col < lW; ++col)
              chanData[idx][static_cast<size_t>(row)*lW+col] = p[pos++];
        } else if (compress == 1) {
          // PackBits — row bytecounts first
          std::vector<uint16_t> rowBytes(lH);
          for (int row = 0; row < lH; ++row) rowBytes[row] = ru16();
          for (int row = 0; row < lH; ++row) {
            const size_t srcLen = rowBytes[row];
            if (pos + srcLen > sz) break;
            packBitsDecode(p+pos, static_cast<int>(srcLen),
                           chanData[idx].data() + static_cast<size_t>(row)*lW, lW);
            pos += srcLen;
          }
        } else {
          // ZIP など — skip
          skip(ch.len - 2);
        }

        // Fill destination layer
        if (chanData[0].size() && chanData[1].size() && chanData[2].size() && chanData[3].size()) {
          // All 4 channels ready — will composite after the loop
        }
      }

      // Build PixelBuffer from channel data
      // (only if all 4 channels are present)
      const bool hasR = !chanData[0].empty();
      const bool hasG = !chanData[1].empty();
      const bool hasB = !chanData[2].empty();
      const bool hasA = !chanData[3].empty();

      // Store decoded channel data in info for later document build
      info.channels.clear(); // repurpose as marker
      // We'll just build the layer directly here
      core::PixelBuffer layerBuf(W, H);
      for (int row = 0; row < lH; ++row) {
        for (int col = 0; col < lW; ++col) {
          const int cx = info.left + col;
          const int cy = info.top  + row;
          if (cx < 0 || cy < 0 || cx >= W || cy >= H) continue;
          const size_t idx2 = static_cast<size_t>(row)*lW+col;
          core::Color c;
          c.r = hasR ? chanData[0][idx2] : 0;
          c.g = hasG ? chanData[1][idx2] : 0;
          c.b = hasB ? chanData[2][idx2] : 0;
          c.a = hasA ? chanData[3][idx2] : 255;
          layerBuf.setPixel(cx, cy, c);
        }
      }

      // ドキュメントにレイヤーを追加
      const bool visible = !(info.flags & 0x02);
      const int blendModeInt = blendModeFromKey(std::string(info.blendKey, 4));

      doc.addLayer(info.name.empty() ? "Layer" : info.name, core::LayerKind::Raster);
      core::Layer& L = doc.layerAt(doc.layerCount() - 1);
      L.buffer() = std::move(layerBuf);
      L.setVisible(visible);
      L.setOpacity(static_cast<float>(info.opacity) / 255.f);
      L.setBlendMode(static_cast<core::BlendMode>(blendModeInt));
    }
    pos = layerInfoEnd;
  }

  pos = layerSectionEnd;
  return {true, {}};
}

} // namespace app::psd
