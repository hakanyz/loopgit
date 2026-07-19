$logFile = "C:\Users\hakan\.gemini\antigravity-ide\brain\dbde4162-04d0-4478-b4e8-1568b9031e92\.system_generated\logs\transcript_full.jsonl"
$lines = Get-Content $logFile
$diffLines = @()
foreach ($l in $lines) {
    if ($l -match "@@ -214,259 \+214,6 @@") {
        $startIdx = $l.IndexOf("@@ -214,259 +214,6 @@")
        $endIdx = $l.IndexOf("[diff_block_end]", $startIdx)
        if ($endIdx -gt 0) {
            $block = $l.Substring($startIdx, $endIdx - $startIdx)
            $diffLines = $block -split "\\n"
            break
        }
    }
}
$deletedBlock = @()
foreach ($line in $diffLines) {
    if ($line.StartsWith("-")) {
        # replace \" with "
        $unsecaped = $line.Substring(1).Replace("\\", "`"").Replace("\"", """)
        $deletedBlock += $unsecaped
    }
}
$deletedBlock | Set-Content "deleted_lines.txt"
"Done. Lines: " + $deletedBlock.Count
