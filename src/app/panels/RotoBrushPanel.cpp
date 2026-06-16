#include "app/panels/RotoBrushPanel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

#include "app/bridge/AppController.h"

namespace app::panels {

RotoBrushPanel::RotoBrushPanel(app::bridge::AppController* controller,
                               QWidget* parent)
    : QWidget(parent)
{
  // ── ブラシ種別 ──────────────────────────────────────────────────────────
  m_fgBtn    = new QPushButton(QString::fromUtf8(u8"前景 (FG)"), this);
  m_bgBtn    = new QPushButton(QString::fromUtf8(u8"背景 (BG)"), this);
  m_clearBtn = new QPushButton(QString::fromUtf8(u8"クリア"), this);

  m_fgBtn->setCheckable(true);
  m_bgBtn->setCheckable(true);
  m_fgBtn->setChecked(true);

  // ── ブラシ半径 ──────────────────────────────────────────────────────────
  m_radiusSlider = new QSlider(Qt::Horizontal, this);
  m_radiusSlider->setRange(2, 60);
  m_radiusSlider->setValue(8);
  m_radiusLabel  = new QLabel("8 px", this);
  m_radiusLabel->setMinimumWidth(36);

  // ── レイアウト ──────────────────────────────────────────────────────────
  auto* modeRow = new QHBoxLayout;
  modeRow->addWidget(m_fgBtn);
  modeRow->addWidget(m_bgBtn);
  modeRow->addStretch(1);
  modeRow->addWidget(m_clearBtn);

  auto* radiusRow = new QHBoxLayout;
  radiusRow->addWidget(new QLabel(QString::fromUtf8(u8"半径:"), this));
  radiusRow->addWidget(m_radiusSlider, 1);
  radiusRow->addWidget(m_radiusLabel);

  auto* vbox = new QVBoxLayout(this);
  vbox->setContentsMargins(6, 6, 6, 6);
  vbox->setSpacing(6);
  vbox->addLayout(modeRow);
  vbox->addLayout(radiusRow);
  vbox->addStretch(1);

  // ── 接続 ────────────────────────────────────────────────────────────────
  connect(m_fgBtn,    &QPushButton::clicked, this, &RotoBrushPanel::onFgClicked);
  connect(m_bgBtn,    &QPushButton::clicked, this, &RotoBrushPanel::onBgClicked);
  connect(m_clearBtn, &QPushButton::clicked, this, &RotoBrushPanel::onClearClicked);
  connect(m_radiusSlider, &QSlider::valueChanged,
          this, &RotoBrushPanel::onRadiusChanged);

  setController(controller);
}

void RotoBrushPanel::setController(app::bridge::AppController* controller) {
  m_controller = controller;
  updateButtonStates();
}

void RotoBrushPanel::onFgClicked() {
  m_isForeground = true;
  updateButtonStates();
  if (m_controller)
    m_controller->setRotoBrushForeground(true);
}

void RotoBrushPanel::onBgClicked() {
  m_isForeground = false;
  updateButtonStates();
  if (m_controller)
    m_controller->setRotoBrushForeground(false);
}

void RotoBrushPanel::onClearClicked() {
  if (m_controller)
    m_controller->clearRotoStrokes();
}

void RotoBrushPanel::onRadiusChanged(int value) {
  m_radiusLabel->setText(QString::number(value) + " px");
  if (m_controller)
    m_controller->setRotoBrushRadius(static_cast<float>(value));
}

void RotoBrushPanel::updateButtonStates() {
  m_fgBtn->setChecked( m_isForeground);
  m_bgBtn->setChecked(!m_isForeground);
}

} // namespace app::panels
