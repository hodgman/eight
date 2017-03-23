set ispcPath=%~dp0

if "%1"=="Debug"   set ispc_cfg_flags=-DeiBUILD_DEBUG    --math-lib=fast --wno-perf -O0
if "%1"=="Dev"     set ispc_cfg_flags=-DeiBUILD_DEV      --math-lib=fast --opt=fast-math
if "%1"=="Retail"  set ispc_cfg_flags=-DeiBUILD_RETAIL   --math-lib=fast --opt=fast-math --opt=disable-assertions

REM throw the first parameter away
shift
set args=%1
:loop
 shift
 if "%1"=="" goto afterloop
 set args=%args% %1
 goto loop
:afterloop

%ispcPath%/ispc.exe %args% %ispc_cfg_flags% --target=sse2 --arch=x86-64 
