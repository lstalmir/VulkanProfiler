@echo OFF

REM Get script directory
SET LAYER_DIR=%~dp0
SET JSON_PATH="%LAYER_DIR%VkLayer_profiler_layer.json"

SET QUIET=0
SET VERBOSE=0
SET UNINSTALL=0
SET INSTALL_TARGET=
SET OPTIONS=

REM Parse arguments
:PARSE_ARGS
IF NOT "%1" EQU "" (
    IF "%1" EQU "--quiet" (SET QUIET=1)
    IF "%1" EQU "--verbose" (SET VERBOSE=1)
    IF "%1" EQU "--uninstall" (SET UNINSTALL=1)
    IF "%1" EQU "--allUsers" (SET INSTALL_TARGET=HKLM)
    IF "%1" EQU "--currentUser" (SET INSTALL_TARGET=HKCU)
    IF "%1" EQU "--path" (SET JSON_PATH="%2"; SHIFT)
    SHIFT
    GOTO PARSE_ARGS )

REM Convert path to absolute
PUSHD .
CD %JSON_PATH%\..
SET JSON_PATH=%CD%\VkLayer_profiler_layer.json
POPD

IF %VERBOSE% EQU 1 (
    ECHO JSON_PATH = %JSON_PATH% )
    
IF NOT EXIST %JSON_PATH% (
    ECHO VkLayer_profiler_layer.json not found
    EXIT /B 2 )

IF %UNINSTALL% EQU 0 ( CALL :INSTALL ) ELSE ( CALL :UNINSTALL )
EXIT /B %ERRORLEVEL%

REM Update INSTALL_TARGET
:SELECT_INSTALL_TARGET
SET /P TARGET_HKLM=Install for all users? ([y]/n) || ^
    SET TARGET_HKLM=y

IF /I "%TARGET_HKLM%" EQU "y" (
    SET INSTALL_TARGET=HKLM
) ELSE IF /I "%TARGET_HKLM%" EQU "n" (
    SET INSTALL_TARGET=HKCU
) ELSE ( EXIT /B 1 )

EXIT /B 0
REM END SELECT_INSTALL_TARGET 

REM Install the layer
:INSTALL

IF %QUIET% EQU 1 (
    SET OPTIONS=/F
    IF "%INSTALL_TARGET%" EQU "" (
        ECHO Installation scope must be defined when using --quiet option
        ECHO Use --allUsers or --currentUser
        EXIT /B 1 )
) ELSE IF "%INSTALL_TARGET%" EQU "" (
    CALL :SELECT_INSTALL_TARGET )

IF %ERRORLEVEL% EQU 0 (
    IF %VERBOSE% EQU 1 (
        ECHO Installing to %INSTALL_TARGET%\Software\Khronos\Vulkan\ExplicitLayers
        ECHO   %JSON_PATH% = REG_DWORD 0 )

    REG ADD %INSTALL_TARGET%\Software\Khronos\Vulkan\ExplicitLayers ^
        %OPTIONS% ^
        /V %JSON_PATH% ^
        /T REG_DWORD ^
        /D 0 )
        
IF %ERRORLEVEL% NEQ 0 ( ECHO Failed to install layer )
EXIT /B %ERRORLEVEL%
REM END INSTALL

REM Uninstall the layer
:UNINSTALL

IF %QUIET% EQU 1 (
    SET OPTIONS=/F)

IF %VERBOSE% EQU 1 ( ECHO Uninstalling from HKCU\Software\Khronos\Vulkan\ExplicitLayers )
REG DELETE HKCU\Software\Khronos\Vulkan\ExplicitLayers %OPTIONS% /V %JSON_PATH%

IF %VERBOSE% EQU 1 ( ECHO Uninstalling from HKLM\Software\Khornos\Vulkan\ExplicitLayers )
REG DELETE HKLM\Software\Khronos\Vulkan\ImplicitLayers %OPTIONS% /V %JSON_PATH%

EXIT /B 0
REM END UNINSTALL
