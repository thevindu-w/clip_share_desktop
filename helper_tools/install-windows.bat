@ECHO OFF

NET SESSION >NUL 2>&1
IF %ERRORLEVEL% == 0 (
    ECHO This installer must not be run as administrator.
    ECHO Please run without admin privileges.
    ECHO Installation aborted.
    IF NOT "%~1" == "NO_PAUSE" PAUSE
    EXIT /B 1
)

IF NOT EXIST clip-share-client*.exe (
    ECHO 'clip-share-client.exe' file does not exist.
    ECHO Please download the 'clip-share-client.exe' Windows version and place it in this folder.
    ECHO Install failed.
    IF NOT "%~1" == "NO_PAUSE" PAUSE
    EXIT /B 1
)

ECHO This will install ClipShare-desktop to run on startup.
SET /P confirm=Proceed? [y/N] 
IF /I NOT "%confirm%" == "y" (
    ECHO Aborted.
    ECHO You can still use clip-share-client by manually running the program.
    IF NOT "%~1" == "NO_PAUSE" PAUSE
    EXIT /B 0
)

RENAME clip-share-client*.exe clip-share-client.exe
clip-share-client.exe -s
MKDIR "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\" >NUL 2>&1
COPY clip-share-client.exe "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\clip-share-client.exe" >NUL 2>&1

CD %USERPROFILE%
IF NOT EXIST clipshare-desktop.conf (
    MKDIR "%USERPROFILE%\Downloads" >NUL 2>&1
    ECHO working_dir=%USERPROFILE%\Downloads>clipshare-desktop.conf
    ECHO Created new configuration file %USERPROFILE%\clipshare-desktop.conf
)
ECHO Installed ClipShare to run on startup.

SET /P start_now=Start ClipShare now? [y/N] 
IF /I "%start_now%" == "y" (
    START "" "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\clip-share-client.exe"
    ECHO Started ClipShare desktop.
)
IF NOT "%~1" == "NO_PAUSE" PAUSE
EXIT /B 0
