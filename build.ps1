# Builds the Zygisk module and packs dist/zygisk_il2cpp_dumper.zip
# Requires: Android NDK (set ANDROID_NDK_HOME), cmake, ninja (ships with NDK/SDK cmake).
$ErrorActionPreference = "Stop"
$root = $PSScriptRoot
$ndk = $env:ANDROID_NDK_HOME
if (-not $ndk) { throw "Set ANDROID_NDK_HOME to your NDK path" }
$stage = "$root\dist\stage"
Remove-Item -Recurse -Force "$root\dist" -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force "$stage\zygisk" | Out-Null
Copy-Item "$root\module\template\*" $stage -Recurse

foreach ($abi in @("arm64-v8a", "armeabi-v7a")) {
    $b = "$root\build\$abi"
    cmake -S "$root\module\jni" -B $b -G "Unix Makefiles" "-DCMAKE_MAKE_PROGRAM=$ndk\prebuilt\windows-x86_64\bin\make.exe" `
        "-DCMAKE_TOOLCHAIN_FILE=$ndk\build\cmake\android.toolchain.cmake" `
        "-DANDROID_ABI=$abi" -DANDROID_PLATFORM=android-24 -DCMAKE_BUILD_TYPE=Release
    cmake --build $b -j 8
    Copy-Item "$b\libil2cppdumper.so" "$stage\zygisk\$abi.so"
}

# Compress-Archive writes entry names with backslashes on Windows
# ("zygisk\arm64-v8a.so"); Magisk/APatch then extract them as one file and the
# zygisk/ directory never exists, so the module fails to install. Build the zip
# manually with forward-slash entry names instead.
$zipPath = "$root\dist\zygisk_il2cpp_dumper.zip"
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [System.IO.Compression.ZipFile]::Open($zipPath, 'Create')
try {
    Get-ChildItem -Path $stage -Recurse -File | ForEach-Object {
        $rel = $_.FullName.Substring($stage.Length + 1).Replace('\', '/')
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
            $zip, $_.FullName, $rel, [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
} finally {
    $zip.Dispose()
}
Write-Host "Done: dist\zygisk_il2cpp_dumper.zip"

