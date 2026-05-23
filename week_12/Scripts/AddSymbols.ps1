param(
    [string]$Configuration = "Debug",
    [string]$Platform = "x64",
    [string]$BuildName = "",
    [string]$SymbolServer = "\\172.21.10.28\symbols",
    [string]$BuildDir = "",
    [string]$ProjectRoot = "",
    [string]$DebuggerToolsDir = "",
    [switch]$EnableSourceServer,
    [string]$SourceRepo = ""
)

$ErrorActionPreference = "Stop"

function Write-Step([string]$Message) {
    Write-Host "[Info] $Message"
}

function Write-Skip([string]$Message) {
    Write-Warning $Message
}

function Resolve-FullPath([string]$Path) {
    return [System.IO.Path]::GetFullPath($Path.Trim().Trim('"').Trim("'"))
}

function Resolve-DebuggingTool([string]$ToolName, [string]$DebuggerToolsDir) {
    if ($DebuggerToolsDir) {
        $Candidate = Join-Path $DebuggerToolsDir $ToolName
        if (Test-Path -LiteralPath $Candidate) { return (Resolve-Path -LiteralPath $Candidate).Path }
    }

    $FromPath = Get-Command $ToolName -ErrorAction SilentlyContinue
    if ($FromPath) { return $FromPath.Source }

    $WindowsKits = "${env:ProgramFiles(x86)}\Windows Kits\10\Debuggers"
    foreach ($Arch in @("x64", "x86")) {
        $Candidate = Join-Path (Join-Path $WindowsKits $Arch) $ToolName
        if (Test-Path -LiteralPath $Candidate) { return $Candidate }

        $SrcSrvCandidate = Join-Path (Join-Path (Join-Path $WindowsKits $Arch) "srcsrv") $ToolName
        if (Test-Path -LiteralPath $SrcSrvCandidate) { return $SrcSrvCandidate }
    }

    return $null
}

function Get-BinaryFiles([string]$Directory) {
    return Get-ChildItem -LiteralPath $Directory -File |
        Where-Object { $_.Extension -in @(".exe", ".dll") }
}

function Copy-FilesToFallback([array]$Files, [string]$FallbackDir) {
    New-Item -ItemType Directory -Force -Path $FallbackDir | Out-Null
    foreach ($File in $Files) {
        Copy-Item -LiteralPath $File.FullName -Destination $FallbackDir -Force
    }
}

function Add-FileToSymbolStore(
    [string]$SymStore,
    [string]$FilePath,
    [string]$SymbolServer,
    [string]$ProductName,
    [string]$BuildName,
    [string]$Commit
) {
    Write-Step "Registering file to symbol server: $FilePath"
    & $SymStore add /f $FilePath /s $SymbolServer /t $ProductName /v $BuildName /c "Commit $Commit"
    if ($LASTEXITCODE -ne 0) {
        Write-Skip "symstore.exe failed for $FilePath. Continuing without failing the build."
    }
}

function Ensure-SourceRepo([string]$RepoRoot, [string]$RepoPath, [string]$Commit) {
    $RepoParent = Split-Path -Parent $RepoPath
    New-Item -ItemType Directory -Force -Path $RepoParent | Out-Null

    if (-not (Test-Path -LiteralPath $RepoPath)) {
        Write-Step "Creating bare source repo: $RepoPath"
        & git clone --bare $RepoRoot $RepoPath
        if ($LASTEXITCODE -ne 0) {
            throw "git clone --bare failed for source repo: $RepoPath"
        }
    }

    Write-Step "Updating bare source repo with commit: $Commit"
    & git -C $RepoRoot push $RepoPath "$Commit`:refs/heads/source-server-latest" --force
    if ($LASTEXITCODE -ne 0) {
        throw "git push failed for source repo: $RepoPath"
    }
}

