param ([string] $templateFilename, [string] $outputFilename)

$versionParts = Select-String -Path "${env:SOLUTIONDIR}configure.ac" -Pattern '^AC_INIT\(\[HexChat\],\[([^]]+)\]\)$' | Select-Object -First 1 | %{ $_.Matches[0].Groups[1].Value.Split('.') }

[string[]] $contents = Get-Content $templateFilename -Encoding UTF8 | %{
	while ($_ -match '^(.*?)<#=(.*?)#>(.*?)$') {
		$_ = $Matches[1] + $(Invoke-Expression $Matches[2]) + $Matches[3]
	}
	$_
}

[System.IO.File]::WriteAllLines($outputFilename, $contents)
