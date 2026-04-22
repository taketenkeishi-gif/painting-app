# System Status Board

Legend: `DONE` / `WIP` / `PARTIAL` / `TODO` / `BLOCKED`

## Brush Engine System
- `DONE`: size/opacity/hardness/flow/spacing basic parameters
- `DONE`: stabilization/post correction/velocity correction controls
- `PARTIAL`: angle/roundness/taper state + UI exposure
- `TODO`: pressure curve mapping, texture mixing, advanced smear/accumulation

## Eraser System
- `DONE`: stroke erase with brush-like properties
- `PARTIAL`: vector erase mode split
- `TODO`: touched/intersection/whole-line vector eraser modes

## Fill System
- `DONE`: contiguous fill (raster) and undo/redo
- `PARTIAL`: selection-limited behavior
- `TODO`: gap close, refer-layer modes, tolerance expansion/shrink

## Selection System
- `DONE`: rect selection, clear/invert, undo/redo
- `PARTIAL`: select all / deselect menu integration
- `TODO`: lasso/polygon/quick mask/feather/grow/shrink

## Transform / Object Control System
- `PARTIAL`: layer move tool
- `TODO`: rotate/scale/pivot/transform handles

## Layer / Folder / Mask / Clip System
- `DONE`: raster/vector/folder layers, visibility, duplicate, merge down, rasterize vector
- `DONE`: active-layer clipping toggle + mask create/toggle/remove (minimum)
- `DONE`: drag/drop reorder with undo/redo path
- `TODO`: folder hierarchy, clipping groups, editable masks, lock modes, multi-layer selection

## Vector Editing System
- `DONE`: vector line storage and compositing
- `PARTIAL`: snap-angle presets
- `TODO`: point edit, split/connect, simplify edit tools

## Assist / Snap / Ruler / Guide System
- `PARTIAL`: line snap angle + optional grid/overlay view
- `TODO`: guides/rulers/angle snap UI and snapping presets

## File / Import / Export System
- `DONE`: open/save/save as/export png/export flattened/recent files (+ clear recent)
- `DONE`: clipboard image copy/paste via raster layer
- `DONE`: new-from-clipboard and import-image-as-layer flow
- `TODO`: PSD/PSB/export options/import as layer variants

## Tablet / Input Device System
- `PARTIAL`: mouse-first smoothing hooks
- `TODO`: pressure/tilt/eraser tip/native tablet APIs

## Workspace / Shortcut / Command System
- `DONE`: dock toggle + reset workspace
- `DONE`: workspace layout save/load/delete + restore last
- `DONE`: shortcut settings dialog with runtime reassignment + persistence
- `TODO`: command palette, workspace preset import/export

## UI Theme / Token / Icon System
- `WIP`: dark theme hierarchy and state colors
- `WIP`: panel/button/list spacing consistency improvements
- `PARTIAL`: color system commands (FG/BG swap, reset B/W, transparent color)
- `TODO`: icon pack normalization and token extraction to one source file