try {
    $RepoRoot = if ($ProjectRoot) {
        Resolve-FullPath $ProjectRoot
    } else {
        Resolve-FullPath (Join-Path $PSScriptRoot "..")
    }

    $EngineDir = Join-Path $RepoRoot "KraftonEngine"
    $OutDir = if ($BuildDir) {
        Resolve-FullPath $BuildDir
    } else {
        Join-Path $EngineDir "Bin\$Configuration"
    }

    if (-not (Test-Path -LiteralPath $OutDir)) {
        Write-Skip "Build output directory not found: $OutDir"
        exit 0
    }

    try {
        if (-not (Test-Path -LiteralPath $SymbolServer)) {
            Write-Skip "Symbol server is not reachable: $SymbolServer"
            exit 0
        }
    } catch {
        Write-Skip "Cannot access symbol server '$SymbolServer'. Detail: $($_.Exception.Message)"
        exit 0
    }

    $PdbFiles = @(Get-ChildItem -LiteralPath $OutDir -Filter "*.pdb" -File)
    if ($PdbFiles.Count -eq 0) {
        Write-Skip "No PDB files found in: $OutDir"
        exit 0
    }

    $BinaryFiles = @(Get-BinaryFiles $OutDir)
    $Commit = (git -C $RepoRoot rev-parse HEAD 2>$null).Trim()
    if (-not $Commit) {
        Write-Skip "Cannot resolve git commit. Skipping symbol upload."
        exit 0
    }

    if (-not $BuildName) {
        $BuildName = "$Configuration-$($Commit.Substring(0, [Math]::Min(12, $Commit.Length)))"
    }

    if (-not $SourceRepo) {
        $SourceRepo = Join-Path $SymbolServer "SourceRepos\Week12.git"
    }

    Write-Step "Configuration: $Configuration"
    Write-Step "Platform: $Platform"
    Write-Step "BuildName: $BuildName"
    Write-Step "Commit: $Commit"
    Write-Step "OutDir: $OutDir"
    Write-Step "SymbolServer: $SymbolServer"
    Write-Step "SourceRepo: $SourceRepo"

    if ($EnableSourceServer) {
        $AddSourceServer = Join-Path $PSScriptRoot "AddSourceServer.ps1"
        if (-not (Test-Path -LiteralPath $AddSourceServer)) {
            Write-Skip "AddSourceServer.ps1 not found. Source server data will not be embedded."
        } else {
            $DirtyFiles = @(git -C $RepoRoot status --short 2>$null)
            if ($DirtyFiles.Count -gt 0) {
                Write-Skip "Working tree has uncommitted changes. Source server will use committed files from $Commit."
            }

            try {
                Ensure-SourceRepo $RepoRoot $SourceRepo $Commit

                $SourceArgs = @(
                    "-ExecutionPolicy", "Bypass",
                    "-File", $AddSourceServer,
                    "-BuildDir", $OutDir,
                    "-RepoRoot", $RepoRoot,
                    "-SourceRepo", $SourceRepo,
                    "-Commit", $Commit
                )

                if ($DebuggerToolsDir) {
                    $SourceArgs += @("-DebuggerToolsDir", $DebuggerToolsDir)
                }

                Write-Step "Embedding source server data before symbol registration..."
                & powershell @SourceArgs
                if ($LASTEXITCODE -ne 0) {
                    Write-Skip "AddSourceServer.ps1 failed with exit code $LASTEXITCODE."
                }
            } catch {
                Write-Skip "Source server data was not embedded. Detail: $($_.Exception.Message)"
            }
        }
    }

    $SymStore = Resolve-DebuggingTool "symstore.exe" $DebuggerToolsDir
    if (-not $SymStore) {
        $FallbackDir = Join-Path $SymbolServer ("_fallback\{0}\{1}_{2}" -f $env:COMPUTERNAME, $Configuration, $Platform)
        Write-Skip "symstore.exe not found. Copying PDB and binary files to fallback folder: $FallbackDir"
        Copy-FilesToFallback (@($PdbFiles) + @($BinaryFiles)) $FallbackDir
        exit 0
    }

    foreach ($Pdb in $PdbFiles) {
        Add-FileToSymbolStore $SymStore $Pdb.FullName $SymbolServer "KraftonEngine" $BuildName $Commit
    }

    foreach ($Binary in $BinaryFiles) {
        Add-FileToSymbolStore $SymStore $Binary.FullName $SymbolServer "KraftonEngine" $BuildName $Commit
    }

    Write-Step "Symbol upload step finished."
    exit 0
} catch {
    Write-Skip "Symbol upload skipped because of an unexpected error: $($_.Exception.Message)"
    if ($_.InvocationInfo -and $_.InvocationInfo.PositionMessage) {
        Write-Warning $_.InvocationInfo.PositionMessage
    }
    exit 0
}
