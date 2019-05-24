#### compile.bat
rm uDMXArtnet.exe

#### SET THESE DIRECTORIES as needed !
INC_ARTNET=/C/Projekte/libartnet-win32/Develope/libartnet
#LIB_ARTNET=/usr/local/lib
LIB_ARTNET=/C/Projekte/libartnet-win32/Develope/libartnet/artnet/.libs
#### needed for USB
INC_LIBUSB=/C/Projekte/Libs/libusb-win32/src
LIB_LIBUSB=/C/Projekte/Libs/libusb-win32
#### needed for FLTK
INC_LIBFLTK=/C/Projekte/Libs/fltk-1.3.x-r6777
LIB_LIBFLTK=/C/Projekte/Libs/fltk-1.3.x-r6777/lib


echo '=== Compiling uDMXArtnet'
echo ' Include path ' $INC_ARTNET
echo ' Library path ' $LIB_ARTNET
echo ' YOU MIGHT NEED TO CHANGE THESE PATHS '
echo

echo '= making resources'
windres uDMXArtnet.rc uDMXArtnetres.o

echo '= making executable'
g++ -Wall -mwindows -DWIN32 \
  -I $INC_ARTNET \
  -I $INC_LIBFLTK \
  -I $INC_LIBUSB \
  uDMXArtnet.cpp uDMX_fkt.cpp uDMXArtnetres.o -o uDMXArtnet.exe \
  -L $LIB_LIBFLTK \
  -lfltk -lfltk_forms -lfltk_gl -lfltk_images \
  -lfltk_jpeg -lfltk_png -lfltk_z \
  -L $LIB_ARTNET -lartnet \
  -L $LIB_LIBUSB -lusb \
  -lole32 -luuid -lcomctl32 -lwsock32 -lnetapi32 -liphlpapi
 
echo 'finished compiling'