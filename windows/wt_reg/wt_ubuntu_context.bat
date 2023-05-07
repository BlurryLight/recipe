reg add "HKCU\Software\Classes\Directory\shell\Open WSL here" /v icon /d %LOCALAPPDATA%\terminal\wsl_ubuntu.ico /f
reg add "HKCU\Software\Classes\Directory\shell\Open WSL here\command" /d "\"%LOCALAPPDATA%\Microsoft\WindowsApps\wt.exe\" -d . -p Ubuntu " /f
reg add "HKCU\Software\Classes\Directory\Background\shell\Open WSL here" /v icon /d %LOCALAPPDATA%\terminal\wsl_ubuntu.ico /f
reg add "HKCU\Software\Classes\Directory\Background\shell\Open WSL here\command" /d "\"%LOCALAPPDATA%\Microsoft\WindowsApps\wt.exe\" -d . -p Ubuntu "  /f
reg add "HKCU\Software\Classes\LibraryFolder\Background\shell\Open WSL here" /v icon /d %LOCALAPPDATA%\terminal\wsl_ubuntu.ico /f
reg add "HKCU\Software\Classes\LibraryFolder\Background\shell\Open WSL here\command" /d "\"%LOCALAPPDATA%\Microsoft\WindowsApps\wt.exe\" -d . -p Ubuntu "  /f
robocopy ./ %LOCALAPPDATA%\terminal wsl_ubuntu.ico /E /IS /IT
echo "Context Menu For Windows Terminal is Configured Successfully! Try Right Clicking to see the option."
pause