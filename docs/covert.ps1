Get-Content $PSScriptRoot\calist.txt|ForEach-Object {
    $nl = $_.TrimStart()
    "L$nl"|Out-File -Append catemp.cc
}