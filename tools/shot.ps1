# 启动计时器并截图到 shots 目录（开发自检用）
# 可用环境变量：
#   SHOT_OUT    输出文件名（不含扩展名）
#   SHOT_WAIT   启动后等待毫秒（默认 1500）
#   SHOT_CLICKS 依次点击的 DIP 坐标，形如 '220,482;264,73'
#   SHOT_AFTER  点击完成后再等待毫秒（默认 400）
#   SHOT_ARGS   传给 Timer.exe 的额外参数，如 '--seconds 8'
#   SHOT_FULL   设为 1 时截整屏（用于看托盘气泡）
Add-Type -AssemblyName System.Drawing

$name = if ($env:SHOT_OUT) { $env:SHOT_OUT } else { 'shot' }
$wait = if ($env:SHOT_WAIT) { [int]$env:SHOT_WAIT } else { 1500 }
$exe  = 'D:\workspace\timer\build\Release\Timer.exe'
$ctl  = 'D:\workspace\timer\tools\uictl.exe'
$rect = 'D:\workspace\timer\tools\winrect.exe'
$dir  = 'D:\workspace\timer\shots'
New-Item -ItemType Directory -Force -Path $dir | Out-Null

Get-Process Timer -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 250

if ($env:SHOT_ARGS) {
    $cmdline = "start `"`" `"$exe`" " + $env:SHOT_ARGS
} else {
    $cmdline = "start `"`" `"$exe`""
}
cmd /c $cmdline
Start-Sleep -Milliseconds $wait

if ($env:SHOT_CLICKS) {
    foreach ($c in ($env:SHOT_CLICKS -split ';')) {
        if ($c.Trim() -eq '') { continue }
        $parts = $c -split ','
        & $ctl click $parts[0] $parts[1] | Out-Null
        Start-Sleep -Milliseconds 350
    }
}

$after = if ($env:SHOT_AFTER) { [int]$env:SHOT_AFTER } else { 400 }
Start-Sleep -Milliseconds $after

# 保证窗口在最前面，否则截图会被其它窗口遮挡
& $ctl top | Out-Null
Start-Sleep -Milliseconds 260

$fields = (& $rect) -split '\s+'
if ($fields[0] -eq 'NOTFOUND') { "NO WINDOW"; exit 1 }

$out = Join-Path $dir ($name + '.png')

if ($env:SHOT_FULL -eq '1') {
    Add-Type -AssemblyName System.Windows.Forms
    $vs = [System.Windows.Forms.SystemInformation]::VirtualScreen
    $bmp = New-Object System.Drawing.Bitmap $vs.Width, $vs.Height
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($vs.X, $vs.Y, 0, 0, (New-Object System.Drawing.Size($vs.Width, $vs.Height)))
    $bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose(); $bmp.Dispose()
    "$out  fullscreen  " + (Get-Item $out).Length + " bytes"
}
else {
    $l = [int]$fields[0]; $t = [int]$fields[1]; $w = [int]$fields[2]; $h = [int]$fields[3]
    $bmp = New-Object System.Drawing.Bitmap $w, $h
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($l, $t, 0, 0, (New-Object System.Drawing.Size($w, $h)))
    $bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose(); $bmp.Dispose()
    "$out  ${w}x${h} @($l,$t)  " + (Get-Item $out).Length + " bytes"
}
