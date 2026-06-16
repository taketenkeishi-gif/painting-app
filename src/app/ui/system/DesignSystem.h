#pragma once

// Single-include entry point for the Design System.
// Panels and widgets should include this file instead of individual headers.
//
// Usage:
//   #include "app/ui/system/DesignSystem.h"
//   namespace DS = app::ui::system;
//   int h = DS::row::kNormal;

#include "UiMetrics.h"
#include "UiTheme.h"
#include "UiMotion.h"
#include "UiResponsive.h"
