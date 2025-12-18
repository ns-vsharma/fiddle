# --- Configuration ---
$ProjectRoot = Get-Location
$Version = "1.0.1.0"
$BuildDir = "$ProjectRoot\Build"
$MSI_Name = "MyApp_v$Version.msi"

# Tools Paths
$WixBinDir = "C:\Program Files (x86)\WiX Toolset v3.11\bin"
# Path to find vcvarsall.bat (Update based on your VS version)
$VsDevCmdPath = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"

# 1. Cleanup and Prepare
if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }
New-Item -ItemType Directory -Path $BuildDir

Write-Host "--- Step 1: Loading Visual Studio Compiler ---" -ForegroundColor Cyan

# 2. Import VC Environment variables into PowerShell
# This allows us to use 'cl.exe' directly
$tempFile = [IO.Path]::GetTempFileName()
cmd /c " `"$VsDevCmdPath`" x64 && set > `"$tempFile`" "
Get-Content $tempFile | Foreach-Object {
    if ($_ -match '^(.*?)=(.*)$') {
        Set-Content "env:\$($matches[1])" $matches[2]
    }
}
Remove-Item $tempFile

Write-Host "--- Step 2: Compiling C++ Files Directly ---" -ForegroundColor Cyan

# 3. Compile MainService.cpp
# /O2: Optimization, /MT: Static Link (no DLL dependencies), /D: Defines
& cl.exe /O2 /MT /EHsc /DUNICODE /D_UNICODE "$ProjectRoot\MainService.cpp" /Fe:"$BuildDir\MainService.exe" /link Advapi32.lib User32.lib

# 4. Compile UpdaterService.cpp
& cl.exe /O2 /MT /EHsc /DUNICODE /D_UNICODE "$ProjectRoot\UpdaterService.cpp" /Fe:"$BuildDir\UpdaterService.exe" /link Advapi32.lib User32.lib User32.lib Shell32.lib

Write-Host "--- Step 3: Building MSI with WiX ---" -ForegroundColor Cyan

# 5. Compile WiX Source
& "$WixBinDir\candle.exe" "$ProjectRoot\Product.wxs" -dVersion=$Version -o "$BuildDir\Product.wixobj"

# 6. Link WiX Objects
& "$WixBinDir\light.exe" "$BuildDir\Product.wixobj" -o "$BuildDir\$MSI_Name" -ext WixUIExtension

Write-Host "--- Success! MSI generated at $BuildDir\$MSI_Name ---" -ForegroundColor Green
