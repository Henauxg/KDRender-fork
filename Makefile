CXX = g++
CXXFLAGS = -W -Wall -std=c++11 -O3 -I./include -I.
LDFLAGS = 
 
COMMONSRCFILES=$(wildcard src_common/*.cpp) $(wildcard src_common/*/*.cpp)
BUILDERSRCFILES=$(wildcard src_builder/*.cpp) $(wildcard src_builder/*/*.cpp)
RENDERERSRCFILES=$(wildcard src_renderer/*.cpp) $(wildcard src_renderer/*/*.cpp)
ALLSRCFILES=$(COMMONSRCFILES) $(BUILDERSRCFILES) $(RENDERERSRCFILES)

CPPFILES=$(COMMONSRCFILES:src_common/%=%) $(BUILDERSRCFILES:src_builder/%=%) $(RENDERERSRCFILES:src_builder/%=%)
OBJS=$(CPPFILES:%.cpp=obj/%.o)

EXECBUILDER=mapbuilder
 
builder : bin/$(EXECBUILDER) 
 
bin/mapbuilder : $(OBJS)
	mkdir -p ./bin
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS) -L$(SFML_PATH)/lib
	
obj/%.o : src_common/%.cpp src_builder/%.cpp src_renderer/%.cpp
	mkdir -p ./obj
	$(CXX) $(CXXFLAGS) -o $@ -c $<
	
clean :
	@rm obj/*.o
	
cleaner : clean
	@rm bin/$(EXECBUILDER)

run: bin/$(EXECBUILDER)
	export LD_LIBRARY_PATH=$(SFML_PATH)/lib ; bin/$(EXEC)
