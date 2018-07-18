# Configure the rpi-rgb-led-matrix library here:
# The HARDWARE_DESC=adafruit-hat value configures the library to use the
# Adafruit LED matrix HAT wiring, and the -DRGB_SLOWDOWN_GPIO=1 value configures
# the library to work with a Raspberry Pi 2.  For a Pi 1 (or perhaps even on
# a Pi 2,  but I found it necessary in my testing) you can remove the
# -DRGB_SLOWDOWN_GPIO=1 option (You also can set this value at runtime via the
# command line option --led-slowdown-gpio=1).
# You can also add any other rpi-rgb-led-matrix library defines here
# to configure the library for more special needs.  See the library's docs for
# details on options:
#   https://github.com/hzeller/rpi-rgb-led-matrix/blob/master/lib/Makefile
export HARDWARE_DESC=adafruit-hat
export USER_DEFINES=-DRGB_SLOWDOWN_GPIO=1

# Configure compiler and libraries:
CXX = g++
CXXFLAGS = -Wall -std=c++11 -O3 -I. -I./rpi-rgb-led-matrix/include -I/opt/vc/include -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host -I/opt/vc/include/interface/vmcs_host/linux -L./rpi-rgb-led-matrix/lib -L/opt/vc/lib
DISPLAY_LIBS = -lrgbmatrix -lrt -lm -lpthread -lbcm_host -lconfig++
SOUND_LIBS = -lasound -lsndfile
FFT_LIBS = -ldl

# Makefile rules:
all: microphone-test display-test fft-test

microphone-test: microphone-test.o Microphone.o
	$(CXX) -o $@ $^ $(CXXFLAGS) $(SOUND_LIBS)

display-test: display-test.o Bitmap.o BitmapSet.o GridTransformer.o Microphone.o FFT.o mailbox.o gpu_fft.o gpu_fft_base.o gpu_fft_shaders.o gpu_fft_twiddles.o Config.o glcdfont.o ./rpi-rgb-led-matrix/lib/librgbmatrix.a
	$(CXX) -o $@ $^ $(CXXFLAGS) $(DISPLAY_LIBS) $(SOUND_LIBS) $(FFT_LIBS)
	
fft-test: fft-test.o FFT.o mailbox.o gpu_fft.o gpu_fft_base.o gpu_fft_shaders.o gpu_fft_twiddles.o
	$(CXX) -o $@ $^ $(CXXFLAGS) $(FFT_LIBS)

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

./rpi-rgb-led-matrix/lib/librgbmatrix.a:
	$(MAKE) -C ./rpi-rgb-led-matrix/lib

.PHONY: clean

clean:
	rm -f *.o rpi-fb-matrix display-test microphone-test fft-test
	$(MAKE) -C ./rpi-rgb-led-matrix/lib clean
