#include "app/bridge/WorkflowFactory.h"

#include <QJsonArray>

namespace app::bridge {

// ─────────────────────────────────────────────────────────────────────────────
// buildSamWorkflow
//   1: LoadImage
//   2: SAM2ModelLoader
//   3: SAM2Segmentation
//   4: MaskToImage
//   5: SaveImage (prefix = "paintapp_sam")
// ─────────────────────────────────────────────────────────────────────────────
QJsonObject WorkflowFactory::buildSamWorkflow(const SamParams& p) {
    QJsonObject wf;

    // Node 1: LoadImage
    {
        QJsonObject inputs;
        inputs["image"] = p.uploadedFilename;
        QJsonObject node;
        node["class_type"] = "LoadImage";
        node["inputs"] = inputs;
        wf["1"] = node;
    }

    // Node 2: SAM2ModelLoader
    {
        QJsonObject inputs;
        inputs["model"] = p.samModel;
        QJsonObject node;
        node["class_type"] = "SAM2ModelLoader";
        node["inputs"] = inputs;
        wf["2"] = node;
    }

    // Node 3: SAM2Segmentation
    {
        QJsonObject inputs;
        inputs["sam2_model"] = QJsonArray{ "2", 0 };
        inputs["image"]      = QJsonArray{ "1", 0 };
        inputs["point_x"]    = p.pointX;
        inputs["point_y"]    = p.pointY;
        inputs["label"]      = p.positivePoint ? 1 : 0;
        QJsonObject node;
        node["class_type"] = "SAM2Segmentation";
        node["inputs"] = inputs;
        wf["3"] = node;
    }

    // Node 4: MaskToImage
    {
        QJsonObject inputs;
        inputs["mask"] = QJsonArray{ "3", 1 };
        QJsonObject node;
        node["class_type"] = "MaskToImage";
        node["inputs"] = inputs;
        wf["4"] = node;
    }

    // Node 5: SaveImage
    {
        QJsonObject inputs;
        inputs["images"]          = QJsonArray{ "4", 0 };
        inputs["filename_prefix"] = "paintapp_sam";
        QJsonObject node;
        node["class_type"] = "SaveImage";
        node["inputs"] = inputs;
        wf["5"] = node;
    }

    return wf;
}

// ─────────────────────────────────────────────────────────────────────────────
// buildUpscaleWorkflow
//   1: LoadImage
//   2: UpscaleModelLoader
//   3: ImageUpscaleWithModel
//   4: SaveImage (prefix = "lpa_upscale_")
// ─────────────────────────────────────────────────────────────────────────────
QJsonObject WorkflowFactory::buildUpscaleWorkflow(const QString& inputFilename,
                                                   const QString& modelName) {
    QJsonObject wf;

    // Node 1: LoadImage
    {
        QJsonObject inputs;
        inputs["image"] = inputFilename;
        QJsonObject node;
        node["class_type"] = "LoadImage";
        node["inputs"] = inputs;
        wf["1"] = node;
    }

    // Node 2: UpscaleModelLoader
    {
        QJsonObject inputs;
        inputs["model_name"] = modelName;
        QJsonObject node;
        node["class_type"] = "UpscaleModelLoader";
        node["inputs"] = inputs;
        wf["2"] = node;
    }

    // Node 3: ImageUpscaleWithModel
    {
        QJsonObject inputs;
        inputs["upscale_model"] = QJsonArray{ "2", 0 };
        inputs["image"]         = QJsonArray{ "1", 0 };
        QJsonObject node;
        node["class_type"] = "ImageUpscaleWithModel";
        node["inputs"] = inputs;
        wf["3"] = node;
    }

    // Node 4: SaveImage
    {
        QJsonObject inputs;
        inputs["images"]          = QJsonArray{ "3", 0 };
        inputs["filename_prefix"] = "lpa_upscale_";
        QJsonObject node;
        node["class_type"] = "SaveImage";
        node["inputs"] = inputs;
        wf["4"] = node;
    }

    return wf;
}

} // namespace app::bridge
