CC=g++-9 -O3 -std=c++17
OBJ = NZBMounter.o


.cpp.o:
	$(CC) -D_FILE_OFFSET_BITS=64 -I. -I/usr/local/include -I../NZBDriveLib -I../NZBFuseDrive -c -o $@ $< $(CFLAGS)

nzbmounter: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) -L/usr/local/lib -L ../NZBDriveLib -L../NZBFuseDrive -lNZBDrive -lNZBFuseDrive -ltinyxml2 -lboost_thread -lboost_filesystem -lboost_system  -lboost_regex `pkg-config fuse --cflags --libs` `pkg-config --libs openssl` `pkg-config --libs curlpp` -lstdc++fs -Wl,-rpath,/usr/local/lib


.PHONY: clean

clean:
	rm -f *.o *~ core nzbmounter
