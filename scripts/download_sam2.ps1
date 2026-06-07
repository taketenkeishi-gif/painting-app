# SAM2 ONNX モデルセットアップスクリプト
# 使い方: powershell -File scripts\download_sam2.ps1
#
# このスクリプトは以下を行います:
#   1. Python 仮想環境を作成して依存パッケージをインストール
#   2. SAM2-Tiny の重みをダウンロード
#   3. ONNX モデルにエクスポート (encoder + decoder)
#   4. build\Release\models\ に配置
#
# 前提条件:
#   - Python 3.10+ がインストール済み
#   - PAINT_USE_ONNX=ON でビルドするには vcpkg が必要:
#       vcpkg install onnxruntime:x64-windows
#       cmake -DPAINT_USE_ONNX=ON -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ...

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$ScriptDir  = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot   = Split-Path -Parent $ScriptDir
$ModelsDir  = Join-Path $RepoRoot "build\Release\models"
$VenvDir    = Join-Path $RepoRoot ".venv_sam2"
$ExportPy   = Join-Path $RepoRoot "scripts\_sam2_export_impl.py"

Write-Host "==== SAM2 ONNX モデルセットアップ ====" -ForegroundColor Cyan
Write-Host ""

# ─── 出力ディレクトリ ────────────────────────────────────────────────────
New-Item -ItemType Directory -Force -Path $ModelsDir | Out-Null
Write-Host "[1/4] 出力先: $ModelsDir"

# ─── Python 仮想環境 ─────────────────────────────────────────────────────
if (-not (Test-Path "$VenvDir\Scripts\python.exe")) {
    Write-Host "[2/4] Python 仮想環境を作成中..."
    python -m venv $VenvDir
} else {
    Write-Host "[2/4] 既存の仮想環境を使用: $VenvDir"
}
$Py = "$VenvDir\Scripts\python.exe"
& $Py -m pip install --quiet --upgrade pip

