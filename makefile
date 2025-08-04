TOOLCHAIN_PREFIX=/home/slzkud/duo256_project/host-tools-master/gcc/riscv64-linux-musl-x86_64/bin/riscv64-unknown-linux-musl-
U8G2_PREFIX=/home/slzkud/duo256_project/libu8g2arm-milkvduo-256m/install

CXXFLAGS+=-I${U8G2_PREFIX}/include -std=c++17
CFLAGS+= -I${U8G2_PREFIX}/include
LDFLAGS+= -L${U8G2_PREFIX}/lib -static
LDLIBS+=-l:libu8g2arm.a

ifeq (,$(I2C_DISPLAY))
I2C_DISPLAY=0
endif

ifeq (,$(KEYPAD_INPUT))
KEYPAD_INPUT=0
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