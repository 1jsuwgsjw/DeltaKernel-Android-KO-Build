# CTF|认证 三角洲内核（Android arm64）

本工程已经从错误的 Windows 驱动方向改为 Android GKI 6.6 外置内核模块 `.ko`。旧 Windows 原型保存在 `windows_prototype`，不再参与当前构建。

## 当前设备基线

```text
Android: 16
ABI: arm64-v8a
Kernel: 6.6.89-android15-7-o-g880af585653a9e
Kernel family: Android 15 GKI 6.6
Root: KernelSU 4.1.3
SELinux: Enforcing
CONFIG_MODULES=y
CONFIG_MODULE_UNLOAD=y
CONFIG_MODVERSIONS=y
```

## 工程结构

```text
kernel_module/       delta_kernel.ko 源码和 Kbuild Makefile
shared/              内核与 Android 用户层共用 IOCTL 协议
android_client/      /dev/delta_kernel 连通测试
scripts/             WSL 编译、推送、加载、卸载脚本
research/            设备基线与高精度检索结论
third_party/imgui/   Dear ImGui 源码，后续接 Android NativeActivity/EGL
windows_prototype/   已归档的错误平台原型
```

## 最小功能

模块加载后创建：

```text
/dev/delta_kernel
```

当前协议实现：

- `DELTA_IOCTL_GET_VERSION`
- `DELTA_IOCTL_PING`

这一步先验证四件事：源码匹配、Module.symvers、vermagic、用户层 IOCTL。基础链稳定后，再把三角洲数据快照和 ImGui Android 绘制接入。

## 编译

准备匹配的 Android kernel source/output 后，在 WSL 中执行：

```bash
export KERNEL_SRC=/path/to/android15-6.6
export KERNEL_OUT=/path/to/matching/kernel/out
bash scripts/build_ko.sh
```

或从 PowerShell 调用：

```powershell
.\scripts\build_ko.ps1 -KernelSrc 'D:\android-kernel' -KernelOut 'D:\android-kernel\out'
```

输出：

```text
dist/delta_kernel.ko
```

由于本机剩余磁盘不足且尚未安装 WSL Linux 发行版，推荐直接使用已准备的云端精确构建：

```text
.github/workflows/build_android_ko.yml
```

工作流采用：

```text
manifest branch: oneplus/sm8750
manifest file: oneplus_ace_6.xml
device build: sun perf
```

产物不仅包含 `delta_kernel.ko`，还包含构建时的 `.config`、`Module.symvers`、`modinfo.txt` 和 SHA-256，避免只拿到一个无法判断 ABI 的孤立 KO。

检索报告位于 `research/高精度检索结论.md`。
