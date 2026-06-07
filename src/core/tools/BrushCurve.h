#pragma once
// BrushCurve — multi-point input→output mapping curve
//
// アルゴリズム吸収元: libmypaint/brushlib/mypaint-mapping.c (MIT License)
//   https://github.com/mypaint/libmypaint
//
// libmypaint は GIMP・MyPaint・Krita のブラシエンジンとして使われており、
// CSP も同等のカーブシステムを「感度設定」として実装していると推測される。
//
// オリジナルの C 実装を C++17 ヘッダオンリーに移植:
//  - ControlPoint の配列を std::vector に変更
//  - Catmull-Rom 接線推定 → cubic Hermite 補間
//  - fromGamma() でレガシーな γ パラメータとの互換を保つ

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <vector>

namespace core {

class BrushCurve {
public:
  struct Point { float x, y; };

  // デフォルト: [(0,0)→(1,1)] の線形マップ（gamma=1.0 と等価）
  BrushCurve() { setLinear(); }

  // ── ファクトリ ─────────────────────────────────────────────────────────────

  static BrushCurve linear() {
    BrushCurve c; c.setLinear(); return c;
  }

  // libmypaint の sigmoid-like γ 近似。
  // gamma < 1: 軽タッチで最大反応 / gamma > 1: 強い圧が必要
  static BrushCurve fromGamma(float gamma) {
    BrushCurve c;
    c.m_points.clear();
    // 5 点でガンマ曲線を近似 (Casteljau でも可だが分かりやすさ優先)
    constexpr int kSteps = 5;
    for (int i = 0; i <= kSteps; ++i) {
      const float x = static_cast<float>(i) / kSteps;
      const float y = std::pow(x, gamma);
      c.m_points.push_back({x, y});
    }
    return c;
  }

  // CSP の「感度カーブ」に相当するプリセット
  static BrushCurve soft() {
    // 軽い圧でほぼ最大（水彩・マーカー向き）
    return fromGamma(0.4f);
  }
  static BrushCurve hard() {
    // 強く押さないと反応しない（鉛筆・エンピツ向き）
    return fromGamma(2.5f);
  }
  static BrushCurve sCurve() {
    // S 字カーブ: 中間圧でリニア、両端で緩やか
    BrushCurve c;
    c.m_points = {{0.0f,0.0f},{0.25f,0.1f},{0.5f,0.5f},{0.75f,0.9f},{1.0f,1.0f}};
    return c;
  }

  // ── 編集 API ──────────────────────────────────────────────────────────────

  void setLinear() {
    m_points = {{0.0f, 0.0f}, {1.0f, 1.0f}};
  }

  void setPoints(std::initializer_list<Point> pts) {
    m_points.assign(pts.begin(), pts.end());
    sortAndClamp();
  }

  void addPoint(float x, float y) {
    m_points.push_back({std::clamp(x,0.0f,1.0f), std::clamp(y,0.0f,1.0f)});
    sortAndClamp();
  }

  // ── 評価 ─────────────────────────────────────────────────────────────────

  // x ∈ [0,1] → y ∈ [0,1] を cubic Hermite スプラインで補間。
  // libmypaint の mypaint_mapping_get_base_value() と同等の計算。
  float evaluate(float x) const noexcept {
    const auto& pts = m_points;
    const int n = static_cast<int>(pts.size());
    if (n == 0) return x;
    if (n == 1) return pts[0].y;

    x = std::clamp(x, 0.0f, 1.0f);

    // 2 点のとき: 線形補間（高速パス）
    if (n == 2) {
      const float dx = pts[1].x - pts[0].x;
      if (dx < 1e-6f) return pts[0].y;
      return pts[0].y + (pts[1].y - pts[0].y) * (x - pts[0].x) / dx;
    }

    // 所属セグメントを binary search で特定
    int seg = 0;
    {
      int lo = 0, hi = n - 2;
      while (lo < hi) {
        int mid = (lo + hi + 1) / 2;
        if (pts[mid].x <= x) lo = mid; else hi = mid - 1;
      }
      seg = lo;
    }

    const float x0 = pts[seg].x,   y0 = pts[seg].y;
    const float x1 = pts[seg+1].x, y1 = pts[seg+1].y;
    const float dx = x1 - x0;
    if (dx < 1e-6f) return y0;

    const float t = (x - x0) / dx;

    // Catmull-Rom 接線推定 (libmypaint 方式)
    const float m0 = catmullTangent(pts, seg,   dx);
    const float m1 = catmullTangent(pts, seg+1, dx);

    // Cubic Hermite 基底関数 (De Casteljau より数値安定)
    const float t2 = t * t;
    const float t3 = t2 * t;
    const float h00 =  2*t3 - 3*t2 + 1;
    const float h10 =    t3 - 2*t2 + t;
    const float h01 = -2*t3 + 3*t2;
    const float h11 =    t3 -   t2;

    const float y = h00*y0 + h10*m0*dx + h01*y1 + h11*m1*dx;
    return std::clamp(y, 0.0f, 1.0f);
  }

  bool isLinear() const noexcept {
    if (m_points.size() != 2) return false;
    return m_points[0].x == 0.0f && m_points[0].y == 0.0f
        && m_points[1].x == 1.0f && m_points[1].y == 1.0f;
  }

  const std::vector<Point>& points() const noexcept { return m_points; }

private:
  std::vector<Point> m_points;

  void sortAndClamp() {
    for (auto& p : m_points) {
      p.x = std::clamp(p.x, 0.0f, 1.0f);
      p.y = std::clamp(p.y, 0.0f, 1.0f);
    }
    std::sort(m_points.begin(), m_points.end(),
              [](const Point& a, const Point& b){ return a.x < b.x; });
  }

  // Catmull-Rom 方式の接線推定 (libmypaint mypaint-mapping.c と同アルゴリズム)
  static float catmullTangent(const std::vector<Point>& pts, int i, float /*refDx*/) noexcept {
    const int n = static_cast<int>(pts.size());
    if (n < 2) return 0.0f;
    const int prev = std::max(0, i - 1);
    const int next = std::min(n - 1, i + 1);
    if (prev == next) return 0.0f;
    const float dxPN = pts[next].x - pts[prev].x;
    if (dxPN < 1e-6f) return 0.0f;
    return (pts[next].y - pts[prev].y) / dxPN;
  }
};

} // namespace core
