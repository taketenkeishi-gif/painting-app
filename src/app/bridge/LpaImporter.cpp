#include "app/bridge/LpaImporter.h"

#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QtCore/private/qzipreader_p.h>

#include "core/document/Document.h"
#include "core/layer/Layer.h"
#include "platform/qt/QtImageConverter.h"

namespace app::lpa {

namespace {

// ── 文字列→列挙 ────────────────────────────────────────────────────────────

core::LayerKind kindFromString(const QString& s) noexcept {
    if (s == QStringLiteral("vector"))     return core::LayerKind::Vector;
    if (s == QStringLiteral("folder"))     return core::LayerKind::Folder;
    if (s == QStringLiteral("adjustment")) return core::LayerKind::Adjustment;
    return core::LayerKind::Raster;
}

core::AdjustmentKind adjKindFromString(const QString& s) noexcept {
    if (s == QStringLiteral("HueSaturation"))      return core::AdjustmentKind::HueSaturation;
    if (s == QStringLiteral("ColorBalance"))       return core::AdjustmentKind::ColorBalance;
    if (s == QStringLiteral("Levels"))             return core::AdjustmentKind::Levels;
    if (s == QStringLiteral("Curves"))             return core::AdjustmentKind::Curves;
    if (s == QStringLiteral("GradientMap"))        return core::AdjustmentKind::GradientMap;
    if (s == QStringLiteral("Invert"))             return core::AdjustmentKind::Invert;
    if (s == QStringLiteral("Threshold"))          return core::AdjustmentKind::Threshold;
    if (s == QStringLiteral("Vibrance"))           return core::AdjustmentKind::Vibrance;
    return core::AdjustmentKind::BrightnessContrast;
}

// ── JSON ヘルパー ───────────────────────────────────────────────────────────

core::Color colorFromJson(const QJsonObject& o) noexcept {
    return core::Color{
        static_cast<uint8_t>(o["r"].toInt(0)),
        static_cast<uint8_t>(o["g"].toInt(0)),
        static_cast<uint8_t>(o["b"].toInt(0)),
        static_cast<uint8_t>(o["a"].toInt(255))};
}

core::AdjustmentParams deserializeAdjustment(const QJsonObject& o) {
    core::AdjustmentParams p;
    p.kind        = adjKindFromString(o["kind"].toString());
    p.brightness  = static_cast<float>(o["brightness"].toDouble(0.0));
    p.contrast    = static_cast<float>(o["contrast"].toDouble(0.0));
    p.hue         = static_cast<float>(o["hue"].toDouble(0.0));
    p.saturation  = static_cast<float>(o["saturation"].toDouble(0.0));
    p.lightness   = static_cast<float>(o["lightness"].toDouble(0.0));
    p.vibrance    = static_cast<float>(o["vibrance"].toDouble(0.0));
    p.inputBlack  = static_cast<float>(o["inputBlack"].toDouble(0.0));
    p.inputWhite  = static_cast<float>(o["inputWhite"].toDouble(1.0));
    p.gamma       = static_cast<float>(o["gamma"].toDouble(1.0));
    p.outputBlack = static_cast<float>(o["outputBlack"].toDouble(0.0));
    p.outputWhite = static_cast<float>(o["outputWhite"].toDouble(1.0));
    p.threshold   = static_cast<float>(o["threshold"].toDouble(0.5));
    return p;
}

// PNG bytes → PixelBuffer（失敗時は空白のバッファを返す）
core::PixelBuffer pngToBuffer(const QByteArray& data, int fallbackW, int fallbackH) {
    if (data.isEmpty()) {
        return core::PixelBuffer(fallbackW, fallbackH, core::Color::Transparent());
    }
    QImage img;
    img.loadFromData(data, "PNG");
    if (img.isNull()) {
        return core::PixelBuffer(fallbackW, fallbackH, core::Color::Transparent());
    }
    return platform::qt::QtImageConverter::fromQImage(img);
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// loadLpa
// ─────────────────────────────────────────────────────────────────────────────
LoadResult loadLpa(const std::string& pathStr) {
    const QString path = QString::fromStdString(pathStr);

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return {false, "ファイルを開けませんでした: " + pathStr, nullptr};
    }
    const QByteArray zipData = f.readAll();
    f.close();

    QBuffer zipBuf;
    zipBuf.setData(zipData);
    zipBuf.open(QIODevice::ReadOnly);
    QZipReader zip(&zipBuf);

    // ── project.json ──────────────────────────────────────────────────────
    const QByteArray jsonRaw = zip.fileData(QStringLiteral("project.json"));
    if (jsonRaw.isEmpty()) {
        return {false, "project.json が見つかりません (不正な .lpa ファイル)", nullptr};
    }

    QJsonParseError parseErr;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonRaw, &parseErr);
    if (jsonDoc.isNull()) {
        return {false, "project.json の解析に失敗: " + parseErr.errorString().toStdString(), nullptr};
    }

