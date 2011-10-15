call "%VS80COMNTOOLS%vsvars32.bat"
devenv chess.sln /Rebuild "Release-Qt|Win32" /project qchess /projectconfig "Release-Qt|Win32" /Log
devenv chess.sln /Rebuild "Release|Win32" /project tequilla /projectconfig "Release|Win32" /Log