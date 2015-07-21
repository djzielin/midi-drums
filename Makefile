LDFLAGS+=-L$(wildcard ~/stk-4.4.3/src) -lstk -ljack -lsndfile -lsamplerate -lfltk -lX11 -lXext -lconfig++
CXXFLAGS+= -O3 -I$(wildcard ~/stk-4.4.3/include) -I$(TINYXML) -DTIXML_USE_STL
#CXXFLAGS+= -DRECORDING

CXXFLAGS += -DBASSPORT

COMMON_OBJECTS=single_drum.o mono_sample.o simple_envelope.o comb_filter.o single_parameter.o common.o wave_position.o wave_generator.o echo_effect.o simple_parameter.o 

all: $(COMMON_OBJECTS) drums.o
	g++ drums.o $(COMMON_OBJECTS) $(LDFLAGS) -o drums

clean:
	rm -f *.o
	rm -f drums

