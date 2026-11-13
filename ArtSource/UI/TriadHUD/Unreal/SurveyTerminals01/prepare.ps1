$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
$root=$PSScriptRoot
$items=@(@{source='verdance-generated.png';final='T_Survey_Verdance_Discovery_1920x1080.png'},@{source='cinder-generated.png';final='T_Survey_Cinder_Match_1920x1080.png'})
$records=@()
foreach($item in $items){
 $src=Join-Path $root ('source/'+$item.source);$dest=Join-Path $root $item.final
 if(Test-Path -LiteralPath $dest){throw 'Delivery exists; do not overwrite'}
 $im=[Drawing.Image]::FromFile($src)
 try{
  $bitmap=[Drawing.Bitmap]::new(1920,1080,[Drawing.Imaging.PixelFormat]::Format24bppRgb)
  $g=[Drawing.Graphics]::FromImage($bitmap)
  try{$g.CompositingMode=[Drawing.Drawing2D.CompositingMode]::SourceCopy;$g.InterpolationMode=[Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic;$g.PixelOffsetMode=[Drawing.Drawing2D.PixelOffsetMode]::HighQuality;$g.DrawImage($im,0,0,1920,1080);$bitmap.Save($dest,[Drawing.Imaging.ImageFormat]::Png)}finally{$g.Dispose();$bitmap.Dispose()}
  $check=[Drawing.Image]::FromFile($dest)
  try{if($check.Width -ne 1920 -or $check.Height -ne 1080){throw 'Invalid resolution'};$records+=@{file=$item.final;width=$check.Width;height=$check.Height;bytes=(Get-Item $dest).Length;sha256=(Get-FileHash $dest -Algorithm SHA256).Hash;source=$item.source;source_width=$im.Width;source_height=$im.Height;source_sha256=(Get-FileHash $src -Algorithm SHA256).Hash}}finally{$check.Dispose()}
 }finally{$im.Dispose()}
}
@{stage='presentation approved';integrated=$false;presentation_approved=$true;approval="Nicholas approved both images and requested delivery to Ryan's team on November 13, 2026 (America/New_York).";images=$records} | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $root 'manifest.json')
$records | ForEach-Object {[pscustomobject]$_} | Format-Table file,width,height,bytes
