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
      emit generationFinished(QStringLiteral("generate"));
    });
    connect(m_genCtrl, &AiGenerationController::errorOccurred,
            this, &AiService::generationError);
    connect(m_genCtrl, &AiGenerationController::batchCandidatesReady,
            this, &AiService::batchCandidatesReady);
  }
}

void AiService::generate(const GenerateRequest& req, int batchCount) {
  ensureInitialized();

  AiGenerationController::Request genReq;
  genReq.workflowPath        = req.workflowPath;
  genReq.workflowDoc         = req.workflowDoc;
  genReq.useActiveLayer      = req.useActiveLayer;
  genReq.useCompositedBuffer = req.useCompositedBuffer;
  genReq.inputImageNodeId    = req.inputImageNodeId;
  genReq.useSelectionAsMask  = req.useSelectionAsMask;
  genReq.maskImageNodeId     = req.maskImageNodeId;
  genReq.extraBindings       = req.extraBindings;
  genReq.outputLayerName     = req.outputLayerName;
  genReq.timeoutMs           = req.timeoutMs;

  m_genCtrl->executeBatch(genReq, batchCount);
}

} // namespace app::bridge
