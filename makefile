include config.mk

all: 
	@for dir in $(BUILD_DIR); \
	do \
		make -C $$dir; \
	done

clean:
	rm -rf app/link_obj app/dep andromeda
	rm -rf signal/*.gch app/*.gch
	rm -rf $(BUILD_ROOT)/log.txt

memcheck:
	valgrind --tool=memcheck --leak-check=full --show-reachable=yes --trace-children=yes --log-file=$(BUILD_ROOT)/log.txt ./andromeda

kill:
	rm -rf logs/error.log
	@for pid in $(shell pgrep andromeda); \
	do \
		kill -9 $$pid; \
	done

protoc:
	protoc -I=/projects/andromeda-linux-cpp/proto --cpp_out=./generated /projects/andromeda-linux-cpp/proto/*.proto

# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib