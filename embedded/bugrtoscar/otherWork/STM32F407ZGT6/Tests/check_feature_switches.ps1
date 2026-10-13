$ErrorActionPreference = "Stop"

$testDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectDirectory = Split-Path -Parent $testDirectory
$compiler = "D:\Application\Keil5\ARM\ARMCLANG\Bin\armclang.exe"

$includeArguments = @(
    "-I$projectDirectory\Libraries\STM32F4xx_StdPeriph_Driver\inc",
    "-I$projectDirectory\CMSIS\Core\Include",
    "-I$projectDirectory\CMSIS\Device\Include",
    "-I$projectDirectory\App\Inc",
    "-I$projectDirectory\Config\Inc",
    "-I$projectDirectory\Common\Inc",
    "-I$projectDirectory\BSP\Inc",
    "-I$projectDirectory\Libraries\IMU\Inc",
    "-I$projectDirectory\Libraries\StepperMotor\Inc",
    "-I$projectDirectory\RTOS\FreeRTOS-Kernel\include",
    "-I$projectDirectory\RTOS\FreeRTOS-Kernel\portable\GCC\ARM_CM4F"
)

$sources = @(
    "$projectDirectory\App\Src\app.c",
    "$projectDirectory\App\Src\app_tasks.c",
    "$projectDirectory\App\Src\host_protocol.c",
    "$projectDirectory\App\Src\mecanum_chassis.c",
    "$projectDirectory\BSP\Src\bsp_uart.c",
    "$projectDirectory\Libraries\IMU\Src\imu.c"
)

$configurations = @(
    @{ Imu = 0; Host = 0; Watchdog = 0 },
    @{ Imu = 0; Host = 0; Watchdog = 1 },
    @{ Imu = 0; Host = 1; Watchdog = 0 },
    @{ Imu = 0; Host = 1; Watchdog = 1 },
    @{ Imu = 1; Host = 0; Watchdog = 0 },
    @{ Imu = 1; Host = 0; Watchdog = 1 },
    @{ Imu = 1; Host = 1; Watchdog = 0 },
    @{ Imu = 1; Host = 1; Watchdog = 1 }
)

foreach ($configuration in $configurations) {
    $arguments = @(
        "--target=arm-arm-none-eabi",
        "-mcpu=cortex-m4",
        "-std=c11",
        "-fsyntax-only",
        "-Werror",
        "-DSTM32F407xx",
        "-DSTM32F40_41xxx",
        "-DUSE_STDPERIPH_DRIVER",
        "-DAPP_CONFIG_IMU_ENABLED=$($configuration.Imu)U",
        "-DAPP_CONFIG_HOST_CONTROL_ENABLED=$($configuration.Host)U",
        "-DAPP_CONFIG_IWDG_ENABLED=$($configuration.Watchdog)U"
    )
    $arguments += $includeArguments
    $arguments += $sources

    & $compiler $arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Feature switch check failed: IMU=$($configuration.Imu), HOST=$($configuration.Host), IWDG=$($configuration.Watchdog)"
    }
    Write-Host "feature switches passed: IMU=$($configuration.Imu), HOST=$($configuration.Host), IWDG=$($configuration.Watchdog)"
}
