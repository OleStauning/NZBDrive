CC=g++-8 -O3 -std=c++17
OBJ = NZBFuseDrive.o 


.cpp.o:
	$(CC) -D_FILE_OFFSET_BITS=64 -I. -I/usr/local/include -I../NZBDriveLib -c -o $@ $< $(CFLAGS)


libNZBFuseDrive.a: $(OBJ)
	ar rcs $@ $^


.PHONY: clean

clean:
	rm -f *.o *.lib
