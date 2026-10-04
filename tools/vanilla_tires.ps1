# vanilla_tires.ps1: the grip of every vanilla SnowRunner tire, read from the game's initial.pak (read only), and the
# table src\vanilla_tires.h that TirePressure.asi's vanilla balance uses.
# Each tire's WheelFriction is resolved the way the game does it: the _template it names (the file's own _templates
# first, then [media]\_templates\trucks.xml), then its own attributes on top; a tire without a WheelFriction takes the
# one of its TruckTire template. The header keeps only the tires no other tire beats in ground, asphalt and mud grip
# at once (the vanilla envelope): a tire that some vanilla tire matches or beats in all three is inside it.
#   powershell -ExecutionPolicy Bypass -File tools\vanilla_tires.ps1 [-Pak <initial.pak>] [-Csv <all tires.csv>]
param(
    [string]$Pak = 'C:\Program Files (x86)\Steam\steamapps\common\Snowrunner\preload\paks\client\initial.pak',
    [string]$Csv = ''
)
$ErrorActionPreference = 'Stop'
$inv = [Globalization.CultureInfo]::InvariantCulture
Add-Type -AssemblyName System.IO.Compression.FileSystem

function ReadXml([IO.Compression.ZipArchiveEntry]$e) {
    $r = New-Object IO.StreamReader($e.Open())
    try { $t = $r.ReadToEnd() } finally { $r.Dispose() }
    $x = New-Object Xml.XmlDocument
    $x.LoadXml('<root>' + ($t -replace '<\?xml[^>]*\?>', '') + '</root>') # the files hold several top level elements
    $x
}
function Num([string]$s) { [double]::Parse($s, $inv) }

$zip = [IO.Compression.ZipFile]::OpenRead($Pak)
try {
    $global = ReadXml ($zip.Entries | Where-Object { $_.FullName -match '^\[media\]\\_templates\\trucks\.xml$' } | Select-Object -First 1)
    $globalWf = @{}
    foreach ($n in $global.SelectNodes('/root/_templates/WheelFriction/*')) { $globalWf[$n.Name] = $n }
    $tires = New-Object Collections.Generic.List[object]
    foreach ($e in $zip.Entries | Where-Object { $_.FullName -match '\\classes\\wheels\\[^\\]+\.xml$' }) {
        $x = ReadXml $e
        $localWf = @{}; foreach ($n in $x.SelectNodes('/root/_templates/WheelFriction/*')) { $localWf[$n.Name] = $n }
        $localTire = @{}; foreach ($n in $x.SelectNodes('/root/_templates/TruckTire/*')) { $localTire[$n.Name] = $n }
        foreach ($tt in $x.SelectNodes('//TruckWheels/TruckTires/TruckTire')) {
            $wf = $tt.SelectSingleNode('WheelFriction')
            $tpl = $tt.GetAttribute('_template')
            if (-not $wf -and $tpl -and $localTire.ContainsKey($tpl)) { $wf = $localTire[$tpl].SelectSingleNode('WheelFriction') }
            if (-not $wf) { continue }
            $v = @{ BodyFriction = $null; BodyFrictionAsphalt = $null; SubstanceFriction = $null }
            $chain = @(); $name = $wf.GetAttribute('_template'); $seen = @{}
            while ($name -and -not $seen.ContainsKey($name)) {
                $seen[$name] = 1
                $t = if ($localWf.ContainsKey($name)) { $localWf[$name] } elseif ($globalWf.ContainsKey($name)) { $globalWf[$name] } else { $null }
                if (-not $t) { break }
                $chain = @($t) + $chain
                $name = $t.GetAttribute('_template')
            }
            foreach ($node in $chain + @($wf)) { foreach ($k in @($v.Keys)) { if ($node.HasAttribute($k)) { $v[$k] = $node.GetAttribute($k) } } }
            if ($null -eq $v.BodyFriction -or $null -eq $v.BodyFrictionAsphalt -or $null -eq $v.SubstanceFriction) { continue }
            $tires.Add([pscustomobject]@{
                File = [IO.Path]::GetFileNameWithoutExtension($e.FullName); Tire = $tt.GetAttribute('Name'); Class = $wf.GetAttribute('_template')
                G = Num $v.BodyFriction; A = Num $v.BodyFrictionAsphalt; M = Num $v.SubstanceFriction })
        }
    }
} finally { $zip.Dispose() }

# tires without a tire class are props (an invisible cabin wheel, a rocket trailer): not trucks' tires
$real = @($tires | Where-Object { $_.Class -and $_.Tire -ne 'invisible' })
if ($Csv) { $real | ForEach-Object { '{0},{1},{2},{3},{4},{5}' -f $_.File, $_.Tire, $_.Class, $_.G.ToString($inv), $_.A.ToString($inv), $_.M.ToString($inv) } | Set-Content -Encoding UTF8 $Csv }
$unique = @($real | Group-Object { '{0}|{1}|{2}' -f $_.G.ToString($inv), $_.A.ToString($inv), $_.M.ToString($inv) } | ForEach-Object { $_.Group[0] })
$front = @($unique | Where-Object { $p = $_; -not ($unique | Where-Object { $_.G -ge $p.G -and $_.A -ge $p.A -and $_.M -ge $p.M -and ($_.G -gt $p.G -or $_.A -gt $p.A -or $_.M -gt $p.M) } | Select-Object -First 1) } | Sort-Object G, A, M)
"vanilla tires: $($real.Count), different grip sets: $($unique.Count), envelope: $($front.Count)"

$out = New-Object Text.StringBuilder
[void]$out.AppendLine('// vanilla_tires.h: written by tools\vanilla_tires.ps1 from the game''s initial.pak. Do not edit by hand.')
[void]$out.AppendLine("// The vanilla grip envelope: of the $($real.Count) vanilla truck tires ($($unique.Count) different grip sets), the ones no other")
[void]$out.AppendLine('// tire matches or beats in ground (BodyFriction), asphalt (BodyFrictionAsphalt) and mud (SubstanceFriction) at once.')
[void]$out.AppendLine('#pragma once')
[void]$out.AppendLine('')
[void]$out.AppendLine("static const float kVanillaEnvelope[$($front.Count)][3] = {")
foreach ($f in $front) { [void]$out.AppendLine(('    {{ {0}f, {1}f, {2}f }}, // {3} / {4} ({5})' -f $f.G.ToString('0.0##', $inv), $f.A.ToString('0.0##', $inv), $f.M.ToString('0.0##', $inv), $f.File, $f.Tire, $f.Class)) }
[void]$out.AppendLine('};')
$header = Join-Path (Split-Path $PSScriptRoot -Parent) 'src\vanilla_tires.h'
[IO.File]::WriteAllText($header, $out.ToString(), (New-Object Text.UTF8Encoding $false))
"wrote $header"
$front | ForEach-Object { '  {0,5} {1,5} {2,5}  {3} / {4} ({5})' -f $_.G.ToString($inv), $_.A.ToString($inv), $_.M.ToString($inv), $_.File, $_.Tire, $_.Class }
