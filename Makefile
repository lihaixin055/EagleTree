
# EagleTree makefile

CC = /usr/bin/gcc
CFLAGS = -std=c++0x -g -w 
CXX = /usr/bin/g++
CXXFLAGS = $(CFLAGS)
ELF0 = run_test
ELF1 = run_trace
HDR = ssd.h block_management.h lsm_tree_manager.h
VPATH = FTLs MTRand BlockManagers OperatingSystem Utilities Scheduler
SRC = Logarithmic_Gecko_garbage_collector.cpp page_ftl_in_flash.cpp k_modal_group.cpp bm_k_modal_groups.cpp ftl_parent.cpp special_events_queue.cpp LSM_FTL.cpp bm_gc_locality.cpp StatisticData.cpp bm_tags.cpp OS_Schedulers.cpp Queue_Length_Statistics.cpp experiment_graphing.cpp experiment_result.cpp Individual_Threads_Statistics.cpp Migrator.cpp Free_Space_Meter.cpp Utilization_Meter.cpp Workload_Definitions.cpp Garbage_Collector_Greedy.cpp Garbage_Collector_LRU.cpp Garbage_Collector_LRU2.cpp Scheduling_Strategies.cpp events_queue.cpp wear_leveling_strategy.cpp grace_hash_join.cpp page_ftl.cpp DFTL.cpp FAST.cpp address.cpp block.cpp config.cpp die.cpp event.cpp package.cpp page.cpp plane.cpp ssd.cpp scheduler.cpp bm_shortest_queue.cpp page_hotness_measurer.cpp bm_locality.cpp  bm_hot_cold_seperation.cpp bm_parent.cpp visual_tracer.cpp state_visualiser.cpp statistics_gatherer.cpp operating_system.cpp thread_implementations.cpp sequential_pattern_detector.cpp mtrand.cpp external_sort.cpp bm_round_robin.cpp File_Manager.cpp random_order_iterator.cpp experiment_runner.cpp flexible_reader.cpp
OBJ = Logarithmic_Gecko_garbage_collector.o page_ftl_in_flash.o k_modal_group.o bm_k_modal_groups.o ftl_parent.o special_events_queue.o LSM_FTL.o bm_gc_locality.o StatisticData.o bm_tags.o OS_Schedulers.o Queue_Length_Statistics.o experiment_graphing.o experiment_result.o Individual_Threads_Statistics.o Migrator.o Free_Space_Meter.o Utilization_Meter.o Workload_Definitions.o Garbage_Collector_Greedy.o Garbage_Collector_LRU.o Garbage_Collector_LRU2.o Scheduling_Strategies.o events_queue.o wear_leveling_strategy.o grace_hash_join.o page_ftl.o address.o block.o config.o die.o DFTL.o FAST.o event.o package.o page.o plane.o ssd.o scheduler.o bm_shortest_queue.o page_hotness_measurer.o bm_locality.o bm_hot_cold_seperation.o bm_parent.o visual_tracer.o state_visualiser.o statistics_gatherer.o operating_system.o thread_implementations.o sequential_pattern_detector.o mtrand.o external_sort.o bm_round_robin.o File_Manager.o random_order_iterator.o experiment_runner.o flexible_reader.o
PERMS = 660
EPERMS = 770

all: demo demo1 demo2

demo: $(HDR) $(OBJ)
	$(CXX) $(CXXFLAGS) -o Experiments/demo Experiments/demo.cpp $(OBJ) -lboost_serialization
	-chmod $(PERMS) $(OBJ) 
	-chmod $(EPERMS) Experiments/demo

demo1: $(HDR) $(OBJ)
	$(CXX) $(CXXFLAGS) -o Experiments/demo1 Experiments/demo1.cpp $(OBJ) -lboost_serialization
	-chmod $(PERMS) $(OBJ) 
	-chmod $(EPERMS) Experiments/demo1
	
demo2: $(HDR) $(OBJ)
	$(CXX) $(CXXFLAGS) -o Experiments/demo2 Experiments/demo2.cpp $(OBJ) -lboost_serialization
	-chmod $(PERMS) $(OBJ) 
	-chmod $(EPERMS) Experiments/demo2

clean:
	-rm -f $(OBJ) $(LOG) $(ELF0) $(ELF1) $(ELF2) Experiments/demo Experiments/demo2

files:
	echo $(SRC) $(HDR)
