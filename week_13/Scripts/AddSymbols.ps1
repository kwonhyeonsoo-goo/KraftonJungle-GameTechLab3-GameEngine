param(
    [string]$Configuration = "Debug",
    [string]$Platform = "x64",
    [string]$BuildName = "",
    [string]$SymbolServer = "\\172.21.10.28\symbols",
    [string]$BuildDir = "",
    [string]$ProjectRoot = "",
    [string]$DebuggerToolsDir = "",
    [switch]$EnableSourceServer
)

$ErrorActionPreference = "Stop"

function Write-Step([string]$Message) {
    Write-Host "[Info] $Message"
}

function Write-Skip([string]$Message) {
    Write-Warning $Message
}

function Resolve-FullPath([string]$Path) {
    $CleanPath = $Path.Trim().Trim('"').Trim("'")
    try {
        return [System.IO.Path]::GetFullPath($CleanPath)
    } catch {
        Write-Warning ("Invalid path passed to Resolve-FullPath: <{0}>" -f $CleanPath)
        throw
    }
}

function Find-Tool([string]$ToolName) {
    if ($DebuggerToolsDir) {
        $Candidate = Join-Path $DebuggerToolsDir $ToolName
        if (Test-Path $Candidate) { return $Candidate }
    }

    $FromPath = Get-Command $ToolName -ErrorAction SilentlyContinue
    if ($FromPath) { return $FromPath.Source }

    $WindowsKits = "${env:ProgramFiles(x86)}\Windows Kits\10\Debuggers"
    foreach ($Arch in @("x64", "x86")) {
        $Candidate = Join-Path (Join-Path $WindowsKits $Arch) $ToolName
        if (Test-Path $Candidate) { return $Candidate }

        $SrcSrvCandidate = Join-Path (Join-Path (Join-Path $WindowsKits $Arch) "srcsrv") $ToolName
        if (Test-Path $SrcSrvCandidate) { return $SrcSrvCandidate }
    }

    return $null
}

