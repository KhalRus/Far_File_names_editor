cls
@echo off
Brc32 -R FNEdit.rc
bcc32 -c -a1 -R- -RT- -M- -v- FNEdit.cpp
tlink32 -s -m -M -Tpd -aa -v- FNEdit.obj, FNEdit.dll, , import32 cw32,, FNEdit.res
del *.map
del *.obj
del *.res