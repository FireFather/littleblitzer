[CmdletBinding()]
param(
    [string]$LittleBlitzer = (Join-Path (Split-Path -Parent $PSScriptRoot) 'LittleBlitzer.exe')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Assert-True {
    param(
        [Parameter(Mandatory = $true)][bool]$Condition,
        [Parameter(Mandatory = $true)][string]$Message
    )
    if (-not $Condition) {
        throw $Message
    }
}

function Quote-Argument {
    param([Parameter(Mandatory = $true)][string]$Value)
    return '"' + $Value.Replace('"', '\"') + '"'
}

$repository = Split-Path -Parent $PSScriptRoot
$littleBlitzerPath = (Resolve-Path -LiteralPath $LittleBlitzer).Path
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
Assert-True (Test-Path -LiteralPath $vswhere -PathType Leaf) "vswhere.exe was not found: $vswhere"
$visualStudio = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
Assert-True (-not [string]::IsNullOrWhiteSpace($visualStudio)) 'A Visual Studio C++ installation was not found.'
$developerCommand = Join-Path $visualStudio 'Common7\Tools\VsDevCmd.bat'

$testRoot = Join-Path ([System.IO.Path]::GetTempPath()) ('LittleBlitzer-3.01-tests-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testRoot | Out-Null
$succeeded = $false

try {
    $fixtures = @('scripted_uci', 'illegal_uci', 'timeout_uci', 'crash_uci')
    foreach ($fixture in $fixtures) {
        $source = Join-Path $PSScriptRoot "fixtures\$fixture.cpp"
        $output = Join-Path $testRoot "$fixture.exe"
        $object = Join-Path $testRoot "$fixture.obj"
        $compile = '"{0}" -arch=amd64 -host_arch=amd64 >nul && cl.exe /nologo /EHsc /O2 "{1}" /Fo:"{2}" /Fe:"{3}"' -f
            $developerCommand, $source, $object, $output
        & $env:ComSpec /d /s /c $compile
        Assert-True ($LASTEXITCODE -eq 0 -and (Test-Path -LiteralPath $output -PathType Leaf)) "Failed to build $fixture"
    }

    function Invoke-LittleBlitzerCase {
        param(
            [Parameter(Mandatory = $true)][string]$Name,
            [Parameter(Mandatory = $true)][string]$FirstFixture,
            [Parameter(Mandatory = $true)][string]$FirstUciName,
            [Parameter(Mandatory = $true)][string]$Position,
            [Parameter(Mandatory = $true)][int]$TimeControl,
            [Parameter(Mandatory = $true)][int]$Base,
            [Parameter(Mandatory = $true)][int]$ExpectedExit,
            [Parameter(Mandatory = $true)][string]$ExpectedTermination,
            [Parameter(Mandatory = $true)][int]$ExpectedIllegal,
            [Parameter(Mandatory = $true)][int]$ExpectedEngineFailures,
            [UInt64]$OpeningSeed = 292
        )

        $caseDirectory = Join-Path $testRoot $Name
        New-Item -ItemType Directory -Path $caseDirectory | Out-Null
        $engines = Join-Path $caseDirectory 'Engines.lbe'
        $settings = Join-Path $caseDirectory 'Tournament.lbt'
        $results = Join-Path $caseDirectory 'Results.pgn'
        $status = Join-Path $caseDirectory 'status.log'
        $firstPath = Join-Path $testRoot "$FirstFixture.exe"
        $scriptedPath = Join-Path $testRoot 'scripted_uci.exe'

        @"
Engine=$firstPath
LB_Name=$Name first
LB_ExpectedUCI=$FirstUciName
Engine=$scriptedPath
LB_Name=$Name second
LB_ExpectedUCI=LittleBlitzer scripted fixture
"@ | Set-Content -LiteralPath $engines -Encoding ASCII

        @"
Type: 0
TC: $TimeControl
Base: $Base
Inc: 0
Rounds: 2
Ponder: 0
OwnBook: 0
Hash: 1
NumParallel: 1
Variant: 0
Position: $Position
Randomize: 0
OpeningSeed: $OpeningSeed
AdjudicateMateScore: 500
AdjudicateMateMoves: 2
AdjudicateDrawMoves: 20
"@ | Set-Content -LiteralPath $settings -Encoding ASCII

        $arguments = @(
            '--batch',
            '--engines', (Quote-Argument $engines),
            '--settings', (Quote-Argument $settings),
            '--results', (Quote-Argument $results),
            '--status', (Quote-Argument $status)
        ) -join ' '
        $process = Start-Process -FilePath $littleBlitzerPath -ArgumentList $arguments -Wait -PassThru -WindowStyle Hidden
        Assert-True ($process.ExitCode -eq $ExpectedExit) "$Name exit code was $($process.ExitCode), expected $ExpectedExit"

        $resultTags = @(Select-String -LiteralPath $results -Pattern '^\[Result "(1-0|0-1|1/2-1/2)"\]$')
        $terminations = @(Select-String -LiteralPath $results -Pattern ('^\[Termination "' + [regex]::Escape($ExpectedTermination) + '"\]$'))
        Assert-True ($resultTags.Count -eq 2) "$Name produced $($resultTags.Count) games instead of 2"
        Assert-True ($terminations.Count -eq 2) "$Name did not produce two '$ExpectedTermination' terminations"

        $completePattern = " COMPLETE completed=2 total=2 illegal=$ExpectedIllegal engine_failures=$ExpectedEngineFailures exit=$ExpectedExit$"
        Assert-True ($null -ne (Select-String -LiteralPath $status -Pattern $completePattern | Select-Object -Last 1)) `
            "$Name completion status was missing or inconsistent"

        $manifestPath = $results + '.manifest.json'
        Assert-True (Test-Path -LiteralPath $manifestPath -PathType Leaf) "$Name did not create a run manifest"
        $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
        if ($OpeningSeed -eq 0) {
            Assert-True ([string]$manifest.opening_seed -ne '0') "$Name manifest did not record a generated opening seed"
        }
        else {
            Assert-True ([string]$manifest.opening_seed -eq [string]$OpeningSeed) `
                "$Name manifest did not preserve the opening seed"
        }
        Assert-True ($manifest.engines.Count -eq 2) "$Name manifest engine count was not 2"
        if ($Position.StartsWith('FEN:', [System.StringComparison]::OrdinalIgnoreCase)) {
            Assert-True ([string]$manifest.tournament.starting_fen -eq $Position.Substring(4)) `
                "$Name manifest did not preserve its direct starting FEN"
        }
        foreach ($engine in $manifest.engines) {
            $actualHash = (Get-FileHash -LiteralPath $engine.path -Algorithm SHA256).Hash
            Assert-True ($engine.sha256 -eq $actualHash) "$Name manifest engine hash mismatch: $($engine.path)"
        }

        return [pscustomobject]@{
            Name = $Name
            ExitCode = $process.ExitCode
            Games = $resultTags.Count
            Termination = $ExpectedTermination
            Manifest = $manifestPath
        }
    }

    $results = @()
    $results += Invoke-LittleBlitzerCase -Name 'normal' -FirstFixture 'scripted_uci' `
        -FirstUciName 'LittleBlitzer scripted fixture' -Position 'FEN:7k/5K2/6Q1/8/8/8/8/8 w - - 0 1' `
        -TimeControl 1 -Base 1000 -ExpectedExit 0 -ExpectedTermination 'normal' -ExpectedIllegal 0 -ExpectedEngineFailures 0 `
        -OpeningSeed 0
    $results += Invoke-LittleBlitzerCase -Name 'illegal' -FirstFixture 'illegal_uci' `
        -FirstUciName 'LittleBlitzer illegal-move fixture' -Position 'OPENING' `
        -TimeControl 1 -Base 1000 -ExpectedExit 3 -ExpectedTermination 'rules infraction' -ExpectedIllegal 2 -ExpectedEngineFailures 0
    $results += Invoke-LittleBlitzerCase -Name 'timeout' -FirstFixture 'timeout_uci' `
        -FirstUciName 'LittleBlitzer timeout fixture' -Position 'OPENING' `
        -TimeControl 0 -Base 100 -ExpectedExit 0 -ExpectedTermination 'time forfeit' -ExpectedIllegal 0 -ExpectedEngineFailures 0
    $results += Invoke-LittleBlitzerCase -Name 'crash' -FirstFixture 'crash_uci' `
        -FirstUciName 'LittleBlitzer crash fixture' -Position 'OPENING' `
        -TimeControl 1 -Base 1000 -ExpectedExit 3 -ExpectedTermination 'death' -ExpectedIllegal 0 -ExpectedEngineFailures 2
    $blackStart = Invoke-LittleBlitzerCase -Name 'black-start' -FirstFixture 'scripted_uci' `
        -FirstUciName 'LittleBlitzer scripted fixture' -Position 'FEN:7k/8/8/8/8/8/8/K7 b - - 0 1' `
        -TimeControl 1 -Base 1000 -ExpectedExit 0 -ExpectedTermination 'normal' -ExpectedIllegal 0 -ExpectedEngineFailures 0
    $results += $blackStart
    $blackPgn = Get-Content -LiteralPath (Join-Path $testRoot 'black-start\Results.pgn') -Raw
    Assert-True ($blackPgn.Contains('1... Kg8')) 'Black-to-move PGN did not use the standard three-dot move number.'
    Assert-True (-not $blackPgn.Contains(' 0 0"]')) 'PGN contained an invalid zero fullmove number.'

    $identityDirectory = Join-Path $testRoot 'identity-mismatch'
    New-Item -ItemType Directory -Path $identityDirectory | Out-Null
    $identityEngines = Join-Path $identityDirectory 'Engines.lbe'
    $identitySettings = Join-Path $identityDirectory 'Tournament.lbt'
    $identityResults = Join-Path $identityDirectory 'Results.pgn'
    $identityStatus = Join-Path $identityDirectory 'status.log'
    $scriptedPath = Join-Path $testRoot 'scripted_uci.exe'
    @"
Engine=$scriptedPath
LB_Name=mislabeled candidate
LB_ExpectedUCI=An intentionally wrong identity
Engine=$scriptedPath
LB_Name=control
LB_ExpectedUCI=LittleBlitzer scripted fixture
"@ | Set-Content -LiteralPath $identityEngines -Encoding ASCII
    @"
Type: 0
TC: 1
Base: 1000
Inc: 0
Rounds: 2
Hash: 1
NumParallel: 1
Variant: 0
Position: OPENING
Randomize: 0
OpeningSeed: 292
AdjudicateMateScore: 500
AdjudicateMateMoves: 2
AdjudicateDrawMoves: 20
"@ | Set-Content -LiteralPath $identitySettings -Encoding ASCII
    $identityArguments = @(
        '--batch',
        '--engines', (Quote-Argument $identityEngines),
        '--settings', (Quote-Argument $identitySettings),
        '--results', (Quote-Argument $identityResults),
        '--status', (Quote-Argument $identityStatus)
    ) -join ' '
    $identityProcess = Start-Process -FilePath $littleBlitzerPath -ArgumentList $identityArguments -Wait -PassThru -WindowStyle Hidden
    Assert-True ($identityProcess.ExitCode -eq 2) "Identity mismatch exit code was $($identityProcess.ExitCode), expected 2"
    Assert-True ($null -ne (Select-String -LiteralPath $identityStatus -Pattern 'ENGINE_ERROR .*Engine identity mismatch' |
        Select-Object -Last 1)) 'Identity mismatch was not explained in the batch status log.'
    Assert-True (-not (Test-Path -LiteralPath ($identityResults + '.manifest.json'))) `
        'Identity mismatch unexpectedly started a tournament and produced a manifest.'
    $results += [pscustomobject]@{
        Name = 'identity-mismatch'
        ExitCode = $identityProcess.ExitCode
        Games = 0
        Termination = 'preflight rejection'
        Manifest = '(not started)'
    }

    $results | Format-Table -AutoSize
    Write-Output "PASS: LittleBlitzer integration tests completed in $testRoot"
    $succeeded = $true
}
finally {
    if ($succeeded -and (Test-Path -LiteralPath $testRoot -PathType Container)) {
        $resolvedTestRoot = (Resolve-Path -LiteralPath $testRoot).Path
        $resolvedTemp = (Resolve-Path -LiteralPath ([System.IO.Path]::GetTempPath())).Path
        if ($resolvedTestRoot.StartsWith($resolvedTemp, [System.StringComparison]::OrdinalIgnoreCase)) {
            Remove-Item -LiteralPath $resolvedTestRoot -Recurse -Force
        }
    }
    elseif (-not $succeeded) {
        Write-Warning "Test evidence was preserved at $testRoot"
    }
}
