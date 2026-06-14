#pragma once

#include <QJsonObject>
#include <QString>

namespace app::bridge {

// WorkflowFactory — Comfy ワークフロー JSON 生成専用クラス。
// HTTP 送信・UI 操作は行わない。
class WorkflowFactory {
public:
  struct SamParams {
    QString uploadedFilename;
    int     pointX        {0};
    int     pointY        {0};
    bool    positivePoint {true};
    QString samModel      {"sam2_hiera_large.pt"};
  };

  static QJsonObject buildSamWorkflow    (const SamParams& p);
  static QJsonObject buildUpscaleWorkflow(const QString& inputFilename,
                                          const QString& modelName);
};

} // namespace app::bridge