    const QJsonObject root = jsonDoc.object();

    const int version = root["version"].toInt(0);
    if (version != 1) {
        return {false, "未対応のバージョン: " + std::to_string(version), nullptr};
    }

    // ── キャンバス作成 ─────────────────────────────────────────────────────
    const QJsonObject canvas = root["canvas"].toObject();
    const int W   = canvas["width"].toInt(800);
    const int H   = canvas["height"].toInt(600);
    const int dpi = canvas["dpi"].toInt(72);

    auto doc = std::make_unique<core::Document>(W, H, dpi);

    // paper
    const QJsonObject paper = root["paper"].toObject();
    doc->setPaperVisible(paper["visible"].toBool(true));
    doc->setPaperColor(colorFromJson(paper["color"].toObject()));

    // デフォルトで追加された Layer 1 を削除し、ロード用に白紙にする
    doc->clearLayersForLoad();

    // ── レイヤーを順番に復元 ───────────────────────────────────────────────
    const QJsonArray layersArr = root["layers"].toArray();
    for (const QJsonValue& lv : layersArr) {
        const QJsonObject lj = lv.toObject();

        const core::LayerKind kind =
            kindFromString(lj["kind"].toString(QStringLiteral("raster")));
        const std::string name =
            lj["name"].toString(QStringLiteral("Layer")).toStdString();

        core::Layer layer(name, W, H, kind);

        layer.setId(static_cast<uint32_t>(lj["id"].toInt(0)));
        layer.setParentId(static_cast<uint32_t>(lj["parentId"].toInt(0)));
        layer.setVisible(lj["visible"].toBool(true));
        layer.setOpacity(static_cast<float>(lj["opacity"].toDouble(1.0)));
        layer.setBlendMode(
            static_cast<core::BlendMode>(lj["blendMode"].toInt(0)));
        layer.setLocked(lj["locked"].toBool(false));
        layer.setAlphaLocked(lj["alphaLocked"].toBool(false));
        layer.setPositionLocked(lj["positionLocked"].toBool(false));
        layer.setClippedToBelow(lj["clippedToBelow"].toBool(false));

        // ラスターピクセルデータ
        if (kind == core::LayerKind::Raster) {
            const QString pixFile = lj["pixelFile"].toString();
            if (!pixFile.isEmpty()) {
                layer.buffer() = pngToBuffer(zip.fileData(pixFile), W, H);
            }
        }

        // マスク
        if (lj["hasMask"].toBool(false)) {
            const QString maskFile = lj["maskFile"].toString();
            if (!maskFile.isEmpty()) {
                const QByteArray maskData = zip.fileData(maskFile);
                if (!maskData.isEmpty()) {
                    layer.createMask();
                    layer.maskBuffer() = pngToBuffer(maskData, W, H);
                    layer.setMaskEnabled(lj["maskEnabled"].toBool(true));
                }
            }
        }

        // ベクターパス
        if (kind == core::LayerKind::Vector) {
            for (const QJsonValue& pv : lj["paths"].toArray()) {
                const QJsonObject pj = pv.toObject();
                core::VectorPath vp;
                for (const QJsonValue& ptv : pj["points"].toArray()) {
                    const QJsonObject ptj = ptv.toObject();
                    vp.points.push_back({
                        static_cast<float>(ptj["x"].toDouble(0.0)),
                        static_cast<float>(ptj["y"].toDouble(0.0))});
                }
                vp.color   = colorFromJson(pj["color"].toObject());
                vp.width   = pj["width"].toInt(1);
                vp.opacity = static_cast<float>(pj["opacity"].toDouble(1.0));
                layer.addVectorPath(std::move(vp));
            }
        }

        // 調整レイヤーパラメータ
        if (kind == core::LayerKind::Adjustment) {
            layer.setAdjustmentParams(
                deserializeAdjustment(lj["adjustment"].toObject()));
        }

        doc->insertLoadedLayer(std::move(layer));
    }

    // ID カウンタとアクティブレイヤーを復元
    doc->setNextLayerId(
        static_cast<uint32_t>(root["nextLayerId"].toInt(1)));
    doc->setActiveLayer(
        static_cast<std::size_t>(root["activeLayerIndex"].toInt(0)));

    return {true, {}, std::move(doc)};
}

} // namespace app::lpa
