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