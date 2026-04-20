#include "app/panels/ToolPropertyPanel.h"

#include <cstdint>

#include <QColorDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

#include "app/bridge/AppController.h"

namespace app::panels {

namespace {

core::Color toCoreColor(const QColor& color) {
  return core::Color {
      static_cast<std::uint8_t>(color.red()),
      static_cast<std::uint8_t>(color.green()),
      static_cast<std::uint8_t>(color.blue()),
      static_cast<std::uint8_t>(color.alpha())};
}

QColor toQColor(const core::Color& color) {
  return QColor(color.r, color.g, color.b, color.a);
}

} // namespace

ToolPropertyPanel::ToolPropertyPanel(QWidget* parent)
    : QWidget(parent),
      m_toolNameLabel(new QLabel("Tool: -", this)),
      m_guideLabel(new QLabel("", this)),
      m_colorLabel(new QLabel("Color", this)),
      m_sizeLabel(new QLabel("Size", this)),
      m_opacityLabel(new QLabel("Opacity", this)),
      m_hardnessLabel(new QLabel("Hardness", this)),
      m_colorButton(new QPushButton("Color", this)),
      m_sizeSpin(new QSpinBox(this)),
      m_opacitySlider(new QSlider(Qt::Horizontal, this)),
      m_opacitySpin(new QSpinBox(this)),
      m_hardnessSlider(new QSlider(Qt::Horizontal, this)),
      m_hardnessSpin(new QSpinBox(this)) {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(6, 6, 6, 6);
  layout->setSpacing(6);

  m_guideLabel->setWordWrap(true);
  m_sizeSpin->setRange(1, 128);
  m_opacitySlider->setRange(0, 100);
  m_opacitySpin->setRange(0, 100);
  m_hardnessSlider->setRange(0, 100);
  m_hardnessSpin->setRange(0, 100);

  auto* opacityRow = new QHBoxLayout();
  opacityRow->setContentsMargins(0, 0, 0, 0);
  opacityRow->setSpacing(6);
  opacityRow->addWidget(m_opacitySlider, 1);
  opacityRow->addWidget(m_opacitySpin);

  auto* hardnessRow = new QHBoxLayout();
  hardnessRow->setContentsMargins(0, 0, 0, 0);
  hardnessRow->setSpacing(6);
  hardnessRow->addWidget(m_hardnessSlider, 1);
  hardnessRow->addWidget(m_hardnessSpin);

  layout->addWidget(m_toolNameLabel);
  layout->addWidget(m_guideLabel);
  layout->addSpacing(4);
  layout->addWidget(m_colorLabel);
  layout->addWidget(m_colorButton);
  layout->addWidget(m_sizeLabel);
  layout->addWidget(m_sizeSpin);
  layout->addWidget(m_opacityLabel);
  layout->addLayout(opacityRow);
  layout->addWidget(m_hardnessLabel);
  layout->addLayout(hardnessRow);
  layout->addStretch(1);

  connect(m_colorButton, &QPushButton::clicked, this, &ToolPropertyPanel::onChooseColor);
  connect(m_sizeSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onSizeChanged);
  connect(m_opacitySlider, &QSlider::valueChanged, this, &ToolPropertyPanel::onOpacitySliderChanged);
  connect(m_opacitySpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onOpacitySpinChanged);
  connect(m_hardnessSlider, &QSlider::valueChanged, this, &ToolPropertyPanel::onHardnessSliderChanged);
  connect(m_hardnessSpin, qOverload<int>(&QSpinBox::valueChanged), this, &ToolPropertyPanel::onHardnessSpinChanged);
}

void ToolPropertyPanel::setController(app::bridge::AppController* controller) {
  if (m_controller != nullptr) {
    disconnect(m_controller, nullptr, this, nullptr);
  }

  m_controller = controller;
  if (m_controller == nullptr) {
    return;
  }

  connect(m_controller, &app::bridge::AppController::toolStateChanged, this, &ToolPropertyPanel::refreshFromController);
  refreshFromController();
}

void ToolPropertyPanel::refreshFromController() {
  if (m_controller == nullptr) {
    return;
  }

  m_toolNameLabel->setText(QString("Tool: %1").arg(QString::fromStdString(m_controller->currentToolDisplayName())));
  m_guideLabel->setText(QString::fromStdString(m_controller->currentToolGuide()));

  const bool supportsColor = m_controller->currentToolSupportsColor();
  const bool supportsSize = m_controller->currentToolSupportsSize();
  const bool supportsOpacity = m_controller->currentToolSupportsOpacity();
  const bool supportsHardness = m_controller->currentToolSupportsHardness();
  m_colorLabel->setVisible(supportsColor);
  m_colorButton->setVisible(supportsColor);
  m_sizeLabel->setVisible(supportsSize);
  m_sizeSpin->setVisible(supportsSize);
  m_opacityLabel->setVisible(supportsOpacity);
  m_opacitySlider->setVisible(supportsOpacity);
  m_opacitySpin->setVisible(supportsOpacity);
  m_hardnessLabel->setVisible(supportsHardness);
  m_hardnessSlider->setVisible(supportsHardness);
  m_hardnessSpin->setVisible(supportsHardness);

  const app::bridge::ToolStateViewModel state = m_controller->toolState();
  const QSignalBlocker blocker1(m_sizeSpin);
  const QSignalBlocker blocker2(m_opacitySlider);
  const QSignalBlocker blocker3(m_opacitySpin);
  const QSignalBlocker blocker4(m_hardnessSlider);
  const QSignalBlocker blocker5(m_hardnessSpin);
  m_sizeSpin->setValue(state.size);
  m_opacitySlider->setValue(state.opacity);
  m_opacitySpin->setValue(state.opacity);
  m_hardnessSlider->setValue(state.hardness);
  m_hardnessSpin->setValue(state.hardness);
  updateColorButton();
}

void ToolPropertyPanel::onChooseColor() {
  if (m_controller == nullptr || !m_controller->currentToolSupportsColor()) {
    return;
  }
  const app::bridge::ToolStateViewModel state = m_controller->toolState();
  const QColor picked = QColorDialog::getColor(toQColor(state.color), this, "Tool Color", QColorDialog::ShowAlphaChannel);
  if (!picked.isValid()) {
    return;
  }
  m_controller->setBrushColor(toCoreColor(picked));
}

void ToolPropertyPanel::onSizeChanged(int size) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsSize()) {
    return;
  }
  m_controller->setBrushSize(size);
}

