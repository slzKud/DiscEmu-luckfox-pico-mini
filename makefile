TOOLCHAIN_PREFIX=/home/slzkud/new_sdk/luckfox-pico/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-
U8G2_PREFIX=/home/slzkud/new_sdk/luckfox_project/libu8g2arm/install
BOOST_PREFIX=/home/slzkud/new_sdk/luckfox_project/boost_1_88_0/install

CXXFLAGS+=-I${U8G2_PREFIX}/include -I${BOOST_PREFIX}/include --std=c++17
CFLAGS+= -I${U8G2_PREFIX}/include -I${BOOST_PREFIX}/include
LDFLAGS+= -L${U8G2_PREFIX}/lib -L${BOOST_PREFIX}/lib --static --std=c++17
LDLIBS+=-l:libu8g2arm.a -l:libboost_filesystem.a -l:libboost_system.a

ifeq (,$(I2C_DISPLAY))
I2C_DISPLAY=1
endif

ifeq (,$(KEYPAD_INPUT))
KEYPAD_INPUT=1
endif

ifeq (,$(USB_ON))
USB_ON=1
endif

ifeq (1,$(USB_ON))
CXXFLAGS+= -DUSB_ON
CFLAGS+= -DUSB_ON
endif

ifeq (1,$(KEYPAD_INPUT))
CXXFLAGS+= -DI2C_DISPLAY
CFLAGS+= -DI2C_DISPLAY
endif

ifeq (1,$(KEYPAD_INPUT))
CXXFLAGS+= -DKEYPAD_INPUT
CFLAGS+= -DKEYPAD_INPUT
endif

CC = $(TOOLCHAIN_PREFIX)gcc
CXX = $(TOOLCHAIN_PREFIX)g++

default: disc-emu

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

disc-emu: main.o menu.o input.o usb.o network.o util.o
	$(CXX) $(LDFLAGS)  main.o menu.o input.o usb.o network.o util.o $(LDLIBS) -o disc-emu

clean:
	rm *.o disc-emu