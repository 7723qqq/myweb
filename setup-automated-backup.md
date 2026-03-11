## Git 自动备份任务设置

### 📋 已创建文件
- ✅ `backup-git-workflow.ps1` - 自动备份脚本
- ✅ `.git-hooks.txt` - 仓库最佳实践文档

### ⚙️ 配置自动计划任务

**手动方式 (执行一次):**
```powershell
# 打开任务计划程序并创建新任务
taskkill /F /IM powershell.exe  2>&1 || true

schtasks /create /tn "GitAutoBackup" /tr 'powershell.exe -ExecutionPolicy Bypass -File "C:\Users\34730\.openclaw-autoclaw\workspace\backup-git-workflow.ps1"' /sc daily /st 22:00 /rl highest
```

**计划:**
- **频率**: 每日 22:00 (你可以在 `schtasks` 中修改)
- **日志**: `workspace/backup_YYYYMMDD_HHmmss.log`

### 🔄 立即测试
执行以下命令手动触发一次备份:
```powershell
powershell.exe -ExecutionPolicy Bypass -File "C:\Users\34730\.openclaw-autoclaw\workspace\backup-git-workflow.ps1"
```

需要我现在帮你创建这个计划任务吗？
