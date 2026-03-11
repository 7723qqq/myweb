# Git 自动备份脚本 - 每 24 小时运行一次
param(
    [string]$BackupDir = "C:\Users\34730\.openclaw-autoclaw\workspace\backups"
)

$Workspace = "C:\Users\34730\.openclaw-autoclaw\workspace"
$Date = Get-Date -Format "yyyyMMdd_HHmmss"
$LogFile = Join-Path $Workspace "backup_${Date}.log"

Set-Location $Workspace

Write-Output "=== Git 自动备份 [$(Get-Date)] ===" | Tee-Object $LogFile

# 检查状态
$status = git status --short 2>&1 | Out-String
if ($status) {
    Write-Output "变更文件:`n$status" >> $LogFile
    
    # 自动暂存所有更改
    git add . -q >> $LogFile
    
    # 尝试提交
    try {
        $commitMsg = "Auto-backup: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
        git commit -m $commitMsg -q >> $LogFile 2>&1
        Write-Output "✅ 已自动提交 - $commitMsg" | Tee-Object $LogFile -Append
    } catch {
        Write-Output "⚠️ 无新文件变更或暂存区为空" | Tee-Object $LogFile -Append
    }
} else {
    Write-Output "✅ 仓库干净，无需提交" | Tee-Object $LogFile -Append
}

Write-Output "备份完成。查看日志：backup_${Date}.log"
