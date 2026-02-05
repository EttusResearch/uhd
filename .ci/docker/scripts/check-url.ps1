param(
    [Parameter(Mandatory = $true)]
    [string]$Url
)

if ([string]::IsNullOrWhiteSpace($Url)) {
    throw "URL check failed: URL parameter is empty or null. Please provide a valid VS Build Tools URL via the VS_BUILD_TOOLS_URL build argument."
}

$resp = Invoke-WebRequest -Uri $Url -UseBasicParsing -Method Head
if ($resp.StatusCode -ne 200) {
    throw "URL check failed: StatusCode=$($resp.StatusCode)"
}

$ct = $resp.Headers['Content-Type']
$cl = [int64]($resp.Headers['Content-Length'] | Select-Object -First 1)

if ($ct -and $ct -match 'text/html') {
    throw "URL check failed: Content-Type=$ct"
}

if ($cl -and $cl -lt 1048576) {
    throw "URL check failed: Content-Length=$cl"
}
