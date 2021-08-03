$files=get-childitem -Path . -Include @("*.h","*.cpp") -Recurse
foreach ($f in $files)
{
	(Get-Content $f.PSPath) | 
	Foreach-Object {$_ -replace "\xEF\xBB\xBF", ""} | 
	Set-Content $f.PSPath
}
