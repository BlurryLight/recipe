reg add "HKCU\Software\Classes\Directory\shell\Open MSYS2UCRT here" /v icon /d %LOCALAPPDATA%\terminal\ucrt64.ico /f
reg add "HKCU\Software\Classes\Directory\shell\Open MSYS2UCRT here" /v Extended /d "" /f
reg add "HKCU\Software\Classes\Directory\shell\Open MSYS2UCRT here\command" /d "\"%LOCALAPPDATA%\Microsoft\WindowsApps\wt.exe\" -d . -p \"MSYS2_UCRT\" " /f
reg add "HKCU\Software\Classes\Directory\Background\shell\Open MSYS2UCRT here" /v icon /d %LOCALAPPDATA%\terminal\ucrt64.ico /f
reg add "HKCU\Software\Classes\Directory\Background\shell\Open MSYS2UCRT here" /v Extended /d "" /f
reg add "HKCU\Software\Classes\Directory\Background\shell\Open MSYS2UCRT here\command" /d "\"%LOCALAPPDATA%\Microsoft\WindowsApps\wt.exe\" -d . -p \"MSYS2_UCRT\" "  /f
reg add "HKCU\Software\Classes\LibraryFolder\Background\shell\Open MSYS2UCRT here" /v icon /d %LOCALAPPDATA%\terminal\ucrt64.ico /f
reg add "HKCU\Software\Classes\LibraryFolder\Background\shell\Open MSYS2UCRT here" /v Extended /d "" /f
reg add "HKCU\Software\Classes\LibraryFolder\Background\shell\Open MSYS2UCRT here\command" /d "\"%LOCALAPPDATA%\Microsoft\WindowsApps\wt.exe\" -d . -p \"MSYS2_UCRT\" "  /f
robocopy ./ %LOCALAPPDATA%\terminal ucrt64.ico /E /IS /IT
echo "Context Menu For Windows Terminal is Configured Successfully! Try Right Clicking to see the option."
pause