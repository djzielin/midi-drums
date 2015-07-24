SYS := $(shell gcc -dumpmachine)

ifneq (, $(findstring arm, $(SYS)))
 RASPBERRY=yes
endif

LDFLAGS+=  -ljack -lsndfile -lsamplerate -lasound -lpthread -lfltk -lX11 -lXext -lconfig++
CXXFLAGS+= -O3  -I/usr/include/alsa 

ifdef RASPBERRY
  LDFLAGS  +=-lwiringPi
  CXXFLAGS +=-DRASPBERRY -I/usr/local/include/stk
  LDFLAGS  += -lstk
   
else
  LDFLAGS  +=-L$(wildcard ~/stk-4.4.3/src) -lstk
  CXXFLAGS += -I$(wildcard ~/stk-4.4.3/include)
endif

CXXFLAGS += -DUSE_ALSA

#CXXFLAGS+= -DRECORDING
CXXFLAGS += -DBASSPORT

COMMON_OBJECTS=single_drum.o mono_sample.o simple_envelope.o comb_filter.o single_parameter.o common.o wave_position.o wave_generator.o echo_effect.o simple_parameter.o ../midi-keyboard/audio.o ../midi-keyboard/midi.o

all: $(COMMON_OBJECTS) drums.o
	g++ drums.o $(COMMON_OBJECTS) $(LDFLAGS) -o drums

clean:
	rm -f *.o
	rm -f drums

