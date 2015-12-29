@rem by Roc
@rem
@rem

if exist "%VS120COMNTOOLS%" (
	set VCVARS="%VS120COMNTOOLS%..\..\VC\bin\"
	goto build
) else (
	goto check2012
)

:check2012
if exist "%VS110COMNTOOLS%" (
	set VCVARS="%VS110COMNTOOLS%..\..\VC\bin\"
) else (
	goto missing
)

:build

@set ENV32="%VCVARS%vcvars32.bat"
@set ENV64="%VCVARS%amd64\vcvars64.bat"

@set LAOI_COMPILE=cl /nologo /c /MD /O2 /W3 /D_CRT_SECURE_NO_DEPRECATE
@set LAOI_LINK=link /nologo
@set LAOI_MT=mt /nologo
@set LAOI_LIB=lib /nologo
@set LIB_NAME=aoi
@set LIBOBJS=aoi.obj


@call "%ENV32%"
@call :realbuild
@del %LIB_NAME%32.dll
@del %LIB_NAME%32.lib
@rename %LIB_NAME%.dll %LIB_NAME%32.dll
@rename %LIB_NAME%.lib %LIB_NAME%32.lib
@del *.obj
@pause


@call "%ENV64%"
@call :realbuild
@del %LIB_NAME%64.dll
@del %LIB_NAME%64.lib
@rename %LIB_NAME%.dll %LIB_NAME%64.dll
@rename %LIB_NAME%.lib %LIB_NAME%64.lib
@del *.obj
@pause

@exit 0

:realbuild

%LAOI_COMPILE% *.c
%LAOI_LINK% /DLL /OUT:%LIB_NAME%.dll %LIBOBJS%
%LAOI_LIB% /OUT:%LIB_NAME%.lib %LIBOBJS%

