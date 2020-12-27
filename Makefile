CXX = g++
CXXFLAGS = -W -Wall -std=c++11 -Og -I./include -I.
LDFLAGS = 
 
tCFILES=$(wildcard src/*.cpp) $(wildcard src/*/*.cpp)
CFILES=$(tCFILES:src/%=%)
OBJS=$(CFILES:%.cpp=obj/%.o)

EXEC=mapbuilder
 
all : bin/$(EXEC) 
 
bin/mapbuilder : $(OBJS)
	mkdir -p ./bin
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS) -L$(SFML_PATH)/lib
	
obj/%.o : src/%.cpp
	mkdir -p ./obj
	$(CXX) $(CXXFLAGS) -o $@ -c $<
	
clean :
	@rm obj/*.o
	
cleaner : clean
	@rm bin/$(EXEC)

run: bin/$(EXEC)
	export LD_LIBRARY_PATH=$(SFML_PATH)/lib ; bin/$(EXEC)
	
run_gdb: bin/$(EXEC)
	export LD_LIBRARY_PATH=$(SFML_PATH)/lib ; gdb bin/$(EXEC)

run_valgrind: bin/$(EXEC)
	export LD_LIBRARY_PATH=$(SFML_PATH)/lib ; valgrind --leak-check=full bin/$(EXEC)
