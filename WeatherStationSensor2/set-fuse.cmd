set AVRDUDE=C:\avr\avrdude-6.3-mingw32\avrdude.exe

rem set fuse
%AVRDUDE% -c usbasp -p t44 -B12 -U hfuse:w:0xDF:m -U lfuse:w:0xE2:m

rem read fuse
rem %AVRDUDE% -c usbasp -p t44 -B12

rem set lock bit
rem %AVRDUDE% -c usbasp -p t44 -B12 -U lock:w:0xFF:m

pause

