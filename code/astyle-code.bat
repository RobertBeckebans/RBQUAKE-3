REM astyle.exe -v --formatted --options=astyle-header.ini --exclude="code/libs" --recursive *.h
REM astyle.exe -v --formatted --options=astyle-cpp.ini --exclude="code/libs" --recursive *.c
REM astyle.exe -v --formatted --options=astyle-cpp.ini --exclude="code/libs" --recursive *.cpp
REM astyle.exe -v --formatted --options=astyle-cpp.ini --exclude="code/libs" --recursive *.glsl
REM astyle.exe -v --formatted --options=astyle-cpp.ini --exclude="code/libs" --recursive *.comp
REM astyle.exe -v --formatted --options=astyle-cpp.ini --exclude="code/libs" --recursive *.vert
REM astyle.exe -v --formatted --options=astyle-cpp.ini --exclude="code/libs" --recursive *.frag

astyle.exe -v --formatted --options=astyle-header.ini shared\q_shared.h
astyle.exe -v --formatted --options=astyle-header.ini engine\renderer\tr_local.h

pause