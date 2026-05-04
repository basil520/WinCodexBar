$ts = "D:\WinCodexBar\translations\WinCodexBar_zh_CN.ts"
$content = [System.IO.File]::ReadAllText($ts, [System.Text.Encoding]::UTF8)

$fixes = @(
    # CodexAccountsPane
    @("Accounts", "账户"),
    @("Adding...", "添加中..."),
    @("Add Account", "添加账户"),
    @("Waiting for Codex authorization...", "等待 Codex 授权..."),
    @("Open", "打开"),
    @("Cancel", "取消"),
    @("Select the active Codex account for usage tracking.", "选择用于用量追踪的活动 Codex 账户。"),
    @("System", "系统"),
    @("Active", "活动中"),
    @("No email", "无邮箱"),
    @("Use", "使用"),
    @("Set as active", "设为活动中"),
    @("Auth", "认证"),
    @("Re-authenticating...", "重新认证中..."),
    @("Re-authenticate", "重新认证"),
    @("Promote", "提升"),
    @("Promote to system account", "提升为系统账户"),
    @("Remove", "移除"),
    @("Removing...", "移除中..."),
    @("Remove account", "移除账户"),
    @("No accounts configured. Click 'Add Account' to add a Codex account.", "未配置账户。点击'添加账户'以添加 Codex 账户。"),
    # ProviderDetailView
    @("Account Management", "账户管理"),
    @("Manage multiple Codex accounts. Switch between accounts to track usage separately.", "管理多个 Codex 账户。切换账户以分别追踪用量。"),
    @("Usage Projection", "用量预测"),
    @("Projected rate lanes and credits from the current active account.", "来自当前活动账户的预测速率通道和积分。"),
    @("Buy More Credits", "购买更多积分"),
    @("Supplemental Metrics", "补充指标"),
    # DebugPane
    @("Test Codex Connection", "测试 Codex 连接"),
    @("Run a single fetch attempt and record diagnostics.", "执行单次获取尝试并记录诊断信息。"),
    @("Last session source", "最近会话来源"),
    @("Recent Fetch Attempts", "最近获取尝试"),
    @("Test", "测试"),
    # TokenUsagePane
    @("Token", "令牌"),
    @("Credit", "积分"),
    # TrayPanel
    @("Credit Events", "积分事件"),
    @("Usage by Service", "按服务用量"),
    @("Purchase credits", "购买积分"),
    # Generic
    @("Codex Verbose Logging", "Codex 详细日志"),
    @("Web Dashboard Debug Dump", "Web Dashboard 调试转储"),
    @("Codex Diagnostics", "Codex 诊断")
)

foreach ($fix in $fixes) {
    $src = $fix[0]
    $tran = $fix[1]
    $escSrc = [regex]::Escape($src)
    $escTran = [regex]::Escape($tran)
    $pattern = "(<source>" + $escSrc + "</source>\s*<translation type=`"unfinished`"></translation>)"
    $replacement = "<source>" + $src + "</source>`n        <translation>" + $tran + "</translation>"
    $newContent = $content -replace $pattern, $replacement
    if ($newContent -ne $content) {
        Write-Output "OK: $src -> $tran"
        $content = $newContent
    }
}

[System.IO.File]::WriteAllText($ts, $content, [System.Text.Encoding]::UTF8)
Write-Output "Done. Checking remaining unfinished..."
$count = ([regex]::Matches($content, 'type="unfinished"')).Count
Write-Output "Unfinished remaining: $count"
