#pragma once

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QPixmap>
#include <QString>
#include <QStringList>

#include "platform/comfy/WorkflowBinding.h"
#include "platform/comfy/WorkflowDocument.h"

namespace platform::comfy { class ComfyClient; }

namespace app::bridge {

class AiGenerationController;
class AppController;

// ─────────────────────────────────────────────────────────────────────────────
// AiService
//
// AI 生成機能の Facade。ComfyClient / AiGenerationController の生成・管理を
// AppController から隠蔽する。
//
// 現在の POC 対象: generate (WorkflowBinding ベースのカスタムワークフロー)
// inpaint / selectMask / upscale は今後順次移行予定。
// ─────────────────────────────────────────────────────────────────────────────
class AiService : public QObject {
  Q_OBJECT
public:
  explicit AiService(AppController* appController, QObject* parent = nullptr);

  void    setComfyUrl(const QString& url);
  QString comfyUrl()             const { return m_url; }
  bool    isBusy()               const;
  int     controllerInstanceId() const;

  // ── generate (WorkflowBinding ベース) ─────────────────────────────────────
  struct GenerateRequest {
    QString workflowPath;
    platform::comfy::WorkflowDocument workflowDoc;
    bool useActiveLayer       {true};
    bool useCompositedBuffer  {false};
    QString inputImageNodeId;
    bool useSelectionAsMask   {false};
    QString maskImageNodeId;
    QList<platform::comfy::WorkflowBinding> extraBindings;
    QString outputLayerName   {"AI 生成"};
    int timeoutMs             {90000};
  };

  void generate(const GenerateRequest& req, int batchCount = 1);
  void inpaint (const GenerateRequest& req, int batchCount = 1);

  // ── selectMask (SAM) ──────────────────────────────────────────────────────
  struct SelectMaskRequest {
    QByteArray imagePng;
    int        pointX        {0};
    int        pointY        {0};
    bool       positivePoint {true};
    QString    samModel      {"sam2_hiera_large.pt"};
    int        timeoutMs     {60000};
  };

  void selectMask(const SelectMaskRequest& req);

  // ── upscale ───────────────────────────────────────────────────────────────
  struct UpscaleRequest {
    QByteArray imagePng;
    QString    modelName;
    int        timeoutMs {120000};
  };

  void upscale(const UpscaleRequest& req);
  void fetchUpscaleModels();

  /// ComfyUI の実行中プロンプトを中断する。初期化前に呼んでも安全 (no-op)。
  void cancel();

signals:
  void generationStarted();
  void generationProgressUpdate(int step, int totalSteps);
  void generationFinished(const QString& opType);
  void generationError(const QString& message);
  void batchCandidatesReady(const QList<QPixmap>& candidates);

  void selectMaskResult(QByteArray maskPng);
  void selectMaskError (QString message);

  void upscaleResult(QByteArray imagePng);
  void upscaleError (QString message);
  void upscaleModelsReady(QStringList models);

private:
  void ensureInitialized();

  AppController*                 m_appController   {nullptr};
  platform::comfy::ComfyClient*  m_comfyClient     {nullptr};
  AiGenerationController*        m_genCtrl         {nullptr};
  QString                        m_url             {"http://localhost:8188"};
  QString                        m_currentOpType   {"generate"};
};

} // namespace app::bridge
