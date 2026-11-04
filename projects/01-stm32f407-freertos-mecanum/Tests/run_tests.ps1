$ErrorActionPreference = "Stop"

$testDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectDirectory = Split-Path -Parent $testDirectory
$buildDirectory = Join-Path $testDirectory "build"
$mecanumTestExecutable = Join-Path $buildDirectory "test_mecanum.exe"
$hostProtocolTestExecutable = Join-Path $buildDirectory "test_host_protocol.exe"

New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null

$uartExecutable = Join-Path $buildDirectory "test_uart_tx.exe"
& gcc -std=c11 -Wall -Wextra -Werror -I (Join-Path $projectDirectory "BSP\Src") (Join-Path $testDirectory "test_uart_tx.c") -o $uartExecutable
if ($LASTEXITCODE -ne 0) { throw "UART TX tests build failed" }
& $uartExecutable
if ($LASTEXITCODE -ne 0) { throw "UART TX tests failed" }

$gccArguments = @(
    "-std=c11",
    "-Wall",
    "-Wextra",
    "-Werror",
    "-I", (Join-Path $testDirectory "stubs"),
    "-I", (Join-Path $projectDirectory "App\Inc"),
    "-I", (Join-Path $projectDirectory "Config\Inc"),
    "-I", (Join-Path $projectDirectory "Common\Inc"),
    "-I", (Join-Path $projectDirectory "BSP\Inc"),
    "-I", (Join-Path $projectDirectory "Libraries\StepperMotor\Inc"),
    (Join-Path $testDirectory "test_mecanum.c"),
    (Join-Path $projectDirectory "App\Src\mecanum_chassis.c"),
    "-o", $mecanumTestExecutable
)

& gcc $gccArguments
if ($LASTEXITCODE -ne 0) {
    throw "C unit test build failed with exit code $LASTEXITCODE"
}

& $mecanumTestExecutable
if ($LASTEXITCODE -ne 0) {
    throw "Mecanum C unit tests failed with exit code $LASTEXITCODE"
}

$hostProtocolArguments = @(
    "-std=c11",
    "-Wall",
    "-Wextra",
    "-Werror",
    "-DAPP_CONFIG_IMU_ENABLED=0U",
    "-I", (Join-Path $testDirectory "stubs"),
    "-I", (Join-Path $projectDirectory "App\Inc"),
    "-I", (Join-Path $projectDirectory "Config\Inc"),
    "-I", (Join-Path $projectDirectory "Common\Inc"),
    "-I", (Join-Path $projectDirectory "BSP\Inc"),
    "-I", (Join-Path $projectDirectory "Libraries\StepperMotor\Inc"),
    (Join-Path $testDirectory "test_host_protocol.c"),
    (Join-Path $projectDirectory "App\Src\host_protocol.c"),
    "-o", $hostProtocolTestExecutable
)

& gcc $hostProtocolArguments
if ($LASTEXITCODE -ne 0) {
    throw "Host protocol C unit test build failed with exit code $LASTEXITCODE"
}

& $hostProtocolTestExecutable
if ($LASTEXITCODE -ne 0) {
    throw "Host protocol C unit tests failed with exit code $LASTEXITCODE"
}

# Repeat with IMU enabled to test fresh samples and stale-data protection.
$imuArguments = $hostProtocolArguments | Where-Object { $_ -ne "-DAPP_CONFIG_IMU_ENABLED=0U" }
$imuArguments = @("-I", (Join-Path $projectDirectory "Libraries\IMU\Inc")) + $imuArguments
& gcc $imuArguments
if ($LASTEXITCODE -ne 0) { throw "IMU-enabled host protocol build failed" }
& $hostProtocolTestExecutable
if ($LASTEXITCODE -ne 0) { throw "IMU-enabled host protocol tests failed" }

$integrationExecutable = Join-Path $buildDirectory "test_safety_integration.exe"
$integrationArguments = @($hostProtocolArguments | ForEach-Object {
    if ($_ -eq (Join-Path $testDirectory "test_host_protocol.c")) {
        Join-Path $testDirectory "test_safety_integration.c"
    } elseif ($_ -eq $hostProtocolTestExecutable) {
        $integrationExecutable
    } else { $_ }
})
$integrationArguments += (Join-Path $projectDirectory "App\Src\mecanum_chassis.c")
& gcc $integrationArguments
if ($LASTEXITCODE -ne 0) { throw "Safety integration build failed" }
& $integrationExecutable
if ($LASTEXITCODE -ne 0) { throw "Safety integration tests failed" }

Push-Location $projectDirectory
try {
    & python -m unittest discover -s Tests -p "test_*.py" -v
    if ($LASTEXITCODE -ne 0) {
        throw "Python unit tests failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}
