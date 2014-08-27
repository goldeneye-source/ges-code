:: You will need to make a new Envionment variable to hold the path to your ges mod files 
:: so this batch script can copy the dlls to the right folder
::
:: Start by pressing windows key + page break (which should open system properties)
:: Goto the advanced tab and click on Environment Variables button (lower left)
:: Click on new (user variables for [login]
:: Enter "GES_PATH" (with out the quotes) for the variable name 
:: Enter the path to the mod files for the value (e.g. "D:\steam\steamapps\sourcemods\gesbeta3")
:: Restart Vis and all should be well.


:: %1 = path to compiled files (eg: \src\game\client\Debug_ges)
:: %2 = project (server or client)

echo ------------------------------------------------
echo Compile Path is %1
echo Current Project is %2
echo Mod Files Path is %GES_PATH%
echo ------------------------------------------------

if exist "%GES_PATH%\bin\%2.dll" attrib -r "%GES_PATH%\bin\%2.dll"
if exist "%GES_PATH%\bin\%2.pdb" attrib -r "%GES_PATH%\bin\%2.pdb"

if exist "%1\%2.dll" copy "%1\%2.dll" "%GES_PATH%\bin\%2.dll"
if exist "%1\%2.pdb" copy "%1\%2.pdb" "%GES_PATH%\bin\%2.pdb"
echo ------------------------------------------------