del /S /F *.pch *.pdb *.obj *.sbr *.exp *.lib *.idb *.ncb *.opt *.bsc *.res *.ilk *.scc *.dll *.exe *.suo *.user 
del /S /F /A:H *.suo
rmdir /S /Q debug
rmdir /S /Q release
rmdir /S /Q Release-Qt
rmdir /S /Q Debug-Qt