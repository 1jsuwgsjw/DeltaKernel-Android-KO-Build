# CTF|认证
$adb = 'D:\手机项目\tools\platform-tools\adb.exe'
& $adb shell su -c 'rmmod delta_kernel; rm -f /data/local/tmp/delta_kernel.ko; dmesg | grep delta_kernel | tail -20'

