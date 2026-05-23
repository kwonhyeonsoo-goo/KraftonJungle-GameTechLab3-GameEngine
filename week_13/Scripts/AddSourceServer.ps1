param(
    [string]$PdbPath = "",
    [string]$BuildDir = "",
    [string]$RepoRoot = (Join-Path $PSScriptRoot ".."),
    [string]$SourceRepo = "\\172.21.10.28\symbols\SourceRepos\Week12.git",
    [string]$Commit = "",
    [string]$DebuggerToolsDir = "",
    [string]$SrcToolPath = "",
    [string]$PdbStrPath = "",
    [switch]$KeepStreamFiles
)

$ErrorActionPreference = "Stop"

function Resolve-DebuggingTool {
    param(
        [string]$ToolName,
        [string]$ExplicitPath,
        [string]$DebuggerToolsDir
    )

    if ($ExplicitPath) {
        if (Test-Path -LiteralPath $ExplicitPath) {
            return (Resolve-Path -LiteralPath $ExplicitPath).Path
        }

        throw "$ToolName not found at '$ExplicitPath'."
    }

    if ($DebuggerToolsDir) {
        $Candidate = Join-Path $DebuggerToolsDir $ToolName
        if (Test-Path -LiteralPath $Candidate) {
            return (Resolve-Path -LiteralPath $Candidate).Path
        }
    }

    $FromPath = Get-Command $ToolName -ErrorAction SilentlyContinue
    if ($FromPath) {
        return $FromPath.Source
    }

    $WindowsKits = "${env:ProgramFiles(x86)}\Windows Kits\10\Debuggers"
    foreach ($Arch in @("x64", "x86")) {
        $Candidate = Join-Path (Join-Path $WindowsKits $Arch) $ToolName
        if (Test-Path -LiteralPath $Candidate) { return $Candidate }

        $SrcSrvCandidate = Join-Path (Join-Path (Join-Path $WindowsKits $Arch) "srcsrv") $ToolName
        if (Test-Path -LiteralPath $SrcSrvCandidate) { return $SrcSrvCandidate }
    }

    throw "$ToolName not found. Install Windows SDK Debugging Tools or pass the explicit tool path."
}

function Convert-ToGitPath {
    param(
        [string]$Path,
        [string]$Root
    )

    $FullPath = [System.IO.Path]::GetFullPath($Path)
    $FullRoot = [System.IO.Path]::GetFullPath($Root)

    if (-not $FullRoot.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $FullRoot += [System.IO.Path]::DirectorySeparatorChar
    }

    if (-not $FullPath.StartsWith($FullRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $null
    }

    return $FullPath.Substring($FullRoot.Length).Replace("\", "/")
}

function Write-SourceServerStream {
    param(
        [string]$StreamPath,
        [string]$PdbFile,
        [string[]]$SourceFiles,
        [string]$Root,
        [string]$GitRepo,
        [string]$GitCommit
    )

    $Lines = New-Object System.Collections.Generic.List[string]
    $Lines.Add("SRCSRV: ini ------------------------------------------------")
    $Lines.Add("VERSION=2")
    $Lines.Add("INDEXVERSION=2")
    $Lines.Add("VERCTRL=Git")
    $Lines.Add("DATETIME=$(Get-Date -Format o)")
    $Lines.Add("SRCSRV: variables ------------------------------------------")
    $Lines.Add("GIT_EXE=git.exe")
    $Lines.Add("GIT_REPO=$GitRepo")
    $Lines.Add("SRCSRVTRG=%targ%\%fnfile%(%var2%)")
    $Lines.Add('SRCSRVCMD=cmd /c "%GIT_EXE%" --git-dir="%GIT_REPO%" show %var3%:%var2% > "%SRCSRVTRG%"')
    $Lines.Add("SRCSRV: source files ---------------------------------------")

    $MappedCount = 0
    foreach ($SourceFile in $SourceFiles) {
        $RelativePath = Convert-ToGitPath -Path $SourceFile -Root $Root
        if (-not $RelativePath) {
            continue
        }

        $Lines.Add("$SourceFile*$RelativePath*$GitCommit")
        $MappedCount++
    }

    $Lines.Add("SRCSRV: end ------------------------------------------------")
    [System.IO.File]::WriteAllLines($StreamPath, $Lines, [System.Text.Encoding]::ASCII)

    if ($MappedCount -eq 0) {
        throw "No source files in '$PdbFile' were under repo root '$Root'."
    }

    return $MappedCount
}

function Test-SourceServerStream {
    param(
        [string]$PdbStr,
        [string]$PdbFile
    )

    $ReadBackPath = Join-Path ([System.IO.Path]::GetTempPath()) ("srcsrv_read_{0}.txt" -f ([System.Guid]::NewGuid().ToString("N")))
    try {
        & $PdbStr -r "-p:$PdbFile" "-i:$ReadBackPath" -s:srcsrv
        if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $ReadBackPath)) {
            return $false
        }

        $Content = Get-Content -LiteralPath $ReadBackPath -Raw
        return ($Content -match "SRCSRV: source files" -and $Content -match "SRCSRVCMD=")
    } finally {
        Remove-Item -LiteralPath $ReadBackPath -Force -ErrorAction SilentlyContinue
    }
}

$ResolvedRepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path

try {
    $SourceRepoExists = Test-Path -LiteralPath $SourceRepo
} catch {
    throw "Cannot access source repo '$SourceRepo'. Check network share credentials and read permission. Detail: $($_.Exception.Message)"
}

if (-not $SourceRepoExists) {
    throw "Source repo not found: $SourceRepo"
}

if (-not $Commit) {
    $Commit = (& git -C $ResolvedRepoRoot rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or -not $Commit) {
        throw "Failed to resolve git commit from '$ResolvedRepoRoot'. Pass -Commit explicitly."
    }
}

if ($PdbPath) {
    $PdbFiles = @((Resolve-Path -LiteralPath $PdbPath).Path)
} elseif ($BuildDir) {
    $ResolvedBuildDir = (Resolve-Path -LiteralPath $BuildDir).Path
    $PdbFiles = @(Get-ChildItem -LiteralPath $ResolvedBuildDir -Recurse -Filter "*.pdb" -File | ForEach-Object { $_.FullName })
} else {
    throw "Pass either -PdbPath or -BuildDir."
}

if (-not $PdbFiles) {
    throw "No PDB files found."
}

$SrcTool = Resolve-DebuggingTool -ToolName "srctool.exe" -ExplicitPath $SrcToolPath -DebuggerToolsDir $DebuggerToolsDir
$PdbStr = Resolve-DebuggingTool -ToolName "pdbstr.exe" -ExplicitPath $PdbStrPath -DebuggerToolsDir $DebuggerToolsDir

Write-Host "SrcTool   : $SrcTool"
Write-Host "PdbStr    : $PdbStr"
Write-Host "RepoRoot  : $ResolvedRepoRoot"
Write-Host "SourceRepo: $SourceRepo"
Write-Host "Commit    : $Commit"
Write-Host "PDB Count : $($PdbFiles.Count)"

foreach ($PdbFile in $PdbFiles) {
    Write-Host "Processing: $PdbFile"

    $SourceFiles = @(& $SrcTool $PdbFile | Where-Object { $_ -match "^[A-Za-z]:\\|^\\\\" } | Sort-Object -Unique)
    if ($LASTEXITCODE -ne 0) {
        throw "srctool.exe failed for '$PdbFile' with exit code $LASTEXITCODE."
    }

    $StreamPath = Join-Path ([System.IO.Path]::GetTempPath()) ("srcsrv_{0}.txt" -f ([System.Guid]::NewGuid().ToString("N")))
    $MappedCount = Write-SourceServerStream -StreamPath $StreamPath -PdbFile $PdbFile -SourceFiles $SourceFiles -Root $ResolvedRepoRoot -GitRepo $SourceRepo -GitCommit $Commit

    & $PdbStr -w "-p:$PdbFile" "-i:$StreamPath" -s:srcsrv
    if ($LASTEXITCODE -ne 0) {
        throw "pdbstr.exe failed for '$PdbFile' with exit code $LASTEXITCODE."
    }

    if (-not (Test-SourceServerStream -PdbStr $PdbStr -PdbFile $PdbFile)) {
        throw "Source server stream verification failed for '$PdbFile'."
    }

    Write-Host "Embedded source server stream. Source Count: $MappedCount"

    if (-not $KeepStreamFiles) {
        Remove-Item -LiteralPath $StreamPath -Force
    } else {
        Write-Host "Stream file kept: $StreamPath"
    }
}

Write-Host "Source server data embedded successfully."
