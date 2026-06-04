#include "core/ai/GenerativeFillEngine.h"

#include <cmath>
#include <cstdlib>
#include <random>

#include <QTimer>
#include <QThread>

namespace core::ai {

GenerativeFillEngine::GenerativeFillEngine(QObject* parent)
    : QObject(parent) {}

// ─────────────────────────────────────────────────────────────────────────────
// generate
// ─────────────────────────────────────────────────────────────────────────────
void GenerativeFillEngine::generate(const core::PixelBuffer& canvas,
                                    const core::SelectionMask& mask,
                                    const Settings& settings) {
  if (m_running) {
    emit errorOccurred("既に生成中です。キャンセルしてから再試行してください。");
    return;
  }

  if (settings.backend == Backend::LocalOnnx) {
    emit errorOccurred("ローカル ONNX バックエンドは未実装です。");
    return;
  }
  if (settings.backend == Backend::ApiRemote) {
    emit errorOccurred("リモート API バックエンドは未実装です。");
    return;
  }

  m_running         = true;
  m_cancelRequested = false;

  // Stub をタイマーで非同期に実行（UI をブロックしないようステップを分割）
  const int totalSteps = std::max(1, settings.steps);
  auto* timer         = new QTimer(this);
  auto stepCount      = std::make_shared<int>(0);
  auto canvasCopy     = std::make_shared<core::PixelBuffer>(canvas);
  auto maskCopy       = std::make_shared<core::SelectionMask>(mask);
  auto settingsCopy   = std::make_shared<Settings>(settings);

  connect(timer, &QTimer::timeout, this, [=]() {
    if (m_cancelRequested) {
      timer->stop();
      timer->deleteLater();
      m_running = false;
      emit progressChanged(0);
      return;
    }

    ++(*stepCount);
    const int pct = (*stepCount * 100) / totalSteps;
    emit progressChanged(std::min(pct, 99));

    if (*stepCount >= totalSteps) {
      timer->stop();
      timer->deleteLater();

      // ── Stub 生成処理 ─────────────────────────────────────────────
      core::PixelBuffer result = *canvasCopy;
      const int w = result.width();
      const int h = result.height();

      const std::uint32_t seed = (settingsCopy->seed < 0)
          ? static_cast<std::uint32_t>(std::random_device{}())
          : static_cast<std::uint32_t>(settingsCopy->seed);

      std::mt19937 rng(seed);
      std::uniform_int_distribution<int> colorDist(0, 255);
      std::normal_distribution<float>    noiseDist(0.0f, 40.0f);

      const bool hasSel = maskCopy->hasSelection();
      const float strength = std::clamp(settingsCopy->strength, 0.0f, 1.0f);

      for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
          if (hasSel && !maskCopy->contains(x, y)) {
            continue;
          }
          const core::Color src = canvasCopy->pixel(x, y);

          // プロンプト長に基づいてヒューシフト（スタブ演出）
          const float promptFactor = std::min(1.0f,
              static_cast<float>(settingsCopy->prompt.length()) / 32.0f);
          const float hueShift = promptFactor * 60.0f;  // 0–60° shift

          // HSV 変換（簡易）
          const float r = src.r / 255.0f;
          const float g = src.g / 255.0f;
          const float b = src.b / 255.0f;
          const float cmax = std::max({r, g, b});
          const float cmin = std::min({r, g, b});
          const float delta = cmax - cmin;

          float hue = 0.0f;
          if (delta > 1e-6f) {
            if (cmax == r) {
              hue = 60.0f * std::fmod((g - b) / delta, 6.0f);
            } else if (cmax == g) {
              hue = 60.0f * ((b - r) / delta + 2.0f);
            } else {
              hue = 60.0f * ((r - g) / delta + 4.0f);
            }
          }
          if (hue < 0.0f) hue += 360.0f;
          hue = std::fmod(hue + hueShift, 360.0f);

          const float sat = (cmax > 1e-6f) ? (delta / cmax) : 0.0f;
          const float val = cmax;

          // HSV → RGB
          const float hh = hue / 60.0f;
          const int   ii = static_cast<int>(hh);
          const float ff = hh - static_cast<float>(ii);
          const float pp = val * (1.0f - sat);
          const float qq = val * (1.0f - sat * ff);
          const float tt = val * (1.0f - sat * (1.0f - ff));
          float nr = 0.0f, ng = 0.0f, nb = 0.0f;
          switch (ii % 6) {
            case 0: nr=val; ng=tt;  nb=pp;  break;
            case 1: nr=qq;  ng=val; nb=pp;  break;
            case 2: nr=pp;  ng=val; nb=tt;  break;
            case 3: nr=pp;  ng=qq;  nb=val; break;
            case 4: nr=tt;  ng=pp;  nb=val; break;
            default:nr=val; ng=pp;  nb=qq;  break;
          }

          // ノイズ + ブレンド
          const float noise = noiseDist(rng);
          const auto blend = [&](float orig, float genVal) -> std::uint8_t {
            const float mixed = orig * (1.0f - strength) + (genVal + noise / 255.0f) * strength;
            return static_cast<std::uint8_t>(std::clamp(mixed * 255.0f, 0.0f, 255.0f));
          };

          core::Color out;
          out.r = blend(r, nr);
          out.g = blend(g, ng);
          out.b = blend(b, nb);
          out.a = src.a;
          result.setPixel(x, y, out);
        }
      }

      m_running = false;
      emit progressChanged(100);
      emit resultReady(result);
    }
  });

  // 1 ステップあたり ~40ms（合計: steps × 40ms、デフォルト 20steps = 800ms）
  timer->start(40);
}

void GenerativeFillEngine::cancel() {
  m_cancelRequested = true;
}

} // namespace core::ai
