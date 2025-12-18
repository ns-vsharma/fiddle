# --- Configuration ---
$ProjectRoot = Get-Location
$Version = "1.0.1.0"
$BuildDir = "$ProjectRoot\Build"
$MSI_Name = "MyApp_v$Version.msi"

# Paths to Tools (Update these based on your installation)
$MSBuildPath = "C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\MSBuild.exe"
$WixBinDir = "C:\Program Files (x86)\WiX Toolset v3.11\bin"
$SignToolPath = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe"

# Certificate details for signing (Optional - replace with your actual cert info)
$CertPath = "$ProjectRoot\Certs\MyCompany.pfx"
$CertPass = "YourPassword"

# 1. Cleanup old builds
if (Test-Path $BuildDir) { Remove-Item -Recurse -Force $BuildDir }
New-Item -ItemType Directory -Path $BuildDir

Write-Host "--- Step 1: Compiling C++ Services ---" -ForegroundColor Cyan

# 2. Compile MainService and UpdaterService
# Assumes you have a .sln or individual .vcxproj files
& $MSBuildPath "$ProjectRoot\MyServices.sln" /p:Configuration=Release /p:Platform=x64 /t:Rebuild

# Copy binaries to Build directory
Copy-Item "$ProjectRoot\x64\Release\MainService.exe" $BuildDir
Copy-Item "$ProjectRoot\x64\Release\UpdaterService.exe" $BuildDir

# 3. Sign Binaries (Recommended for Modern Standby/Service stability)
if (Test-Path $SignToolPath -and Test-Path $CertPath) {
    Write-Host "--- Step 2: Signing Binaries ---" -ForegroundColor Cyan
    & $SignToolPath sign /f $CertPath /p $CertPass /t http://timestamp.digicert.com "$BuildDir\*.exe"
}

Write-Host "--- Step 3: Building MSI Installer ---" -ForegroundColor Cyan

# 4. Compile WiX Source
# We pass the version to WiX via a variable
& "$WixBinDir\candle.exe" "$ProjectRoot\Product.wxs" -dVersion=$Version -o "$BuildDir\Product.wixobj"

# 5. Link WiX Objects into MSI
& "$WixBinDir\light.exe" "$BuildDir\Product.wixobj" -o "$BuildDir\$MSI_Name" -ext WixUIExtension

# 6. Sign the MSI
if (Test-Path $SignToolPath -and Test-Path $CertPath) {
    & $SignToolPath sign /f $CertPath /p $CertPass /t http://timestamp.digicert.com "$BuildDir\$MSI_Name"
}

Write-Host "--- Build Complete: $BuildDir\$MSI_Name ---" -ForegroundColor Green
