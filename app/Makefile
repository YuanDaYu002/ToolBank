.PHONY: all clean

SUBDIRS=libencoder libamazon libstream libfmp4Encode  main 
	
all:
	$(foreach D, $(SUBDIRS), make -C $(D) all;)
	cp ./out/ipc18.bin /mnt/hgfs/vmware_share/	
clean:
	$(foreach D, $(SUBDIRS), make -C $(D) clean;)
	
