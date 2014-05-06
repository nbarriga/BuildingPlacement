CC=g++ -g -rdynamic -Wall  -O3
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
GALIB_LIB=$(GALIB_DIR)/ga/libga.a
SPARCRAFT_LIB=$(SPARCRAFT_DIR)/libSparCraft.a

all:BuildPlacement

BuildPlacement:$(GALIB_LIB) $(OBJECTS) $(SPARCRAFT_LIB)
	$(CC) $(OBJECTS) $(SPARCRAFT_LIB) -o $@  $(LDFLAGS) -L$(GALIB_DIR)/ga -lga

.cpp.o:
	$(CC) -MM $(CPPFLAGS) $(INCLUDES) -MT $@ -o $*.d $<
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@ 

.cc.o:
	$(CC) -MM $(CPPFLAGS) $(INCLUDES) -MT $@ -o $*.d $<
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@

$(SPARCRAFT_LIB): $(SPARCRAFT_DIR)/source/*.cpp $(SPARCRAFT_DIR)/source/*.h $(SPARCRAFT_DIR)/source/*.hpp
	make -C $(SPARCRAFT_DIR) all

$(GALIB_LIB): $(GALIB_DIR)/ga/*.C $(GALIB_DIR)/ga/*.h
	make -C $(GALIB_DIR) lib

clean:
	make -C $(GALIB_DIR) clean
	rm -f $(OBJECTS) $(OBJECTS:.o=.d) BuildPlacement

-include $(SOURCES:.cpp=.d)

