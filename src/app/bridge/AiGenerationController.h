#pragma once

#include <functional>

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QPixmap>
#include <QString>

#include "platform/comfy/WorkflowBinding.h"
#include "platform/comfy/WorkflowDocument.h"

namespace platform::comfy { class ComfyClient; }
namespace app::bridge     { class AppController; }

namespace app::bridge {

// ─────────────────────────────────────────────────────────────────────────────
// AiGenerationController
//
// AppController（レイヤー操作）と ComfyClient（HTTP 実行）を繋ぐ橋渡し。
// UI / ワークフローエディタ / core 仕様追加は一切行わない。
//
// ── フロー ─────────────────────────────────────────────────────────────────
//
//   Request req;
//   req.workflowPath   = "C:/workflows/inpaint.json";
//   req.useActiveLayer = true;           // → LoadImage にアップロード
//   req.useSelectionAsMask = true;       // → LoadImageMask にアップロード
//   req.inputImageNodeId  = "4";         // 空 = 最初の LoadImage
//   req.maskImageNodeId   = "5";         // 空 = 最初の LoadImageMask
//   req.extraBindings = {
//       WorkflowBinding::kSampler("7").seed(42).steps(20),
//       WorkflowBinding::clipText("2", "a cat"),
//   };
//   req.outputLayerName = "AI 生成";
//   req.timeoutMs = 60000;
//   controller.execute(req);
//
// ── シグナル ────────────────────────────────────────────────────────────────
//   started()              — 処理開始
//   progressUpdate(step, total) — ComfyUI の進捗
//   finished()             — 新規レイヤーとして貼り付け完了
//   errorOccurred(message) — 失敗理由
// ─────────────────────────────────────────────────────────────────────────────
class AiGenerationController : public QObject {
  Q_OBJECT

public:
  explicit AiGenerationController(AppController*          appController,
                                   platform::comfy::ComfyClient* comfyClient,
                                   QObject* parent = nullptr);

  struct Request {
    // ── workflow ─────────────────────────────────────────────────────────────
    /// workflow.json のファイルパス。workflowDoc が有効なら無視される。
    QString workflowPath;

    /// 事前にロード・バインド済みの WorkflowDocument（省略可）。
    platform::comfy::WorkflowDocument workflowDoc;

    // ── 入力画像 ─────────────────────────────────────────────────────────────
    /// true = アクティブレイヤーを PNG にして LoadImage ノードへアップロード。
    /// false の場合は inputImageNodeId への LoadImage バインドを自動で行わない。
    bool useActiveLayer {true};

    /// true = 合成バッファ（全レイヤー結合）をアクティブレイヤーの代わりに使う。
    bool useCompositedBuffer {false};

    /// LoadImage バインド先ノード ID（空 = doc 内の最初の LoadImage ノード）。
    QString inputImageNodeId;

    // ── 選択マスク ────────────────────────────────────────────────────────────
    /// true = 現在の SelectionMask をグレースケール PNG にして LoadImageMask へアップロード。
    bool useSelectionAsMask {false};

    /// LoadImageMask バインド先ノード ID（空 = 最初の LoadImageMask ノード）。
    QString maskImageNodeId;

    // ── 追加バインド ──────────────────────────────────────────────────────────
    /// ユーザーが追加したい WorkflowBinding をリストで渡す（prompt/seed など）。
    QList<platform::comfy::WorkflowBinding> extraBindings;

    // ── 出力 ─────────────────────────────────────────────────────────────────
    /// 結果を貼り付ける新規レイヤー名。
    QString outputLayerName {"AI 生成"};

    /// ComfyUI 完了待ちのタイムアウト [ms]
    int timeoutMs {90000};
  };

  /// リクエストを非同期実行する。
  /// 実行中に再度呼ぶと errorOccurred を emit してスキップする。
  void execute(const Request& req);

  /// batchCount 枚を連続生成する。batchCount==1 は execute() と同等。
  /// 完了時: batchCount==1 → finished(), batchCount>1 → batchCandidatesReady()
  void executeBatch(const Request& req, int batchCount);

  bool isBusy()       const noexcept { return m_busy; }
  int  instanceId()   const noexcept { return m_instanceId; }

signals:
  void started();
  void progressUpdate(int step, int totalSteps);
  void finished();
  void errorOccurred(QString message);
  void batchCandidatesReady(QList<QPixmap> candidates);

private:
  // ── 内部ステップ ──────────────────────────────────────────────────────────
  // ステップ 1: アクティブレイヤー / 合成バッファを PNG 化
  static QByteArray layerToPng(AppController* ac, const Request& req);

  // ステップ 2: SelectionMask をグレースケール PNG 化
  static QByteArray selectionToPng(AppController* ac);

  // workflow をロードしてバインドを適用する共通ヘルパー
  bool loadWorkflowDoc(const Request& req, platform::comfy::WorkflowDocument& out);

  // batch 2枚目以降の実行開始（m_busy チェックをスキップする）
  void runNextBatchIteration(const Request& req);

  // ステップ 3-N: コールバックチェーン
  void doUploadInputImage (platform::comfy::WorkflowDocument doc, const Request& req);
  void doUploadMask       (platform::comfy::WorkflowDocument doc, const Request& req,
                           const QString& savedImageName);
  void doQueue            (platform::comfy::WorkflowDocument doc, const Request& req);
  void doWait             (const QString& promptId,             const Request& req);
  void doFetch            (const QStringList& filenames,        const Request& req);
  void doApplyResult      (const QByteArray& pngBytes,          const Request& req);

  void fail(const QString& message);

  AppController*                 m_app        {nullptr};
  platform::comfy::ComfyClient*  m_comfy      {nullptr};
  bool                           m_busy       {false};
  int                            m_instanceId {0};

  static int s_nextInstanceId;

  // ── Batch 状態 ────────────────────────────────────────────────────────────
  int            m_batchTotal     {1};
  int            m_batchRemaining {0};
  QList<QPixmap> m_batchResults;
  Request        m_batchBaseReq;
};

} // namespace app::bridge
