#include "app/bridge/AiService.h"

#include <QUrl>

#include "app/bridge/AiGenerationController.h"
#include "platform/comfy/ComfyClient.h"

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

} // namespace app::bridge
