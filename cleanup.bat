@echo off

setlocal
pushd .

:args
rem echo [A] [%_cl_quickmode%] [%_cl_supercleanmode%] %0 .%1 .%2 .%3 .%4 .%5 .%6 .%7 .%8 .%9
if "%1" == "/q" ( shift /1 & ( set _cl_quickmode=1 ) & goto args ) else ( if "%_cl_quickmode%" == "" set _cl_quickmode=0 )
if "%1" == "/Q" ( shift /1 & ( set _cl_quickmode=1 ) & goto args ) else ( if "%_cl_quickmode%" == "" set _cl_quickmode=0 )
rem echo [B] [%_cl_quickmode%] [%_cl_supercleanmode%] %0 .%1 .%2 .%3 .%4 .%5 .%6 .%7 .%8 .%9
if "%1" == "/s" ( shift /1 & ( set _cl_supercleanmode=1 ) & goto args ) else ( if "%_cl_supercleanmode%" == "" set _cl_supercleanmode=0 )
if "%1" == "/S" ( shift /1 & ( set _cl_supercleanmode=1 ) & goto args ) else ( if "%_cl_supercleanmode%" == "" set _cl_supercleanmode=0 )
rem echo [E] [%_cl_quickmode%] [%_cl_supercleanmode%] %0 .%1 .%2 .%3 .%4 .%5 .%6 .%7 .%8 .%9
if "%1" == "/h" ( shift /1 & ( set _cl_helpmode=1 ) & goto args ) else ( if "%_cl_helpmode%" == "" set _cl_helpmode=0 )
if "%1" == "/H" ( shift /1 & ( set _cl_helpmode=1 ) & goto args ) else ( if "%_cl_helpmode%" == "" set _cl_helpmode=0 )
if "%1" == "-h" ( shift /1 & ( set _cl_helpmode=1 ) & goto args ) else ( if "%_cl_helpmode%" == "" set _cl_helpmode=0 )
if "%1" == "-H" ( shift /1 & ( set _cl_helpmode=1 ) & goto args ) else ( if "%_cl_helpmode%" == "" set _cl_helpmode=0 )
if "%1" == "/?" ( shift /1 & ( set _cl_helpmode=1 ) & goto args ) else ( if "%_cl_helpmode%" == "" set _cl_helpmode=0 )
if "%1" == "-?" ( shift /1 & ( set _cl_helpmode=1 ) & goto args ) else ( if "%_cl_helpmode%" == "" set _cl_helpmode=0 )
if "%1" == "--help" ( shift /1 & ( set _cl_helpmode=1 ) & goto args ) else ( if "%_cl_helpmode%" == "" set _cl_helpmode=0 )

if "%_cl_helpmode%" == "1" goto help_em



for /d %%i in ( %1 ) do ( cd /d "%%~fi" )



@echo remove temporary files...

if "%_cl_quickmode%" == "1" goto quick

attrib -r *.bak /s   2> nul:
rem attrib -r *.map /s              -- many Unix source trees have opt and map for different purposes :-(
attrib -r *.bsc /s   2> nul:
attrib -r *.sbr /s   2> nul:
attrib -r *.ncb /s   2> nul:
attrib -r *.pch /s   2> nul:
attrib -r *.idb /s   2> nul:
attrib -r *.pdb /s   2> nul:
attrib -r *.bce /s   2> nul:
rem attrib -r *.cod /s   -- also seems to match *.code, which is used by FCKeditor smaples :-(
attrib -r *.pbi /s   2> nul:
attrib -r *.pbo /s   2> nul:
attrib -r *.pbt /s   2> nul:
rem attrib -r *.opt /s   
attrib -r *.ilk /s   2> nul:
attrib -r *.sup /s   2> nul:
attrib -r *.~ /s   2> nul:
attrib -r *.~? /s   2> nul:
attrib -r *.~?? /s   2> nul:
attrib -r *.~??? /s   2> nul:

:quick

if "%_cl_supercleanmode%" == "" goto superclean

:domythang

del *.obj *.sbr *.bak *.bsc *.ncb *.pch *.idb *.pdb *.bce *.pbi *.pbo *.pbt /s   2> nul:
del *.plg *.ilk *.sup /s   2> nul:
del *.~ *.~? *.~?? *.~??? /s   2> nul:
del *.tlh *.tli *.tlb /s   2> nul:
rem del *.map *.opt *.cod /s   2> nul:

goto ende



:superclean

rem also delete any possible targets

del *.exe *.dll *.lib *.res *.ocx /s   2> nul:

goto domythang


goto ende



:help_em

echo %~nx0 {args} [<path>]
echo.
echo args:
echo  /q /s
echo.
echo.
echo remove temporary files in the current directory and all subdirectories.
echo.
echo /q : quick mode = do NOT remove RHS attributes on all the files 
echo      (recursively) before trying to delete them
echo.
echo /s : superclean : also remove any build targets (DLL, EXE, LIB, RES, OCX)
echo      in the current directory and all subdirectories.
echo. 


goto ende




:ende

popd 
endlocal



