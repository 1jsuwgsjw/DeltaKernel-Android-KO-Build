# CTF|认证
param(
    [Parameter(Mandatory=$true)][string]$KernelSrc,
    [string]$KernelOut = "",
    [string]$WslDistro = ""
)
$root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$srcWsl = (wsl wslpath -a $KernelSrc).Trim()
if (-not $KernelOut) { $KernelOut = Join-Path $KernelSrc 'out\android15-6.6' }
$outWsl = (wsl wslpath -a $KernelOut).Trim()
$scriptWsl = (wsl wslpath -a (Join-Path $root 'scripts\build_ko.sh')).Trim()
$args = @('bash','-lc',"KERNEL_SRC='$srcWsl' KERNEL_OUT='$outWsl' '$scriptWsl'")
if ($WslDistro) { wsl -d $WslDistro -- @args } else { wsl -- @args }
exit $LASTEXITCODE

