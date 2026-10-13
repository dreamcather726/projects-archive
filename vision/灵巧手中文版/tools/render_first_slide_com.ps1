param(
    [Parameter(Mandatory=$true)][string]$InputPath,
    [Parameter(Mandatory=$true)][string]$OutputPath,
    [int]$Width = 1600,
    [int]$Height = 900
)

$ErrorActionPreference = 'Stop'
$inputResolved = (Resolve-Path -LiteralPath $InputPath).Path
New-Item -ItemType Directory -Path $OutputPath -Force | Out-Null
$outputResolved = (Resolve-Path -LiteralPath $OutputPath).Path

$files = Get-ChildItem -LiteralPath $inputResolved -File -Filter '*.pptx' |
    Where-Object { $_.Name -notlike '~$*' -and $_.Name -like '*FullDeck*' } |
    Sort-Object Name

$powerPoint = New-Object -ComObject PowerPoint.Application
$powerPoint.Visible = -1
try {
    foreach ($file in $files) {
        Write-Output "Opening $($file.Name)"
        $presentation = $powerPoint.Presentations.Open($file.FullName, $true, $false, $false)
        try {
            Start-Sleep -Milliseconds 1500
            $target = Join-Path $outputResolved ($file.BaseName + '.png')
            $presentation.Slides.Item(1).Export($target, 'PNG', $Width, $Height)
            Write-Output "Rendered cover: $($file.Name)"
        }
        finally {
            $presentation.Close()
        }
    }
}
finally {
    $powerPoint.Quit()
}
