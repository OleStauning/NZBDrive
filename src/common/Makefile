CC=g++
DEPS = *.h
OBJ = Logger.o MultipartFile.o NewsClientCache.o NewsClientCommand.o NewsClientConnect.o NewsClient.o NewsClientReadArticleStream.o NewsClientReadOverview.o NewsClientSocket.o NewsClientStatArticle.o NewsServer.o NZBDirectory.o NZBDriveIMPL.o NZBDriveMounter.o NZBDrive.o NZBDriveSharedLib.o nzb_fileloader.o NZBFile.o RARFile.o ReadAheadFile.o SplitFile.o yDecoder.o ZIPFile.o text_tool.o


.cpp.o:
	$(CC) -I. -c -o $@ $< 

libCommon.a: $(OBJ)
	ar rcs $@ $^

.PHONY: clean

clean:
	rm -f *.o *~ core libCommon.a 