# ─── 依存パッケージ ──────────────────────────────────────────────────────
Write-Host "[3/4] パッケージをインストール中 (初回は数分かかります)..."
& $Py -m pip install --quiet `
    torch torchvision --extra-index-url https://download.pytorch.org/whl/cpu
& $Py -m pip install --quiet `
    "git+https://github.com/facebookresearch/sam2.git" `
    onnx onnxruntime

# ─── エクスポートスクリプトを生成 ────────────────────────────────────────
$modelsEsc = $ModelsDir.Replace('\', '\\')
@"
#!/usr/bin/env python3
"""SAM2-Tiny -> ONNX encoder + decoder エクスポート"""
import sys, urllib.request, torch
from pathlib import Path

OUT  = Path(r"$modelsEsc")
CKPT = OUT / "sam2_hiera_tiny.pt"

CKPT_URL = "https://dl.fbaipublicfiles.com/segment_anything_2/072824/sam2_hiera_tiny.pt"
if not CKPT.exists():
    print(f"Downloading weights: {CKPT_URL}")
    urllib.request.urlretrieve(CKPT_URL, CKPT)
print(f"Checkpoint: {CKPT}")

import sam2
from sam2.build_sam import build_sam2

cfg_base = Path(sam2.__file__).parent / "configs" / "sam2"
cfg      = cfg_base / "sam2_hiera_t.yaml"
if not cfg.exists():
    # newer sam2 layout
    cfg = cfg_base / "sam2.1" / "sam2.1_hiera_t.yaml"
if not cfg.exists():
    print(f"ERROR: SAM2 config not found in {cfg_base}", file=sys.stderr)
    sys.exit(1)

model = build_sam2(str(cfg), str(CKPT), device="cpu")
model.eval()

# ================================================================
# Encoder  image [1,3,1024,1024] -> image_embed, high_res_feats_0, high_res_feats_1
# ================================================================
class Encoder(torch.nn.Module):
    def __init__(self, m):
        super().__init__()
        self.enc = m.image_encoder
        self.prep = m._prepare_backbone_features

    @torch.no_grad()
    def forward(self, x):
        backbone_out = self.enc(x)
        # _prepare_backbone_features returns (feature_maps, pos_embeds, feat_sizes)
        # feature_maps[-3] = low-res (64x64), [-2] = mid (128x128), [-1] = hi (256x256)
        feats, _, _ = self.prep(backbone_out)
        # feats is a list [N,256,64,64], [N,64,128,128], [N,32,256,256]
        return feats[0], feats[1], feats[2]

enc = Encoder(model)
enc.eval()

# 動作確認
dummy = torch.zeros(1, 3, 1024, 1024)
with torch.no_grad():
    ie, hr0, hr1 = enc(dummy)
print(f"  encoder out shapes: {ie.shape}, {hr0.shape}, {hr1.shape}")

enc_path = OUT / "sam2_encoder.onnx"
torch.onnx.export(
    enc, dummy, str(enc_path),
    input_names=["image"],
    output_names=["image_embed", "high_res_feats_0", "high_res_feats_1"],
    opset_version=17, do_constant_folding=True,
)
print(f"  Encoder -> {enc_path}")

# ================================================================
# Decoder  image_embed + high_res_feats + points -> masks [1,4,256,256], iou [1,4]
# ================================================================
class Decoder(torch.nn.Module):
    def __init__(self, m):
        super().__init__()
        self.pe_enc  = m.sam_prompt_encoder
        self.dec     = m.sam_mask_decoder

    @torch.no_grad()
    def forward(self, image_embed, high_res_feats_0, high_res_feats_1,
                point_coords, point_labels, mask_input, has_mask_input):
        # point_labels are float in our engine; cast to long for SAM2
        sp, dp = self.pe_enc(
            points=(point_coords, point_labels.squeeze(0).long()),
            boxes=None, masks=None,
        )
        low_res_masks, iou_pred, _, _ = self.dec(
            image_embeddings=image_embed,
            image_pe=self.pe_enc.get_dense_pe(),
            sparse_prompt_embeddings=sp,
            dense_prompt_embeddings=dp,
            multimask_output=True,
            repeat_image=False,
            high_res_features=[high_res_feats_0, high_res_feats_1],
        )
        masks = torch.nn.functional.interpolate(
            low_res_masks, size=(256, 256), mode="bilinear", align_corners=False)
        return masks, iou_pred

dec = Decoder(model)
dec.eval()

N = 2
dc = torch.zeros(1, N, 2)
dl = torch.zeros(1, N)
mi = torch.zeros(1, 1, 256, 256)
hm = torch.zeros(1)

with torch.no_grad():
    m_out, iou_out = dec(ie, hr0, hr1, dc, dl, mi, hm)
print(f"  decoder out shapes: masks={m_out.shape}, iou={iou_out.shape}")

dec_path = OUT / "sam2_decoder.onnx"
torch.onnx.export(
    dec, (ie, hr0, hr1, dc, dl, mi, hm), str(dec_path),
    input_names=["image_embed","high_res_feats_0","high_res_feats_1",
                 "point_coords","point_labels","mask_input","has_mask_input"],
    output_names=["masks","iou_predictions"],
    dynamic_axes={
        "point_coords": {1: "num_points"},
        "point_labels": {1: "num_points"},
    },
    opset_version=17, do_constant_folding=True,
)
print(f"  Decoder -> {dec_path}")
print("Export complete!")
"@ | Set-Content $ExportPy -Encoding UTF8

# ─── 実行 ────────────────────────────────────────────────────────────────
Write-Host "[4/4] ONNX エクスポート中..."
& $Py $ExportPy
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "エクスポートに失敗しました。" -ForegroundColor Red
    Write-Host "代替手段: 以下のコマンドで手動エクスポートを試してください:" -ForegroundColor Yellow
    Write-Host "  $Py $ExportPy"
    exit 1
}

Write-Host ""
Write-Host "=== セットアップ完了 ===" -ForegroundColor Green
Write-Host ""
Write-Host "モデル配置先:"
Write-Host "  $ModelsDir\sam2_encoder.onnx"
Write-Host "  $ModelsDir\sam2_decoder.onnx"
Write-Host ""
Write-Host "次のステップ — PAINT_USE_ONNX=ON でビルドする:"
Write-Host ""
Write-Host "  # vcpkg で onnxruntime をインストール (未インストールの場合)"
Write-Host "  vcpkg install onnxruntime:x64-windows"
Write-Host ""
Write-Host "  # CMake configure (vcpkg toolchain を指定)"
Write-Host "  cmake -S . -B build ``"
Write-Host "    -DCMAKE_PREFIX_PATH='C:\Qt\6.7.2\msvc2019_64' ``"
Write-Host "    -DPAINT_USE_ONNX=ON ``"
Write-Host "    -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
Write-Host ""
Write-Host "  cmake --build build --config Release"
Write-Host ""
Write-Host "  .\launch.bat"
Write-Host ""
Write-Host "vcpkg がない場合 (手動 ONNX Runtime):"
Write-Host "  1. https://github.com/microsoft/onnxruntime/releases から"
Write-Host "     onnxruntime-win-x64-*.zip をダウンロードして展開"
Write-Host "  2. cmake ... -DPAINT_USE_ONNX=ON -DONNXRUNTIME_ROOT=<展開先パス>"
