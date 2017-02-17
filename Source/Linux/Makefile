CC=g++
OBJ = NZBFuseDrive.o nzbmounter.o


.cpp.o:
	$(CC) -D_FILE_OFFSET_BITS=64 -I. -I../Common -c -o $@ $< $(CFLAGS)

nzbmounter: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) -L ../Common -lCommon -ltinyxml -lboost_thread -lboost_filesystem -lboost_system `pkg-config fuse --cflags --libs` `pkg-config --libs openssl`


.PHONY: clean

clean:
	rm -f *.o *~ core nzbmounter
