rem set fuse
D:\avr\avrdude-6.1-mingw32\avrdude.exe -c usbasp -p t13 -B12 -U lfuse:w:0x79:m


rem read fuse
D:\avr\avrdude-6.1-mingw32\avrdude.exe -c usbasp -p t13 -B12 -U lfuse:r

pause

