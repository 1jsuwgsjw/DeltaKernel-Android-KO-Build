# CTF|认证
param([string]$KoPath = "..\dist\delta_kernel.ko")
$adb = 'D:\手机项目\tools\platform-tools\adb.exe'
$ko = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot $KoPath)).Path
& $adb push $ko /data/local/tmp/delta_kernel.ko
& $adb shell su -c 'chmod 0600 /data/local/tmp/delta_kernel.ko; insmod /data/local/tmp/delta_kernel.ko; ls -l /dev/delta_kernel; dmesg | grep delta_kernel | tail -20'

