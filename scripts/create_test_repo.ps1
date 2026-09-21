# Script to generate a realistic test repository with 200+ commits, branches, merges, tags, and stashes

$repoPath = "c:\Users\hakan\OneDrive\Desktop\hakan_workspace\hakan\git_program\untitled\test_git"

if (Test-Path $repoPath) {
    Write-Host "Removing existing test_git folder..."
    Remove-Item -Recurse -Force $repoPath
}

New-Item -ItemType Directory -Path $repoPath | Out-Null
Set-Location $repoPath

git init -b main
git config user.name "LoopGit Tester"
git config user.email "tester@loopgit.local"
git config core.autocrlf false

# Helper function to commit with realistic advancing timestamps
$global:commitCount = 0
$baseDate = (Get-Date "2026-01-01 10:00:00")

function Next-Date() {
    $global:commitCount++
    $d = $baseDate.AddMinutes($global:commitCount * 15).ToString("yyyy-MM-dd HH:mm:ss")
    $env:GIT_AUTHOR_DATE = $d
    $env:GIT_COMMITTER_DATE = $d
}

function Make-Commit($file, $text, $msg) {
    Next-Date
    Add-Content -Path $file -Value $text
    git add $file
    git commit -m $msg --quiet
}

Write-Host "Generating base commits on main..."
New-Item -ItemType File -Path "README.md" -Value "# LoopGit Test Repository`n`nTest repository for graph and feature validation.`n" | Out-Null
Next-Date
git add README.md
git commit -m "chore: initial repository setup" --quiet

# Phase 1: 30 commits on main
for ($i = 1; $i -le 30; $i++) {
    Make-Commit "core.cpp" "// Core update $i`nvoid core_$i() {}" "feat(core): implement module step $i"
}
git tag -a "v0.1.0" -m "Release version 0.1.0"

# Phase 2: Fork feature/auth
git checkout -b feature/auth --quiet
for ($i = 1; $i -le 15; $i++) {
    Make-Commit "auth.cpp" "// Auth routine $i`nbool check_token_$i() { return true; }" "feat(auth): token verification routine $i"
}

# Phase 3: Progress on main
git checkout main --quiet
for ($i = 1; $i -le 15; $i++) {
    Make-Commit "network.cpp" "// Network endpoint $i`nvoid endpoint_$i() {}" "feat(net): establish network handler $i"
}

# Phase 4: Fork feature/ui-redesign from main
git checkout -b feature/ui-redesign --quiet
for ($i = 1; $i -le 25; $i++) {
    Make-Commit "ui_theme.css" "/* Theme component $i */ .btn-$i { padding: 4px; }" "style(ui): update theme component $i"
}

# Phase 5: Merge feature/auth into main
git checkout main --quiet
Next-Date
git merge --no-ff feature/auth -m "merge: integrate feature/auth module into main" --quiet

# Phase 6: More commits on main
for ($i = 1; $i -le 20; $i++) {
    Make-Commit "database.cpp" "// Database query $i`nvoid query_$i() {}" "feat(db): optimize query handler $i"
}
git tag -a "v0.5.0" -m "Release version 0.5.0"

# Phase 7: Hotfix branch
git checkout -b hotfix/memory-leak --quiet
for ($i = 1; $i -le 5; $i++) {
    Make-Commit "core.cpp" "// Memory cleanup $i" "fix(memory): prevent pointer leak in step $i"
}
git checkout main --quiet
Next-Date
git merge --no-ff hotfix/memory-leak -m "merge: apply hotfix/memory-leak patches" --quiet

# Phase 8: Fork feature/analytics
git checkout -b feature/analytics --quiet
for ($i = 1; $i -le 25; $i++) {
    Make-Commit "analytics.cpp" "// Metric $i`nvoid log_metric_$i() {}" "feat(metrics): add analytics telemetry $i"
}

# Phase 9: Commits on main and merge feature/ui-redesign
git checkout main --quiet
for ($i = 1; $i -le 15; $i++) {
    Make-Commit "utils.cpp" "// Helper $i" "refactor(utils): sanitize string helper $i"
}
Next-Date
git merge --no-ff feature/ui-redesign -m "merge: adopt new UI theme across dashboard" --quiet

# Phase 10: Commits on main & merge feature/analytics
for ($i = 1; $i -le 20; $i++) {
    Make-Commit "service.cpp" "// Microservice $i" "feat(service): background worker $i"
}
Next-Date
git merge --no-ff feature/analytics -m "merge: incorporate telemetry & telemetry pipeline" --quiet
git tag -a "v1.0.0" -m "Production Release 1.0.0"

# Phase 11: Active ongoing parallel branch feature/experimental
git checkout -b feature/experimental --quiet
for ($i = 1; $i -le 20; $i++) {
    Make-Commit "experimental.cpp" "// Experiment $i" "feat(experiment): test prototype module $i"
}

# Phase 12: Remaining commits on main to reach ~200+
git checkout main --quiet
for ($i = 1; $i -le 35; $i++) {
    Make-Commit "main.cpp" "// App lifecycle $i" "feat(app): enhance application lifecycle $i"
}

# Phase 13: Create stashes
# Stash 1 (older stash@{1}): staged change
Add-Content -Path "config.json" -Value "{ `"debug`": true, `"version`": 2 }"
git add config.json
git stash push -m "WIP: Staged debug settings for local test" --quiet

# Stash 0 (newest stash@{0}): working directory modifications
Add-Content -Path "README.md" -Value "`n## Urgent Notes`n- Do not deploy to prod without signing."
git stash push -m "WIP on main: unfinished readme documentation" --quiet

Write-Host "Total commits generated: $commitCount"
Write-Host "Branches created: main, feature/auth, feature/ui-redesign, hotfix/memory-leak, feature/analytics, feature/experimental"
Write-Host "Stashes created:"
git stash list
Write-Host "Test repository ready at: $repoPath"