function Convert-ToRepoPath([string]$Path, [string]$Root) {
    $FullPath = Resolve-FullPath $Path
    $FullRoot = Resolve-FullPath $Root
    if (-not $FullRoot.EndsWith("\")) { $FullRoot += "\" }

    if ($FullPath.StartsWith($FullRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $FullPath.Substring($FullRoot.Length).Replace("\", "/")
    }

    return $null
}

function Get-SourceFilesFromPdb([string]$PdbPath, [string]$SrcTool, [string]$Root) {
    $Files = @()

    if ($SrcTool) {
        & $SrcTool $PdbPath 2>$null | ForEach-Object {
            $Line = $_.Trim()
            if ($Line -and (Test-Path $Line)) {
                $RepoPath = Convert-ToRepoPath $Line $Root
                if ($RepoPath) {
                    $Files += [pscustomobject]@{
                        LocalPath = (Resolve-FullPath $Line)
                        RepoPath = $RepoPath
                    }
                }
            }
        }
    }

    if ($Files.Count -eq 0) {
        git -C $Root ls-files | ForEach-Object {
            $RepoPath = $_.Trim()
            if ($RepoPath -match '\.(c|cc|cpp|cxx|h|hh|hpp|hxx|inl)$') {
                $LocalPath = Join-Path $Root ($RepoPath -replace '/', '\')
                if (Test-Path $LocalPath) {
                    $Files += [pscustomobject]@{
                        LocalPath = (Resolve-FullPath $LocalPath)
                        RepoPath = $RepoPath
                    }
                }
            }
        }
    }

    return $Files | Sort-Object LocalPath -Unique
}

function Convert-ToCmdEscaped([string]$Value) {
    return $Value.Replace('"', '\"')
}

function Copy-FileFromCommit(
    [string]$Root,
    [string]$Commit,
    [string]$RepoPath,
    [string]$TargetPath
) {
    $TargetDir = Split-Path -Parent $TargetPath
    New-Item -ItemType Directory -Force -Path $TargetDir | Out-Null

    $EscapedRoot = Convert-ToCmdEscaped $Root
    $EscapedTarget = Convert-ToCmdEscaped $TargetPath
    $GitObject = "{0}:{1}" -f $Commit, $RepoPath
    $EscapedGitObject = Convert-ToCmdEscaped $GitObject
    $Command = 'git -C "{0}" show "{1}" > "{2}"' -f $EscapedRoot, $EscapedGitObject, $EscapedTarget

    & cmd.exe /d /s /c $Command
    if ($LASTEXITCODE -ne 0) {
        throw "git show failed for $GitObject"
    }
}

function Copy-SourceSnapshot([array]$SourceFiles, [string]$SnapshotRoot, [string]$Root, [string]$Commit) {
    foreach ($File in $SourceFiles) {
        $TargetPath = Join-Path $SnapshotRoot ($File.RepoPath -replace '/', '\')
        Copy-FileFromCommit $Root $Commit $File.RepoPath $TargetPath
    }
}

function Write-SourceServerFile(
    [string]$OutputPath,
    [array]$SourceFiles,
    [string]$SnapshotRoot
) {
    $Lines = @(
        "SRCSRV: ini ------------------------------------------------",
        "VERSION=2",
        "INDEXVERSION=2",
        "VERCTRL=Snapshot",
        "SRCSRV: variables ------------------------------------------",
        "SRCSRVTRG=%targ%\%var4%",
        'SRCSRVCMD=cmd /c copy /y "%var3%" "%SRCSRVTRG%"',
        "SRCSRV: source files ---------------------------------------"
    )

    foreach ($File in $SourceFiles) {
        $SnapshotPath = Join-Path $SnapshotRoot ($File.RepoPath -replace '/', '\')
        $CacheName = $File.RepoPath -replace '[\\/:\s]', '_'
        $Lines += "$($File.LocalPath)*$($File.RepoPath)*$SnapshotPath*$CacheName"
    }

    $Lines += "SRCSRV: end ------------------------------------------------"
    Set-Content -Path $OutputPath -Value $Lines -Encoding ASCII
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

    if (-not (Test-Path $OutDir)) {
        Write-Skip "Build output directory not found: $OutDir"
        exit 0
    }

    $SymbolServerReachable = $false
    try {
        $SymbolServerReachable = Test-Path -LiteralPath $SymbolServer
    } catch [System.UnauthorizedAccessException] {
        Write-Skip "Symbol server access denied: $SymbolServer"
        exit 0
    }

    if (-not $SymbolServerReachable) {
        Write-Skip "Symbol server is not reachable: $SymbolServer"
        exit 0
    }

    $PdbFiles = @(Get-ChildItem -Path $OutDir -Filter "*.pdb" -File)
    if ($PdbFiles.Count -eq 0) {
        Write-Skip "No PDB files found in: $OutDir"
        exit 0
    }

    $Commit = (git -C $RepoRoot rev-parse HEAD 2>$null).Trim()
    if (-not $Commit) {
        $Commit = "uncommitted"
    }

    if (-not $BuildName) {
        $ShortCommit = if ($Commit.Length -gt 12) { $Commit.Substring(0, 12) } else { $Commit }
        $BuildName = "$Configuration-$ShortCommit"
    }

    $SymStore = Find-Tool "symstore.exe"
    $PdbStr = Find-Tool "pdbstr.exe"
    $SrcTool = Find-Tool "srctool.exe"

    Write-Step "Configuration: $Configuration"
    Write-Step "Platform: $Platform"
    Write-Step "BuildName: $BuildName"
    Write-Step "Commit: $Commit"
    Write-Step "OutDir: $OutDir"
    Write-Step "SymbolServer: $SymbolServer"

    $SourceIndexed = $false

    if ($EnableSourceServer) {
        if (-not $PdbStr) {
            Write-Skip "pdbstr.exe not found. Source server data will not be embedded."
        } else {
            $SourceFiles = @(Get-SourceFilesFromPdb $PdbFiles[0].FullName $SrcTool $RepoRoot)
            if ($SourceFiles.Count -eq 0) {
                Write-Skip "No source files found. Source server data will not be embedded."
            } else {
                $DirtyFiles = @(git -C $RepoRoot status --short 2>$null)
                if ($DirtyFiles.Count -gt 0) {
                    Write-Skip "Working tree has uncommitted changes. Source snapshot will use committed files from $Commit."
                }

                $SnapshotRoot = Join-Path $SymbolServer ("SourceRepos\Snapshots\{0}" -f $Commit)
                Write-Step "Creating source snapshot from commit $Commit`: $SnapshotRoot"
                Copy-SourceSnapshot $SourceFiles $SnapshotRoot $RepoRoot $Commit

                foreach ($Pdb in $PdbFiles) {
                    $SrcSrvFile = Join-Path $env:TEMP ("{0}.{1}.srcsrv" -f $Pdb.BaseName, $Commit.Substring(0, [Math]::Min(12, $Commit.Length)))
                    Write-SourceServerFile $SrcSrvFile $SourceFiles $SnapshotRoot

                    Write-Step "Embedding source server data: $($Pdb.FullName)"
                    & $PdbStr -w -p:$($Pdb.FullName) -i:$SrcSrvFile -s:srcsrv
                    if ($LASTEXITCODE -ne 0) {
                        Write-Skip "pdbstr.exe failed for $($Pdb.FullName). Continuing without failing the build."
                    } else {
                        $SourceIndexed = $true
                    }
                }
            }
        }
    }

    if (-not $SymStore) {
        $FallbackDir = Join-Path $SymbolServer ("_fallback\{0}\{1}_{2}" -f $env:COMPUTERNAME, $Configuration, $Platform)
        Write-Skip "symstore.exe not found. Copying PDB files to fallback folder: $FallbackDir"
        New-Item -ItemType Directory -Force -Path $FallbackDir | Out-Null
        Copy-Item -Path (Join-Path $OutDir "*.pdb") -Destination $FallbackDir -Force
        exit 0
    }

    foreach ($Pdb in $PdbFiles) {
        Write-Step "Registering PDB to symbol server: $($Pdb.FullName)"
        & $SymStore add /f $Pdb.FullName /s $SymbolServer /t "KraftonEngine" /v $BuildName /c "Commit $Commit"
        if ($LASTEXITCODE -ne 0) {
            Write-Skip "symstore.exe failed for $($Pdb.FullName). Continuing without failing the build."
        }
    }

    if ($EnableSourceServer -and $SourceIndexed) {
        Write-Step "Source snapshot and source server data completed."
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
