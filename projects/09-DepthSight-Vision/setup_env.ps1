param([switch]$Cpu)
$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot
$pythonExe = Join-Path $PSScriptRoot '.venv\Scripts\python.exe'
if (-not (Test-Path -LiteralPath $pythonExe)) {
    python -m venv .venv
    if ($LASTEXITCODE -ne 0) { throw 'Python 3.10+ is required.' }
}
if (-not $Cpu -and (Get-Command nvidia-smi -ErrorAction SilentlyContinue)) {
    & $pythonExe -c "from importlib.metadata import distributions; import sys; v={d.metadata['Name'].lower():d.version for d in distributions()}; sys.exit(0 if v.get('torch') == '2.11.0+cu128' and v.get('torchvision') == '0.26.0+cu128' else 1)"
    if ($LASTEXITCODE -ne 0) {
        $pythonTag = & $pythonExe -c "import sys; print('cp%d%d' % sys.version_info[:2])"
        $torchWheel = "https://download.pytorch.org/whl/cu128/torch-2.11.0%2Bcu128-$pythonTag-$pythonTag-win_amd64.whl?download=1"
        $visionWheel = "https://download.pytorch.org/whl/cu128/torchvision-0.26.0%2Bcu128-$pythonTag-$pythonTag-win_amd64.whl?download=1"
        & $pythonExe -m pip --disable-pip-version-check install --keyring-provider disabled --index-url https://pypi.org/simple --timeout 30 $torchWheel $visionWheel
        if ($LASTEXITCODE -ne 0) { throw 'CUDA PyTorch installation failed. Retry or use setup_env.ps1 -Cpu.' }
    }
}
& $pythonExe -m pip --disable-pip-version-check install --keyring-provider disabled --index-url https://pypi.org/simple --timeout 30 -r requirements.txt
if ($LASTEXITCODE -ne 0) { throw 'Dependency installation failed; check your network.' }
& $pythonExe -c "import torch, ultralytics; print('Ultralytics:', ultralytics.__version__); print('PyTorch:', torch.__version__); print('CUDA available:', torch.cuda.is_available())"
if ($LASTEXITCODE -ne 0) { throw 'Environment verification failed.' }
Write-Host 'Setup complete. Run start_camera.cmd.'
