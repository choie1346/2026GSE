param([switch]$Check)
$ErrorActionPreference = 'Stop'
$shaderNames = @('SolidRect.vs', 'SolidRect.fs', 'Lake.fs', 'PostProcess.vs', 'PostProcess.fs', 'Text.fs')
$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add('// Generated from Shaders by GenerateShaderSources.ps1. Do not edit directly.')
$lines.Add('#pragma once')
$lines.Add('namespace ShaderSources {')
$lines.Add('struct Entry { const char* path; const char* source; };')
$lines.Add('static const Entry entries[] = {')
foreach ($name in $shaderNames) {
    $source = [System.IO.File]::ReadAllText((Join-Path $PSScriptRoot ('Shaders/' + $name)))
    $source = $source.Replace("`r`n", "`n").TrimEnd([char[]]"`r`n")
    if ($source.Contains(')GLSL"')) { throw ('Raw string delimiter collision: ' + $name) }
    $lines.Add('{"./Shaders/' + $name + '", R"GLSL(' + $source + "`n" + ')GLSL"},')
}
$lines.Add('};')
$lines.Add('}')
$content = [string]::Join("`n", $lines) + "`n"
$destination = Join-Path $PSScriptRoot 'ShaderSources.generated.h'
$existing = if (Test-Path -LiteralPath $destination) { [System.IO.File]::ReadAllText($destination) } else { '' }
if ($Check) {
    if ($content -cne $existing) { throw 'ShaderSources.generated.h does not match the shader sources.' }
    Write-Output 'All six compiled-in shader sources match the GLSL files.'
} elseif ($content -cne $existing) {
    [System.IO.File]::WriteAllText($destination, $content, [System.Text.UTF8Encoding]::new($false))
}
