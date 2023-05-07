reg delete "HKCU\Software\Classes\Directory\shell\Open WSL here" /f
reg delete "HKCU\Software\Classes\Directory\Background\shell\Open WSL here" /f
reg delete "HKCU\Software\Classes\LibraryFolder\Background\shell\Open WSL here" /f
echo "Removed Configurations."
pause
