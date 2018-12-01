CC=g++-8 -O3 -std=c++17
OBJ = NZBFuseDrive.o nzbmounter.o


.cpp.o:
	$(CC) -D_FILE_OFFSET_BITS=64 -I. -I/usr/local/include -I../NZBDriveLib -c -o $@ $< $(CFLAGS)

nzbmounter: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) -L/usr/local/lib -L ../NZBDriveLib -lNZBDrive -ltinyxml2 -lboost_thread -lboost_filesystem -lboost_system  -lboost_regex `pkg-config fuse --cflags --libs` `pkg-config --libs openssl` `pkg-config --libs curlpp` -lstdc++fs


.PHONY: clean

clean:
	rm -f *.o *~ core nzbmounter
