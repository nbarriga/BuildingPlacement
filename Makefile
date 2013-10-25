CC=g++ -g -rdynamic -Wall
#CC=g++ -O3
SDL_LDFLAGS=`sdl-config --libs` 
SDL_CFLAGS=`sdl-config --cflags` 
CFLAGS=$(SDL_CFLAGS)
LDFLAGS=-lGL -lGLU -lSDL_image $(SDL_LDFLAGS) -lboost_program_options
GALIB_DIR=src/galib
SPARCRAFT_DIR=../sparcraft
INCLUDES=-I$(SPARCRAFT_DIR)/bwapidata/include -I$(SPARCRAFT_DIR)/source -I$(GALIB_DIR)
SOURCES=$(wildcard src/*.cpp)
OBJECTS=$(SOURCES:.cpp=.o)
SPARCRAFT_LIB=$(SPARCRAFT_DIR)/libSparCraft.a

all:BuildPlacement

BuildPlacement:galib $(OBJECTS)
	$(CC) $(OBJECTS) $(SPARCRAFT_LIB) -o $@  $(LDFLAGS) -L$(GALIB_DIR)/ga -lga

.cpp.o:
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@ 
	$(CC) -MM $(CPPFLAGS) $(INCLUDES) -MT $@ -o $*.d $<

.cc.o:
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@
	$(CC) -MM $(CPPFLAGS) $(INCLUDES) -MT $@ -o $*.d $<

galib:
	make -C $(GALIB_DIR) lib

clean:
	make -C $(GALIB_DIR) clean
	rm -f $(OBJECTS) $(OBJECTS:.o=.d) BuildPlacement

-include $(SOURCES:.cpp=.d)

