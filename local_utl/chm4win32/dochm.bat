@echo off
for %%i in (*.tgz) do (
        gzip -dc %%i|tar xf -
        cd %%~ni
        ..\hhc.exe %%~ni.hhp
        copy %%~ni.chm ..
        cd ..
        rmdir /s /q %%~ni
)
echo on

