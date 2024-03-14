# This script is used for windows VS2022

$current_path= (Get-Location).path

# If not found vckpkg,Install vcpkg
if (!(Get-Command vcpkg -ErrorAction SilentlyContinue)) {
  Write-Host -ForegroundColor:Yellow "If it is the first time to install vcpkg, it may take several minutes. Please wait..."

  cd c:\
  git clone https://github.com/Microsoft/vcpkg.git
  cd vcpkg
  .\bootstrap-vcpkg.bat

  $path = [Environment]::GetEnvironmentVariable('Path', 'Machine')
  $newpath = 'c:\vcpkg;' + $path 
  [Environment]::SetEnvironmentVariable('Path', $newpath, 'Machine')
  [Environment]::SetEnvironmentVariable('VCPKG_ROOT', 'c:\vcpkg', 'Machine')
}

if (!(Get-Command vcpkg -ErrorAction SilentlyContinue)) {
  Write-Host -ForegroundColor:Red "Install vcpkg failed !!!"

} else {
cd $current_path
# update baseline
# --add-initial-baseline 
vcpkg x-update-baseline 
vcpkg install

Write-Host -ForegroundColor:Green "Now, you can use CMake by -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake"
}