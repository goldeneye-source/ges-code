@echo off

if not "%VS120COMNTOOLS%" == "" (
	set VisualStudio_Version=2013
) else (
	if not "%VS110COMNTOOLS%" == "" (
		set VisualStudio_Version=2012
	) else (
		if not "%VS100COMNTOOLS%" == "" (
			set VisualStudio_Version=2010
		) else (
			echo No Visual Studio C++ could be found, please install VS 2013/2012/2010 and try again
			pause
			exit
		)
	)
)

pushd %~dp0
	devtools\bin\vpc.exe /hl2mp +game /%VisualStudio_Version% /mksln game-vs%VisualStudio_Version%.sln
popd
