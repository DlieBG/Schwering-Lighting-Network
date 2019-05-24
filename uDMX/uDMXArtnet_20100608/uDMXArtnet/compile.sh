#### compile_win32.bat
#rm artnet-monitor.exe
rm uDMXArtnet

#### SET THESE DIRECTORIES as needed !
INC_ARTNET=/home/Projekte/libartnet-win32/Develope/libartnet
LIB_ARTNET=/usr/local/lib
#### needed for FLTK
INC_LIBFLTK=/home/Projekte/Libs/fltk-1.3.x-r6777
LIB_LIBFLTK=/home/Projekte/Libs/fltk-1.3.x-r6777/lib
#### needed for USB
INC_LIBUSB=/home/Projekte/Libs/libusb-0.1.12/src
LIB_LIBUSB=/home/Projekte/Libs/libusb-0.1.12

echo '=== Compiling libartnet-win32 examples'
echo ' Include path ' $INC_ARTNET ' and ' $INC_TVISION
echo ' Library path ' $LIB_ARTNET ' and ' $LIB_TVISION
echo ' YOU MIGHT NEED TO CHANGE THESE PATHS '
echo


echo 'compiling uDMXArtnet ...'
g++ -Wall \
  -I $INC_ARTNET \
  -I $INC_LIBFLTK \
  -I $INC_LIBUSB \
  ./uDMXArtnet.cpp uDMX_fkt.cpp -o uDMXArtnet \
  -L $LIB_ARTNET -Wl,-Bstatic -lartnet \
  -L $LIB_LIBUSB -lusb \
  -L $LIB_LIBFLTK \
  -lfltk -lfltk_forms -lfltk_gl -lfltk_images \
  -Wl,-Bdynamic -L /usr/X11R6/lib \
  -lXext -lXft -lfontconfig -lXinerama -ldl -lm  -lX11 -lXpm \
  -lpthread

echo 'finished compiling'