void ToolPropertyPanel::onOpacitySliderChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsOpacity()) {
    return;
  }
  const QSignalBlocker blocker(m_opacitySpin);
  m_opacitySpin->setValue(value);
  m_controller->setBrushOpacity(value);
}

void ToolPropertyPanel::onOpacitySpinChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsOpacity()) {
    return;
  }
  const QSignalBlocker blocker(m_opacitySlider);
  m_opacitySlider->setValue(value);
  m_controller->setBrushOpacity(value);
}

void ToolPropertyPanel::onHardnessSliderChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsHardness()) {
    return;
  }
  const QSignalBlocker blocker(m_hardnessSpin);
  m_hardnessSpin->setValue(value);
  m_controller->setBrushHardness(value);
}

void ToolPropertyPanel::onHardnessSpinChanged(int value) {
  if (m_controller == nullptr || !m_controller->currentToolSupportsHardness()) {
    return;
  }
  const QSignalBlocker blocker(m_hardnessSlider);
  m_hardnessSlider->setValue(value);
  m_controller->setBrushHardness(value);
}

void ToolPropertyPanel::updateColorButton() {
  if (m_controller == nullptr) {
    return;
  }
  const app::bridge::ToolStateViewModel state = m_controller->toolState();
  const QColor color = toQColor(state.color);
  const int luminance = (299 * color.red() + 587 * color.green() + 114 * color.blue()) / 1000;
  const QString textColor = luminance > 128 ? "#111111" : "#f5f5f5";
  const QString hex = color.name(QColor::HexRgb).toUpper();

  m_colorButton->setText(QString("Color %1").arg(hex));
  m_colorButton->setStyleSheet(
      QString("QPushButton { background-color: rgba(%1, %2, %3, %4); color: %5; border: 1px solid #555; padding: 2px 4px; }")
          .arg(color.red())
          .arg(color.green())
          .arg(color.blue())
          .arg(color.alpha())
          .arg(textColor));
}

} // namespace app::panels
