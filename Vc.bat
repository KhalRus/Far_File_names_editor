cls
@echo off
rc FNEdit.rc
cl /O1 /Zp2 /LD /Oi FNEdit.cpp /link /opt:nowin98 /noentry /def:FNEdit.def FNEdit.res kernel32.lib advapi32.lib
del *.obj
del *.res
del *.lib
del *.exp