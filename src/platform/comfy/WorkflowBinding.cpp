#include "platform/comfy/WorkflowBinding.h"

namespace platform::comfy {

WorkflowBinding::WorkflowBinding(Target target, const QString& nodeId,
                                   const QString& classType, int occurrence)
    : m_target    (target)
    , m_nodeId    (nodeId)
    , m_classType (classType)
    , m_occurrence(occurrence)
{}

// ─────────────────────────────────────────────────────────────────────────────
// ファクトリ — nodeId 指定
// ─────────────────────────────────────────────────────────────────────────────
WorkflowBinding WorkflowBinding::loadImage(const QString& nodeId, const QString& filename) {
    WorkflowBinding b(Target::LoadImage, nodeId);
    b.m_patches["image"]  = filename;
    b.m_patches["upload"] = QString("image");
    return b;
}

WorkflowBinding WorkflowBinding::saveImage(const QString& nodeId, const QString& prefix) {
    WorkflowBinding b(Target::SaveImage, nodeId);
    b.m_patches["filename_prefix"] = prefix;
    return b;
}

WorkflowBinding WorkflowBinding::clipText(const QString& nodeId, const QString& text) {
    WorkflowBinding b(Target::CLIPTextEncode, nodeId);
    b.m_patches["text"] = text;
    return b;
}

WorkflowBinding WorkflowBinding::kSampler(const QString& nodeId) {
    return WorkflowBinding(Target::KSampler, nodeId);
}

WorkflowBinding WorkflowBinding::controlNet(const QString& nodeId) {
    return WorkflowBinding(Target::ControlNet, nodeId);
}

// ─────────────────────────────────────────────────────────────────────────────
// ファクトリ — classType 検索
// ─────────────────────────────────────────────────────────────────────────────
WorkflowBinding WorkflowBinding::byClass(const QString& classType, int occurrence) {
    // classType から Target を推測
    Target t = Target::Generic;
    if      (classType == "LoadImage")       t = Target::LoadImage;
    else if (classType == "SaveImage")       t = Target::SaveImage;
    else if (classType == "CLIPTextEncode")  t = Target::CLIPTextEncode;
    else if (classType == "KSampler" || classType == "KSamplerAdvanced")
                                              t = Target::KSampler;
    else if (classType.contains("ControlNet")) t = Target::ControlNet;

    WorkflowBinding b(t, {}, classType, occurrence);
    return b;
}

// ─────────────────────────────────────────────────────────────────────────────
// Fluent setters
// ─────────────────────────────────────────────────────────────────────────────
WorkflowBinding& WorkflowBinding::seed(int s) {
    m_patches["seed"] = s;
    return *this;
}

WorkflowBinding& WorkflowBinding::steps(int s) {
    m_patches["steps"] = s;
    return *this;
}

WorkflowBinding& WorkflowBinding::cfg(double c) {
    m_patches["cfg"] = c;
    return *this;
}

WorkflowBinding& WorkflowBinding::denoise(double d) {
    m_patches["denoise"] = d;
    return *this;
}

WorkflowBinding& WorkflowBinding::sampler(const QString& name) {
    m_patches["sampler_name"] = name;
    return *this;
}

WorkflowBinding& WorkflowBinding::scheduler(const QString& name) {
    m_patches["scheduler"] = name;
    return *this;
}

WorkflowBinding& WorkflowBinding::strength(double s) {
    m_patches["strength"] = s;
    return *this;
}

WorkflowBinding& WorkflowBinding::image(const QString& filename) {
    m_patches["image"]  = filename;
    m_patches["upload"] = QString("image");
    return *this;
}

WorkflowBinding& WorkflowBinding::set(const QString& key, const QJsonValue& value) {
    m_patches[key] = value;
    return *this;
}

} // namespace platform::comfy
