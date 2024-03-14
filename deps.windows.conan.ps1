# This script is used for windows VS2022

# If not found python
if (!(Get-Command python -ErrorAction SilentlyContinue)){
  Write-Host -ForegroundColor:Red "Install python first !!!"
  exit
}

# If not found conan,Install conan
if (!(Get-Command conan -ErrorAction SilentlyContinue)){
   pip install conan
}

conan profile detect --force

# Install Release
conan install . --output-folder=build --build=missing `
--settings=build_type=Release --settings=compiler="msvc" --settings=compiler.version=193 --settings=compiler.runtime="dynamic" --settings=compiler.cppstd=14

# Install Debug
conan install . --output-folder=build --build=missing `
--settings=build_type=Debug --settings=compiler="msvc" --settings=compiler.version=193 --settings=compiler.runtime="dynamic" --settings=compiler.cppstd=14 
