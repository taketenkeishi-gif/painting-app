#pragma once
// PSD (Photoshop Document) 読み書きユーティリティ
// Adobe PSD 仕様 (https://www.adobe.com/devnet-apps/photoshop/fileformatashtml/) に基づく実装
// 依存ライブラリなし — 標準 C++17 のみ

#include <cstdint>
#include <string>
#include <vector>

namespace app::psd {

// ─────────────────────────────────────────────────────────────────────────────
// ブレンドモード: PSD 4文字コード ↔ core::BlendMode 変換
// ─────────────────────────────────────────────────────────────────────────────
inline std::string blendModeToKey(int mode) {
  // core::BlendMode enum values → PSD 4-char keys
  switch (mode) {
    case 0:  return "norm"; // Normal
    case 1:  return "mul "; // Multiply
    case 2:  return "scrn"; // Screen
    case 3:  return "over"; // Overlay
    case 4:  return "hLit"; // Hard Light
    case 5:  return "sLit"; // Soft Light
    case 6:  return "div "; // Color Dodge
    case 7:  return "idiv"; // Color Burn
    case 8:  return "lddg"; // Linear Dodge (Add)
    case 9:  return "dkCl"; // Darken
    case 10: return "lgCl"; // Lighten
    case 11: return "diff"; // Difference
    case 12: return "smud"; // Exclusion
    default: return "norm";
  }
}

inline int blendModeFromKey(const std::string& key) {
  if (key == "norm") return 0;
  if (key == "mul ") return 1;
  if (key == "scrn") return 2;
  if (key == "over") return 3;
  if (key == "hLit") return 4;
  if (key == "sLit") return 5;
  if (key == "div ") return 6;
  if (key == "idiv") return 7;
  if (key == "lddg") return 8;
  if (key == "dkCl") return 9;
  if (key == "lgCl") return 10;
  if (key == "diff") return 11;
  if (key == "smud") return 12;
  return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// PackBits RLE (PSD 標準圧縮)
// ─────────────────────────────────────────────────────────────────────────────
inline std::vector<uint8_t> packBitsEncode(const uint8_t* src, int len) {
  std::vector<uint8_t> out;
  out.reserve(len + len / 2 + 1);
  int i = 0;
  while (i < len) {
    // 連続したバイトを探す
    int run = 1;
    while (i + run < len && run < 128 && src[i+run] == src[i]) ++run;
    if (run > 1) {
      out.push_back(static_cast<uint8_t>(-(run - 1)));
      out.push_back(src[i]);
      i += run;
    } else {
      // リテラルラン
      int lit = 1;
      while (i + lit < len && lit < 128) {
        if (i + lit + 1 < len && src[i+lit] == src[i+lit+1]) break;
        ++lit;
      }
      out.push_back(static_cast<uint8_t>(lit - 1));
      for (int j = 0; j < lit; ++j) out.push_back(src[i+j]);
      i += lit;
    }
  }
  return out;
}

inline void packBitsDecode(const uint8_t* src, int srcLen, uint8_t* dst, int dstLen) {
  int si = 0, di = 0;
  while (si < srcLen && di < dstLen) {
    const int8_t hdr = static_cast<int8_t>(src[si++]);
    if (hdr >= 0) {
      const int n = hdr + 1;
      for (int j = 0; j < n && di < dstLen; ++j) dst[di++] = src[si++];
    } else if (hdr != -128) {
      const int n = -static_cast<int>(hdr) + 1;
      const uint8_t val = src[si++];
      for (int j = 0; j < n && di < dstLen; ++j) dst[di++] = val;
    }
  }
}

// Big-endian write helpers
inline void writeU16BE(std::vector<uint8_t>& buf, uint16_t v) {
  buf.push_back(uint8_t(v >> 8));
  buf.push_back(uint8_t(v));
}
inline void writeU32BE(std::vector<uint8_t>& buf, uint32_t v) {
  buf.push_back(uint8_t(v >> 24));
  buf.push_back(uint8_t(v >> 16));
  buf.push_back(uint8_t(v >> 8));
  buf.push_back(uint8_t(v));
}
inline void writeI32BE(std::vector<uint8_t>& buf, int32_t v) {
  writeU32BE(buf, static_cast<uint32_t>(v));
}
inline void writeBytes(std::vector<uint8_t>& buf, const void* src, size_t n) {
  const auto* p = reinterpret_cast<const uint8_t*>(src);
  buf.insert(buf.end(), p, p + n);
}
inline void writePadded4(std::vector<uint8_t>& buf) {
  while (buf.size() % 4 != 0) buf.push_back(0);
}
inline void writePaddedStr(std::vector<uint8_t>& buf, const std::string& s) {
  uint8_t len = static_cast<uint8_t>(std::min(s.size(), size_t(255)));
  buf.push_back(len);
  for (uint8_t i = 0; i < len; ++i) buf.push_back(static_cast<uint8_t>(s[i]));
  // PSD Pascal string: pad to even size
  if ((len + 1) % 2 != 0) buf.push_back(0);
}

// Big-endian read helpers
inline uint16_t readU16BE(const uint8_t* p) {
  return (uint16_t(p[0]) << 8) | p[1];
}
inline uint32_t readU32BE(const uint8_t* p) {
  return (uint32_t(p[0])<<24)|(uint32_t(p[1])<<16)|(uint32_t(p[2])<<8)|p[3];
}
inline int32_t readI32BE(const uint8_t* p) {
  return static_cast<int32_t>(readU32BE(p));
}

} // namespace app::psd
