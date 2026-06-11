#include "app/panels/MeshDeformPanel.h"
#include "app/bridge/AppController.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>

namespace app::panels {

MeshDeformPanel::MeshDeformPanel(app::bridge::AppController* controller,
                                 QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
{
    setWindowTitle("Mesh Deform");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // --- Grid Rows slider ---
    {
        auto* rowLayout = new QHBoxLayout;
        m_rowsLabel = new QLabel(tr("Rows: 16"), this);
        m_rowsLabel->setMinimumWidth(70);
        m_rowsSlider = new QSlider(Qt::Horizontal, this);
        m_rowsSlider->setRange(4, 64);
        m_rowsSlider->setValue(16);
        rowLayout->addWidget(m_rowsLabel);
        rowLayout->addWidget(m_rowsSlider);
        mainLayout->addLayout(rowLayout);
    }

    // --- Grid Cols slider ---
    {
        auto* colLayout = new QHBoxLayout;
        m_colsLabel = new QLabel(tr("Cols: 16"), this);
        m_colsLabel->setMinimumWidth(70);
        m_colsSlider = new QSlider(Qt::Horizontal, this);
        m_colsSlider->setRange(4, 64);
        m_colsSlider->setValue(16);
        colLayout->addWidget(m_colsLabel);
        colLayout->addWidget(m_colsSlider);
        mainLayout->addLayout(colLayout);
    }

    // --- Mode combo ---
    {
        auto* modeLayout = new QHBoxLayout;
        modeLayout->addWidget(new QLabel(tr("Mode:"), this));
        m_modeCombo = new QComboBox(this);
        m_modeCombo->addItem(tr("Similarity (scale+rotate)"));
        m_modeCombo->addItem(tr("Rigid (rotate only)"));
        modeLayout->addWidget(m_modeCombo);
        mainLayout->addLayout(modeLayout);
    }

    // --- Generator combo ---
    {
        auto* genLayout = new QHBoxLayout;
        genLayout->addWidget(new QLabel(tr("Generator:"), this));
        m_generatorCombo = new QComboBox(this);
        m_generatorCombo->addItem(tr("Grid (uniform)"));
        m_generatorCombo->addItem(tr("Edge Adaptive"));
        genLayout->addWidget(m_generatorCombo);
        mainLayout->addLayout(genLayout);
    }

    // --- Wireframe checkbox ---
    m_wireframeCheck = new QCheckBox(tr("Show Wireframe"), this);
    m_wireframeCheck->setChecked(true);
    mainLayout->addWidget(m_wireframeCheck);

    // --- Regenerate button ---
    m_regenerateBtn = new QPushButton(tr("Regenerate Mesh"), this);
    mainLayout->addWidget(m_regenerateBtn);

    // --- Confirm / Cancel buttons ---
    {
        auto* btnLayout = new QHBoxLayout;
        m_confirmBtn = new QPushButton(tr("Confirm"), this);
        m_cancelBtn  = new QPushButton(tr("Cancel"),  this);
        btnLayout->addWidget(m_confirmBtn);
        btnLayout->addWidget(m_cancelBtn);
        mainLayout->addLayout(btnLayout);
    }

    mainLayout->addStretch();

    // --- Connect signals ---

    connect(m_rowsSlider, &QSlider::valueChanged, this, [this](int v) {
        m_rowsLabel->setText(tr("Rows: %1").arg(v));
        if (m_controller) {
            m_controller->meshDeformSetGridDensity(m_rowsSlider->value(),
                                                   m_colsSlider->value());
        }
    });

    connect(m_colsSlider, &QSlider::valueChanged, this, [this](int v) {
        m_colsLabel->setText(tr("Cols: %1").arg(v));
        if (m_controller) {
            m_controller->meshDeformSetGridDensity(m_rowsSlider->value(),
                                                   m_colsSlider->value());
        }
    });

    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MeshDeformPanel::onModeChanged);

    connect(m_generatorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MeshDeformPanel::onGeneratorChanged);

    connect(m_regenerateBtn, &QPushButton::clicked,
            this, &MeshDeformPanel::onRegenerateMesh);

    connect(m_confirmBtn, &QPushButton::clicked,
            this, &MeshDeformPanel::onConfirm);

    connect(m_cancelBtn, &QPushButton::clicked,
            this, &MeshDeformPanel::onCancel);
}

// ---------------------------------------------------------------------------
// Public slots
// ---------------------------------------------------------------------------

void MeshDeformPanel::updateFromController()
{
    if (!m_controller) return;

    const bool active = m_controller->isInMeshDeformMode();
    setEnabled(active);

    if (active) {
        const auto mode = m_controller->meshDeformMode();
        const int modeIdx = (mode == core::mesh::DeformMode::Similarity) ? 0 : 1;
        QSignalBlocker blk(m_modeCombo);
        m_modeCombo->setCurrentIndex(modeIdx);
    }
}

// ---------------------------------------------------------------------------
// Private slots
// ---------------------------------------------------------------------------

void MeshDeformPanel::onConfirm()
{
    if (m_controller) m_controller->commitMeshDeformSession();
    hide();
}

void MeshDeformPanel::onCancel()
{
    if (m_controller) m_controller->cancelMeshDeformSession();
    hide();
}

void MeshDeformPanel::onRegenerateMesh()
{
    if (m_controller) m_controller->meshDeformRegenerateMesh();
}

void MeshDeformPanel::onModeChanged(int index)
{
    if (!m_controller) return;
    const auto mode = (index == 0) ? core::mesh::DeformMode::Similarity
                                   : core::mesh::DeformMode::Rigid;
    m_controller->meshDeformSetMode(mode);
}

void MeshDeformPanel::onGeneratorChanged(int index)
{
    if (m_controller) m_controller->meshDeformSetGeneratorType(index);
}

} // namespace app::panels
