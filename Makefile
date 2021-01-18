CXX = g++
CXXFLAGS = -W -Wall -std=c++11 -O3 -I./include -I.
LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system
 
COMMONSRCFILES=$(wildcard src_common/*.cpp) $(wildcard src_common/*/*.cpp)
BUILDERSRCFILES=$(wildcard src_builder/*.cpp) $(wildcard src_builder/*/*.cpp)
RENDERERSRCFILES=$(wildcard src_renderer/*.cpp) $(wildcard src_renderer/*/*.cpp)
ALLSRCFILES=$(COMMONSRCFILES) $(BUILDERSRCFILES) $(RENDERERSRCFILES)

CPPFILESBUILDER=$(COMMONSRCFILES:src_common/%=%) $(BUILDERSRCFILES:src_builder/%=%)
CPPFILESRENDERER=$(COMMONSRCFILES:src_common/%=%) $(RENDERERSRCFILES:src_renderer/%=%)

OBJSBUILDER=$(CPPFILESBUILDER:%.cpp=obj/%.o)
OBJSRENDERER=$(CPPFILESRENDERER:%.cpp=obj/%.o)

ECECRENDERER=maprenderer
EXECBUILDER=mapbuilder

all : builder renderer

renderer : bin/$(ECECRENDERER) 

builder : bin/$(EXECBUILDER) 

bin/maprenderer : $(OBJSRENDERER)
	mkdir -p ./bin
	$(CXX) -o $@ $^ $(LDFLAGS) 
 
bin/mapbuilder : $(OBJSBUILDER)
	mkdir -p ./bin
	$(CXX) -o $@ $^ $(LDFLAGS) 
	
obj/%.o : src_common/%.cpp
	mkdir -p ./obj
	$(CXX) $(CXXFLAGS) -o $@ -c $<
	
obj/%.o : src_builder/%.cpp
	mkdir -p ./obj
	$(CXX) $(CXXFLAGS) -o $@ -c $<
	
obj/%.o : src_renderer/%.cpp
	mkdir -p ./obj
	$(CXX) $(CXXFLAGS) -o $@ -c $<
	
clean :
	@rm obj/*.o
	
cleaner : clean
	@rm bin/$(EXECBUILDER)

