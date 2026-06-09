#include "app/bridge/LpaExporter.h"

#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QtCore/private/qzipwriter_p.h>

#include "core/document/Document.h"
#include "core/layer/Layer.h"
#include "platform/qt/QtImageConverter.h"

namespace app::lpa {

namespace {

// ── 列挙→文字列 ────────────────────────────────────────────────────────────

QString kindToString(core::LayerKind k) noexcept {
    switch (k) {
        case core::LayerKind::Raster:     return QStringLiteral("raster");
        case core::LayerKind::Vector:     return QStringLiteral("vector");
        case core::LayerKind::Folder:     return QStringLiteral("folder");
        case core::LayerKind::Adjustment: return QStringLiteral("adjustment");
        case core::LayerKind::Text:       return QStringLiteral("text");
    }
    return QStringLiteral("raster");
}

QString adjKindToString(core::AdjustmentKind k) noexcept {
    switch (k) {
        case core::AdjustmentKind::BrightnessContrast: return QStringLiteral("BrightnessContrast");
        case core::AdjustmentKind::HueSaturation:      return QStringLiteral("HueSaturation");
        case core::AdjustmentKind::ColorBalance:       return QStringLiteral("ColorBalance");
        case core::AdjustmentKind::Levels:             return QStringLiteral("Levels");
        case core::AdjustmentKind::Curves:             return QStringLiteral("Curves");
        case core::AdjustmentKind::GradientMap:        return QStringLiteral("GradientMap");
        case core::AdjustmentKind::Invert:             return QStringLiteral("Invert");
        case core::AdjustmentKind::Threshold:          return QStringLiteral("Threshold");
        case core::AdjustmentKind::Vibrance:           return QStringLiteral("Vibrance");
    }
    return QStringLiteral("BrightnessContrast");
}

// ── JSON ヘルパー ───────────────────────────────────────────────────────────

QJsonObject colorToJson(const core::Color& c) noexcept {
    QJsonObject o;
    o["r"] = c.r;
    o["g"] = c.g;
    o["b"] = c.b;
    o["a"] = c.a;
    return o;
}

QJsonObject serializeAdjustment(const core::AdjustmentParams& p) {
    QJsonObject o;
    o["kind"]        = adjKindToString(p.kind);
    o["brightness"]  = static_cast<double>(p.brightness);
    o["contrast"]    = static_cast<double>(p.contrast);
    o["hue"]         = static_cast<double>(p.hue);
    o["saturation"]  = static_cast<double>(p.saturation);
    o["lightness"]   = static_cast<double>(p.lightness);
    o["vibrance"]    = static_cast<double>(p.vibrance);
    o["inputBlack"]  = static_cast<double>(p.inputBlack);
    o["inputWhite"]  = static_cast<double>(p.inputWhite);
    o["gamma"]       = static_cast<double>(p.gamma);
    o["outputBlack"] = static_cast<double>(p.outputBlack);
    o["outputWhite"] = static_cast<double>(p.outputWhite);
    o["threshold"]   = static_cast<double>(p.threshold);
    return o;
}

// PixelBuffer → PNG bytes
QByteArray bufferToPng(const core::PixelBuffer& buf) {
    const QImage img = platform::qt::QtImageConverter::toQImage(buf);
    QByteArray data;
    QBuffer qbuf(&data);
    qbuf.open(QIODevice::WriteOnly);
    img.save(&qbuf, "PNG");
    return data;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// saveLpa
// ─────────────────────────────────────────────────────────────────────────────
SaveResult saveLpa(const core::Document& doc, const std::string& pathStr) {

    // ── ZIP を QBuffer に構築 ──────────────────────────────────────────────
    QByteArray zipData;
    {
        QBuffer zipBuf(&zipData);
        zipBuf.open(QIODevice::WriteOnly);
        QZipWriter zip(&zipBuf);
        zip.setCompressionPolicy(QZipWriter::AutoCompress);

        // ── project.json ─────────────────────────────────────────────────
        QJsonObject root;
        root["version"] = 1;
        root["app"]     = QStringLiteral("LayeredPaintApp");

        {
            QJsonObject canvas;
            canvas["width"]  = doc.canvasSize().width;
            canvas["height"] = doc.canvasSize().height;
            canvas["dpi"]    = doc.dpi();
            root["canvas"]   = canvas;
        }

        {
            QJsonObject paper;
            paper["visible"] = doc.paperVisible();
            paper["color"]   = colorToJson(doc.paperColor());
            root["paper"]    = paper;
        }

        root["nextLayerId"]      = static_cast<int>(doc.nextLayerId());
        root["activeLayerIndex"] = static_cast<int>(doc.activeLayerIndex());

        QJsonArray layersArr;
        for (std::size_t i = 0; i < doc.layerCount(); ++i) {
            const core::Layer& layer = doc.layerAt(i);

            QJsonObject lj;
            lj["id"]             = static_cast<int>(layer.id());
            lj["parentId"]       = static_cast<int>(layer.parentId());
            lj["name"]           = QString::fromStdString(layer.name());
            lj["kind"]           = kindToString(layer.kind());
            lj["visible"]        = layer.visible();
            lj["opacity"]        = static_cast<double>(layer.opacity());
            lj["blendMode"]      = static_cast<int>(layer.blendMode());
            lj["locked"]         = layer.locked();
            lj["alphaLocked"]    = layer.alphaLocked();
            lj["positionLocked"] = layer.positionLocked();
            lj["clippedToBelow"] = layer.clippedToBelow();
            lj["hasMask"]        = layer.hasMask();
            lj["maskEnabled"]    = layer.maskEnabled();

            // ラスターピクセルデータ
            if (layer.isRaster()) {
                const QString pixFile =
                    QStringLiteral("layers/%1.png").arg(layer.id());
                lj["pixelFile"] = pixFile;
                zip.addFile(pixFile, bufferToPng(layer.buffer()));
            }

            // マスク
            if (layer.hasMask()) {
                const QString maskFile =
                    QStringLiteral("layers/%1_mask.png").arg(layer.id());
                lj["maskFile"] = maskFile;
                zip.addFile(maskFile, bufferToPng(layer.maskBuffer()));
            }

            // ベクターパス
            if (layer.isVector()) {
                QJsonArray paths;
                for (const core::VectorPath& vp : layer.vectorPaths()) {
                    QJsonObject pj;
                    QJsonArray pts;
                    for (const core::FPoint& pt : vp.points) {
                        QJsonObject ptj;
                        ptj["x"] = static_cast<double>(pt.x);
                        ptj["y"] = static_cast<double>(pt.y);
                        pts.append(ptj);
                    }
                    pj["points"]  = pts;
                    pj["color"]   = colorToJson(vp.color);
                    pj["width"]   = vp.width;
                    pj["opacity"] = static_cast<double>(vp.opacity);
                    paths.append(pj);
                }
                lj["paths"] = paths;
            }

            // 調整レイヤーパラメータ
            if (layer.isAdjustment()) {
                lj["adjustment"] = serializeAdjustment(layer.adjustmentParams());
            }

            // テキストレイヤーデータ
            if (layer.isText()) {
                const core::TextData& td = layer.textData();
                QJsonObject tj;
                tj["text"]       = QString::fromStdString(td.text);
                tj["fontFamily"] = QString::fromStdString(td.fontFamily);
                tj["fontSize"]   = td.fontSize;
                tj["bold"]       = td.bold;
                tj["italic"]     = td.italic;
                tj["colorR"]     = td.colorR;
                tj["colorG"]     = td.colorG;
                tj["colorB"]     = td.colorB;
                tj["colorA"]     = td.colorA;
                tj["originX"]    = td.originX;
                tj["originY"]    = td.originY;
                lj["textData"]   = tj;
            }

            layersArr.append(lj);
        }
        root["layers"] = layersArr;

        // project.json を ZIP に追加
        const QJsonDocument jsonDoc(root);
        zip.addFile(QStringLiteral("project.json"),
                    jsonDoc.toJson(QJsonDocument::Indented));

        zip.close();
    } // zipBuf スコープ終了 → zipData が確定

    // ── ファイルに書き込み ────────────────────────────────────────────────
    const QString path = QString::fromStdString(pathStr);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        return {false, "ファイルを開けませんでした: " + pathStr};
    }
    if (f.write(zipData) != static_cast<qint64>(zipData.size())) {
        f.close();
        return {false, "書き込みに失敗しました: " + pathStr};
    }
    f.close();
    return {true, {}};
}

} // namespace app::lpa
