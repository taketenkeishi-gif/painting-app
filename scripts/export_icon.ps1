param()
$ink = "C:\Program Files\Inkscape\bin\inkscape.exe"
$svg = "$PSScriptRoot\..\src\app\resources\app_icon.svg"
$resDir = "$PSScriptRoot\..\src\app\resources\icon_sizes"

New-Item -ItemType Directory -Force -Path $resDir | Out-Null

$sizes = @(16, 24, 32, 48, 64, 128, 256)
foreach ($s in $sizes) {
    $out = "$resDir\icon_$s.png"
    Start-Process -FilePath $ink `
        -ArgumentList "--export-filename=`"$out`"","--export-width=$s","--export-height=$s","`"$svg`"" `
        -Wait -NoNewWindow
    Write-Host "Exported ${s}x${s}"
}

# Combine into ICO using Python Pillow (only for format packing, not drawing)
$py = @"
from PIL import Image
import os

sizes = [16, 24, 32, 48, 64, 128, 256]
res_dir = r"$($resDir -replace '\\','/')"
images = []
for s in sizes:
    p = os.path.join(res_dir, f'icon_{s}.png')
    img = Image.open(p).convert('RGBA')
    images.append(img)

out_ico = r"$($PSScriptRoot -replace '\\','/')/../src/app/resources/app_icon.ico"
out_ico = os.path.normpath(out_ico)
images[0].save(out_ico, format='ICO',
               sizes=[(s,s) for s in sizes],
               append_images=images[1:])
print(f'ICO saved: {out_ico}')
"@

$py | python
