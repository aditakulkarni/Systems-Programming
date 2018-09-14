all : memory_manager.o bitmap.o memory_manager.a

memory_manager.o : memory_manager.c
	gcc -c memory_manager.c

bitmap.o : bitmap.c
	gcc -c bitmap.c

memory_manager.a : memory_manager.o bitmap.o
	ar rcs memory_manager.a memory_manager.o bitmap.o

clean :
	rm memory_manager.o bitmap.o memory_manager.a
