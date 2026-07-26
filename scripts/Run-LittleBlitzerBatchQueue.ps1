[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$LittleBlitzer,

    [Parameter(Mandatory = $true)]
    [string]$Queue,

    [string]$Log,

    [switch]$ValidateOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Resolve-ExistingFile {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [string]$Description
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Description not found: $Path"
    }
    return (Resolve-Path -LiteralPath $Path).Path
}

function Get-RequiredProperty {
    param(
        [Parameter(Mandatory = $true)]
        [object]$Object,

        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    if (-not ($Object.PSObject.Properties.Name -contains $Name)) {
        throw "Queue entry is missing '$Name'."
    }
    $value = [string]$Object.$Name
    if ([string]::IsNullOrWhiteSpace($value)) {
        throw "Queue entry has an empty '$Name'."
    }
    return $value
}

function Convert-ToQuotedArgument {
    param([Parameter(Mandatory = $true)][string]$Value)

    return '"' + $Value.Replace('"', '\"') + '"'
}

function Write-RunLog {
    param([Parameter(Mandatory = $true)][string]$Message)

    $line = '{0:yyyy-MM-ddTHH:mm:ss} {1}' -f (Get-Date), $Message
    Add-Content -LiteralPath $script:LogPath -Value $line -Encoding UTF8
    Write-Output $line
}

function Get-TournamentRounds {
    param([Parameter(Mandatory = $true)][string]$SettingsPath)

    $match = Select-String -LiteralPath $SettingsPath -Pattern '^\s*Rounds\s*:\s*(\d+)\s*$' |
        Select-Object -First 1
    if ($null -eq $match) {
        throw "Rounds was not found in $SettingsPath"
    }
    return [int]$match.Matches[0].Groups[1].Value
}

function Test-TournamentInputs {
    param(
        [Parameter(Mandatory = $true)][string]$EnginesPath,
        [Parameter(Mandatory = $true)][string]$SettingsPath
    )

    $engineLines = Select-String -LiteralPath $EnginesPath -Pattern '^\s*Engine=(.+?)\s*$'
    if ($engineLines.Count -lt 2) {
        throw "At least two Engine= entries are required in $EnginesPath"
    }
    foreach ($line in $engineLines) {
        $enginePath = $line.Matches[0].Groups[1].Value.Trim()
        Resolve-ExistingFile -Path $enginePath -Description 'Engine executable' | Out-Null
    }

    $position = Select-String -LiteralPath $SettingsPath -Pattern '^\s*Position\s*:\s*(EPD|PGN):(.+?)\s*$' |
        Select-Object -First 1
    if ($null -ne $position) {
        $positionPath = $position.Matches[0].Groups[2].Value.Trim()
        if (-not [System.IO.Path]::IsPathRooted($positionPath)) {
            $positionPath = Join-Path (Split-Path -Parent $SettingsPath) $positionPath
        }
        Resolve-ExistingFile -Path $positionPath -Description 'Starting-position file' | Out-Null
    }
}

$LittleBlitzerPath = Resolve-ExistingFile -Path $LittleBlitzer -Description 'LittleBlitzer executable'
$QueuePath = Resolve-ExistingFile -Path $Queue -Description 'Queue file'
if ([string]::IsNullOrWhiteSpace($Log)) {
    $Log = $QueuePath + '.run.log'
}
$script:LogPath = [System.IO.Path]::GetFullPath($Log)
$logDirectory = Split-Path -Parent $script:LogPath
if (-not (Test-Path -LiteralPath $logDirectory -PathType Container)) {
    New-Item -ItemType Directory -Path $logDirectory | Out-Null
}

try {
    $queueDocument = Get-Content -LiteralPath $QueuePath -Raw | ConvertFrom-Json
    if (-not ($queueDocument.PSObject.Properties.Name -contains 'tests')) {
        throw "Queue file must contain a 'tests' array."
    }
    $tests = @($queueDocument.tests)
    if ($tests.Count -eq 0) {
        throw 'Queue contains no tests.'
    }

    $preparedTests = @()
    $resultPaths = @{}
    foreach ($test in $tests) {
        $name = Get-RequiredProperty -Object $test -Name 'name'
        $engines = Resolve-ExistingFile -Path (Get-RequiredProperty -Object $test -Name 'engines') -Description 'Engines file'
        $settings = Resolve-ExistingFile -Path (Get-RequiredProperty -Object $test -Name 'settings') -Description 'Tournament settings'
        $results = [System.IO.Path]::GetFullPath((Get-RequiredProperty -Object $test -Name 'results'))
        $status = if ($test.PSObject.Properties.Name -contains 'status') {
            [System.IO.Path]::GetFullPath([string]$test.status)
        } else {
            $results + '.status.log'
        }
        $overwrite = ($test.PSObject.Properties.Name -contains 'overwrite') -and [bool]$test.overwrite
        $rounds = Get-TournamentRounds -SettingsPath $settings

        Test-TournamentInputs -EnginesPath $engines -SettingsPath $settings
        if ($resultPaths.ContainsKey($results)) {
            throw "Queue entries '$($resultPaths[$results])' and '$name' use the same results path: $results"
        }
        $resultPaths[$results] = $name
        if ((Test-Path -LiteralPath $results) -and -not $overwrite) {
            throw "Results already exist for '$name'; choose a new path or set overwrite=true: $results"
        }

        foreach ($outputPath in @($results, $status)) {
            $directory = Split-Path -Parent $outputPath
            if (-not (Test-Path -LiteralPath $directory -PathType Container)) {
                New-Item -ItemType Directory -Path $directory | Out-Null
            }
        }

        $preparedTests += [pscustomobject]@{
            Name = $name
            Engines = $engines
            Settings = $settings
            Results = $results
            Status = $status
            Overwrite = $overwrite
            Rounds = $rounds
        }
    }

    $littleBlitzerHash = (Get-FileHash -LiteralPath $LittleBlitzerPath -Algorithm SHA256).Hash
    if ($ValidateOnly) {
        Write-RunLog "QUEUE_VALIDATED tests=$($preparedTests.Count) littleblitzer_sha256=$littleBlitzerHash"
        exit 0
    }
    Write-RunLog "QUEUE_START tests=$($preparedTests.Count) littleblitzer_sha256=$littleBlitzerHash"

    foreach ($test in $preparedTests) {
        Write-RunLog "TEST_START name=$($test.Name) rounds=$($test.Rounds)"

        $arguments = @(
            '--batch',
            '--engines', (Convert-ToQuotedArgument $test.Engines),
            '--settings', (Convert-ToQuotedArgument $test.Settings),
            '--results', (Convert-ToQuotedArgument $test.Results),
            '--status', (Convert-ToQuotedArgument $test.Status)
        )
        if ($test.Overwrite) {
            $arguments += '--overwrite'
        }

        $process = Start-Process -FilePath $LittleBlitzerPath -ArgumentList ($arguments -join ' ') -PassThru -WindowStyle Hidden
        while (-not $process.WaitForExit(5000)) {
            $process.Refresh()
        }
        $exitCode = $process.ExitCode
        Write-RunLog "TEST_PROCESS_EXIT name=$($test.Name) exit=$exitCode"
        if ($exitCode -ne 0) {
            throw "LittleBlitzer failed for '$($test.Name)' with exit code $exitCode. See $($test.Status)"
        }

        $completedGames = @(
            Select-String -LiteralPath $test.Results -Pattern '^\[Result "(1-0|0-1|1/2-1/2)"\]$'
        ).Count
        if ($completedGames -ne $test.Rounds) {
            throw "PGN validation failed for '$($test.Name)': expected $($test.Rounds) games, found $completedGames."
        }

        $completeStatus = Select-String -LiteralPath $test.Status -Pattern "^.* COMPLETE completed=$($test.Rounds) total=$($test.Rounds) illegal=0 exit=0$" |
            Select-Object -Last 1
        if ($null -eq $completeStatus) {
            throw "Completion status validation failed for '$($test.Name)': $($test.Status)"
        }

        $testDirectory = Split-Path -Parent $test.Results
        $illegalFiles = @(Get-ChildItem -LiteralPath $testDirectory -File -Filter 'illegal*' -ErrorAction SilentlyContinue)
        if ($illegalFiles.Count -ne 0) {
            throw "Illegal-move files were produced for '$($test.Name)': $($illegalFiles.FullName -join ', ')"
        }

        $pgnHash = (Get-FileHash -LiteralPath $test.Results -Algorithm SHA256).Hash
        Write-RunLog "TEST_COMPLETE name=$($test.Name) games=$completedGames pgn_sha256=$pgnHash"
    }

    Write-RunLog "QUEUE_COMPLETE tests=$($preparedTests.Count)"
    exit 0
}
catch {
    Write-RunLog "QUEUE_FAILED message=$($_.Exception.Message)"
    Write-Error $_
    exit 1
}
