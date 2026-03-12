$leap_server = ""
$counter = 0
do {
  $leap_server = Get-Process -Name "LeapSvc" -ErrorAction SilentlyContinue
  if ($leap_server) {
    echo "Waited $counter seconds for LeapSvc.exe to exit..."
    Start-Sleep -Seconds 1
    $counter += 1
  }
} while ($leap_server -and $counter -lt 30)
