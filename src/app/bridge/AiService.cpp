#include "app/bridge/AiService.h"

#include <QBuffer>
#include <QUrl>

#include "app/bridge/AiGenerationController.h"
#include "app/bridge/WorkflowFactory.h"
#include "platform/comfy/ComfyClient.h"
#include "platform/comfy/WorkflowDocument.h"

namespace app::bridge {

AiService::AiService(AppController* appController, QObject* parent)
    : QObject(parent), m_appController(appController) {}

void AiService::setComfyUrl(const QString& url) {
  m_url = url;
  if (m_comfyClient)
    m_comfyClient->setBaseUrl(QUrl(url));
}

bool AiService::isBusy() const {
  return m_genCtrl && m_genCtrl->isBusy();
}

int AiService::controllerInstanceId() const {
  return m_genCtrl ? m_genCtrl->instanceId() : -1;
}

void AiService::ensureInitialized() {
  if (!m_comfyClient) {
    m_comfyClient = new platform::comfy::ComfyClient(this);
    m_comfyClient->setBaseUrl(QUrl(m_url));
  }
  if (!m_genCtrl) {
    m_genCtrl = new AiGenerationController(m_appController, m_comfyClient, this);
    connect(m_genCtrl, &AiGenerationController::started,
            this, &AiService::generationStarted);
    connect(m_genCtrl, &AiGenerationController::progressUpdate,
            this, &AiService::generationProgressUpdate);
    connect(m_genCtrl, &AiGenerationController::finished, this, [this]() {
      emit generationFinished(m_currentOpType);
    });
    connect(m_genCtrl, &AiGenerationController::errorOccurred,
            this, &AiService::generationError);
    connect(m_genCtrl, &AiGenerationController::batchCandidatesReady,
            this, &AiService::batchCandidatesReady);
  }
}

static AiGenerationController::Request toGenCtrlRequest(const AiService::GenerateRequest& req) {
  AiGenerationController::Request r;
  r.workflowPath        = req.workflowPath;
  r.workflowDoc         = req.workflowDoc;
  r.useActiveLayer      = req.useActiveLayer;
  r.useCompositedBuffer = req.useCompositedBuffer;
  r.inputImageNodeId    = req.inputImageNodeId;
  r.useSelectionAsMask  = req.useSelectionAsMask;
  r.maskImageNodeId     = req.maskImageNodeId;
  r.extraBindings       = req.extraBindings;
  r.outputLayerName     = req.outputLayerName;
  r.timeoutMs           = req.timeoutMs;
  return r;
}

void AiService::generate(const GenerateRequest& req, int batchCount) {
  ensureInitialized();
  m_currentOpType = QStringLiteral("generate");
  m_genCtrl->executeBatch(toGenCtrlRequest(req), batchCount);
}

void AiService::inpaint(const GenerateRequest& req, int batchCount) {
  ensureInitialized();
  m_currentOpType = QStringLiteral("inpaint");
  m_genCtrl->executeBatch(toGenCtrlRequest(req), batchCount);
}

// ─────────────────────────────────────────────────────────────────────────────
// selectMask (SAM)
// ─────────────────────────────────────────────────────────────────────────────
void AiService::selectMask(const SelectMaskRequest& req) {
  ensureInitialized();
  auto* client = m_comfyClient;

  const int     pointX        = req.pointX;
  const int     pointY        = req.pointY;
  const bool    positivePoint = req.positivePoint;
  const QString samModel      = req.samModel;
  const int     timeoutMs     = req.timeoutMs;

  client->uploadImage(req.imagePng, "sam_input.png",
      [this, client, pointX, pointY, positivePoint, samModel, timeoutMs](
          QString uploadedName, QString err) {
    if (!err.isEmpty()) { emit selectMaskError(err); return; }

    const auto doc = platform::comfy::WorkflowDocument::fromJson(
        WorkflowFactory::buildSamWorkflow(
            {uploadedName, pointX, pointY, positivePoint, samModel}));

    client->queueWorkflow(doc, [this, client, timeoutMs](QString promptId, QString qErr) {
      if (!qErr.isEmpty()) { emit selectMaskError(qErr); return; }
      client->waitForOutputs(promptId,
          [this, client](QStringList outputs, QString wErr) {
        if (!wErr.isEmpty()) { emit selectMaskError(wErr); return; }
        if (outputs.isEmpty()) { emit selectMaskError("No SAM output"); return; }
        client->fetchImage(outputs.first(), QString(), "output",
            [this](QByteArray png, QString fErr) {
          if (!fErr.isEmpty()) { emit selectMaskError(fErr); return; }
          emit selectMaskResult(png);
        });
      }, 500, timeoutMs);
    });
  });
}

// ─────────────────────────────────────────────────────────────────────────────
// upscale
// ─────────────────────────────────────────────────────────────────────────────
void AiService::upscale(const UpscaleRequest& req) {
  ensureInitialized();
  auto* client = m_comfyClient;

  const QString modelName = req.modelName;
  const int     timeoutMs = req.timeoutMs;

  client->uploadImage(req.imagePng, "lpa_upscale_in.png",
      [this, client, modelName, timeoutMs](QString uploadedName, QString err) {
    if (!err.isEmpty()) { emit upscaleError(err); return; }

    const auto doc = platform::comfy::WorkflowDocument::fromJson(
        WorkflowFactory::buildUpscaleWorkflow(uploadedName, modelName));

    client->queueWorkflow(doc, [this, client, timeoutMs](QString promptId, QString qErr) {
      if (!qErr.isEmpty()) { emit upscaleError(qErr); return; }
      client->waitForOutputs(promptId,
          [this, client](QStringList outputs, QString wErr) {
        if (!wErr.isEmpty()) { emit upscaleError(wErr); return; }
        if (outputs.isEmpty()) { emit upscaleError("No upscale output"); return; }
        client->fetchImage(outputs.first(), QString(), "output",
            [this](QByteArray png, QString fErr) {
          if (!fErr.isEmpty()) { emit upscaleError(fErr); return; }
          emit upscaleResult(png);
        });
      }, 500, timeoutMs);
    });
  });
}

// ─────────────────────────────────────────────────────────────────────────────
// fetchUpscaleModels
// ─────────────────────────────────────────────────────────────────────────────
void AiService::fetchUpscaleModels() {
  ensureInitialized();
  m_comfyClient->fetchUpscaleModels(
      [this](QStringList models, QString /*err*/) {
        emit upscaleModelsReady(models);
      });
}

void AiService::cancel() {
  if (m_comfyClient)
    m_comfyClient->interrupt();
}

} // namespace app::bridge
