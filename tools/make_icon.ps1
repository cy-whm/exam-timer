# 生成应用图标 res/app.ico（多尺寸 PNG-in-ICO）
param([string]$OutDir = 'D:\workspace\timer\res')

Add-Type -AssemblyName System.Drawing

$outDir = $OutDir
$icoPath = Join-Path $outDir 'app.ico'

function New-RoundedPath([float]$l, [float]$t, [float]$w, [float]$h, [float]$r) {
    $p = New-Object System.Drawing.Drawing2D.GraphicsPath
    $d = $r * 2
    $p.AddArc($l, $t, $d, $d, 180, 90)
    $p.AddArc($l + $w - $d, $t, $d, $d, 270, 90)
    $p.AddArc($l + $w - $d, $t + $h - $d, $d, $d, 0, 90)
    $p.AddArc($l, $t + $h - $d, $d, $d, 90, 90)
    $p.CloseFigure()
    return $p
}

function Render-Icon([int]$size) {
    $bmp = New-Object System.Drawing.Bitmap($size, $size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.Clear([System.Drawing.Color]::Transparent)

    $s = $size / 256.0
    $g.ScaleTransform($s, $s)

    # 背景
    $bgRect = New-Object System.Drawing.RectangleF(6, 6, 244, 244)
    $bgPath = New-RoundedPath 6 6 244 244 58
    $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
        $bgRect,
        [System.Drawing.Color]::FromArgb(255, 32, 42, 56),
        [System.Drawing.Color]::FromArgb(255, 14, 18, 23),
        90.0)
    $g.FillPath($brush, $bgPath)
    $brush.Dispose()

    # 外环
    $ringPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(255, 79, 195, 247), 13)
    $ringPen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
    $ringPen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
    $g.DrawEllipse($ringPen, 46, 46, 164, 164)
    $ringPen.Dispose()

    # 高亮弧（右上 1/4）
    $arcPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(255, 120, 220, 255), 13)
    $arcPen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
    $arcPen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
    $g.DrawArc($arcPen, 46, 46, 164, 164, 268, 68)
    $arcPen.Dispose()

    # 指针
    $handPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(255, 236, 244, 252), 9)
    $handPen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
    $handPen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
    $g.DrawLine($handPen, 128, 128, 128, 78)     # 分针向上
    $g.DrawLine($handPen, 128, 128, 172, 152)    # 时针
    $handPen.Dispose()

    # 中心点
    $dot = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 79, 195, 247))
    $g.FillEllipse($dot, 118, 118, 20, 20)
    $dot.Dispose()

    $g.Dispose()
    return $bmp
}

$sizes = @(16, 20, 24, 32, 40, 48, 64, 128, 256)
$pngs = @()
foreach ($sz in $sizes) {
    $bmp = Render-Icon $sz
    $ms = New-Object System.IO.MemoryStream
    $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    $pngs += , $ms.ToArray()
    $bmp.Dispose(); $ms.Dispose()
}

$fs = [System.IO.File]::Create($icoPath)
$bw = New-Object System.IO.BinaryWriter($fs)
$bw.Write([UInt16]0)              # reserved
$bw.Write([UInt16]1)              # type = icon
$bw.Write([UInt16]$sizes.Count)

$offset = 6 + 16 * $sizes.Count
for ($i = 0; $i -lt $sizes.Count; $i++) {
    $sz = $sizes[$i]
    $dim = if ($sz -ge 256) { 0 } else { $sz }
    $bw.Write([Byte]$dim)
    $bw.Write([Byte]$dim)
    $bw.Write([Byte]0)
    $bw.Write([Byte]0)
    $bw.Write([UInt16]1)
    $bw.Write([UInt16]32)
    $bw.Write([UInt32]$pngs[$i].Length)
    $bw.Write([UInt32]$offset)
    $offset += $pngs[$i].Length
}
foreach ($p in $pngs) { $bw.Write($p) }
$bw.Flush(); $bw.Dispose(); $fs.Dispose()

"wrote $icoPath ({0:N0} bytes, {1} sizes)" -f (Get-Item $icoPath).Length, $sizes.Count
