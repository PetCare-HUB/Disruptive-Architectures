param(
    [ValidateSet('wokwi', 'fisico')][string]$Perfil = 'wokwi',
    [string]$ArduinoCli = 'C:/Program Files/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe',
    [string]$ArduinoConfig = 'C:/ac/arduino-cli.yaml',
    [string]$Cxx = 'C:/ac/data/packages/esp32/tools/esp-x32/2601/bin/xtensa-esp32s3-elf-g++.exe'
)
$ErrorActionPreference = 'Stop'
& $Cxx -std=c++17 -fsyntax-only (Join-Path $PSScriptRoot 'tests/controle_porcao_test.cpp')
if ($LASTEXITCODE -ne 0) { throw 'Falha nos testes do controlador.' }
Write-Output '7 cenarios do controlador aprovados em static_assert.'
$modo = if ($Perfil -eq 'wokwi') { 1 } else { 0 }
$destino = Join-Path $PSScriptRoot "build/$Perfil"
& $ArduinoCli compile --config-file $ArduinoConfig --fqbn 'esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,CDCOnBoot=default' --build-property "compiler.cpp.extra_flags=-DPETCARE_SIMULATION=$modo" --build-path $destino $PSScriptRoot
if ($LASTEXITCODE -ne 0) { throw "Falha ao compilar $Perfil." }
Write-Output "Binarios gerados em $destino. Compilar nao grava a placa."
