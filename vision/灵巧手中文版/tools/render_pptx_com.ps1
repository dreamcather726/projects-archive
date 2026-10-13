param(
    [Parameter(Mandatory = $true)]
    [string]$InputPath,
    [Parameter(Mandatory = $true)]
    [string]$OutputPath,
    [int]$Width = 1280,
    [int]$Height = 720
)

$ErrorActionPreference = 'Stop'
$inputResolved = (Resolve-Path -LiteralPath $InputPath).Path
$outputRoot = [System.IO.Path]::GetFullPath((Join-Path (Get-Location) $OutputPath))
[System.IO.Directory]::CreateDirectory($outputRoot) | Out-Null

$files = @()
if ([System.IO.Directory]::Exists($inputResolved)) {
    $files = Get-ChildItem -LiteralPath $inputResolved -Filter '*.pptx' |
        Where-Object { -not $_.Name.StartsWith('~$') } |
        Sort-Object Name
} else {
    $files = @(Get-Item -LiteralPath $inputResolved)
}

$powerPoint = New-Object -ComObject PowerPoint.Application
try {
    foreach ($file in $files) {
        $deckOutput = Join-Path $outputRoot $file.BaseName
        [System.IO.Directory]::CreateDirectory($deckOutput) | Out-Null
        Write-Output "Opening $($file.Name)"
        $presentation = $powerPoint.Presentations.Open($file.FullName, $true, $false, $false)
        try {
            foreach ($slide in $presentation.Slides) {
                $png = Join-Path $deckOutput ("slide-{0:D3}.png" -f $slide.SlideIndex)
                $slide.Export($png, 'PNG', $Width, $Height)
            }
            Write-Output "Rendered $($presentation.Slides.Count) slides: $($file.Name)"
        }
        finally {
            $presentation.Close()
            [void][Runtime.InteropServices.Marshal]::ReleaseComObject($presentation)
        }
    }
}
finally {
    $powerPoint.Quit()
    [void][Runtime.InteropServices.Marshal]::ReleaseComObject($powerPoint)
    [GC]::Collect()
    [GC]::WaitForPendingFinalizers()
}
