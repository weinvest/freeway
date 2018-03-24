INCLUDE=-I/home/jz/boost_install_dir/include/ -I./include -I.. -I/home/jz/tmp/boost/libs/lockfree/include/

LIB_DIR=/home/jz/boost_install_dir/lib/
STATIC_LIB=$(LIB_DIR)/libboost_context.a $(LIB_DIR)/libboost_thread.a $(LIB_DIR)/libboost_system.a

main: main.cpp libframework.a
	g++ main.cpp libframework.a -std=c++14 -g3 -o $@  $(INCLUDE) $(STATIC_LIB) -ltbb -lpthread

OBJ = Task.o Worker.o Dispatcher.o DEventNode.o Context.o
all: $(OBJ)

clean:
	rm $(OBJ) libframework.a main core.*



Task.o:Task.cpp
	g++ -c $^ -std=c++14 -g3 -o $@  $(INCLUDE)
Worker.o:Worker.cpp
	g++ -c $^ -std=c++14 -g3 -o $@  $(INCLUDE)
Dispatcher.o:Dispatcher.cpp
	g++ -c $^ -std=c++14 -g3 -o $@  $(INCLUDE)
DEventNode.o:DEventNode.cpp
	g++ -c $^ -std=c++14 -g3 -o $@  $(INCLUDE)
Context.o:Context.cpp
	g++ -c $^ -std=c++14 -g3 -o $@  $(INCLUDE)

libframework.a:$(OBJ)
	ar rvc libframework.a $(OBJ)

