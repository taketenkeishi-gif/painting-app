# sync_claude_md.ps1 — Claude Code Stop フックから呼ばれる
# CLAUDE.md の AUTO-SYNC ブロックを git 情報で更新する
param()
$ErrorActionPreference = "SilentlyContinue"

# フックから呼ばれるときは作業ディレクトリがプロジェクトルートになる
# PSScriptRoot が空の場合に備えて両方試す
$root = if ($PSScriptRoot) {
    [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
} else {
    $PWD.Path
}

$claudeMd = Join-Path $root "CLAUDE.md"
if (-not (Test-Path $claudeMd)) { exit 0 }

# git 情報
$lastCommit = & git -C $root log -1 --format="%h %s (%ad)" --date=short 2>$null
$branch     = & git -C $root rev-parse --abbrev-ref HEAD 2>$null
$dirty      = & git -C $root status --porcelain 2>$null
$dirtyMark  = if ($dirty) { " (*未コミットの変更あり*)" } else { "" }

# system_status.md から WIP/TODO 件数
$wipCount = 0; $todoCount = 0
$statusFile = Join-Path $root "docs\system_status.md"
if (Test-Path $statusFile) {
    $lines = [System.IO.File]::ReadAllLines($statusFile, [System.Text.Encoding]::UTF8)
    $wipCount  = ($lines | Where-Object { $_ -match '`WIP`' }).Count
    $todoCount = ($lines | Where-Object { $_ -match '`TODO`' }).Count
}

$syncBlock = @"
---
<!-- AUTO-SYNC: Stop フックにより更新 -->
## 最終同期情報

- **ブランチ**: $branch$dirtyMark
- **最終コミット**: $lastCommit
- **system_status**: WIP $wipCount 件 / TODO $todoCount 件
<!-- /AUTO-SYNC -->
"@

$enc     = [System.Text.UTF8Encoding]::new($false)
$content = [System.IO.File]::ReadAllText($claudeMd, $enc)

$pattern = "(?s)---\s*<!-- AUTO-SYNC:.*?<!-- /AUTO-SYNC -->"
$updated = if ($content -match $pattern) {
    [System.Text.RegularExpressions.Regex]::Replace($content, $pattern, $syncBlock)
} else {
    $content.TrimEnd() + "`n`n" + $syncBlock
}

[System.IO.File]::WriteAllText($claudeMd, $updated, $enc)
