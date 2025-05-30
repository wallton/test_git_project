
#
# -lrt 是為了連結 POSIX 共享記憶體所需的庫（在某些系統上可能需要）
#
all:
	gcc -o writer writer.c -lrt
	gcc -o reader reader.c -lrt
	gcc -o writer2 writer2.c writer2_common.c -lrt
	gcc -o reader2 reader2.c  -lrt

clean:
	rm -f writer reader




