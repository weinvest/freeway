LIB_DIR=/home/jz/boost_install_dir/lib/
INCLUDE=-I/home/jz/boost_install_dir/include/ -I.. -I/home/jz/src/apollo/common/

#STATIC_LIB=$(LIB_DIR)/libboost_context.a $(LIB_DIR)/libboost_thread.a


OBJ = TimerRange.o TimeRangePhase.o Double.o Enviroment.o
all: $(OBJ)
	ar rvc libframework.a $(OBJ)

clean:
	rm $(OBJ)



TimerRange.o: TimeRange.cpp
	g++ -c $^ -std=c++14 $(INCLUDE) $(STATIC_LIB) -g3 -o $@

TimeRangePhase.o: TimeRangePhase.cpp
	g++ -c $^ -std=c++14 $(INCLUDE) $(STATIC_LIB) -g3 -o $@

TimeRangeScheduler.o:TimeRangeScheduler.cpp
	g++ -c $^ -std=c++14 $(INCLUDE) $(STATIC_LIB) -g3 -o $@

Double.o:Double.cpp
	g++ -c $^ -std=c++14 $(INCLUDE) $(STATIC_LIB) -g3 -o $@

DTimer.o:DTimer.cpp
	g++ -c $^ -std=c++14 $(INCLUDE) $(STATIC_LIB) -g3 -o $@

Enviroment.o:Enviroment.cpp
	g++ -c $^ -std=c++14 $(INCLUDE) $(STATIC_LIB) -g3 -o $@
