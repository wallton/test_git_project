
OUT_DIR:=output


all: 
	@gcc -o test $(CFLAGS) main.c taglist.c iplist.c numlist.c
	@gcc -o test2 $(CFLAGS) main2.c taglist.c iplist.c numlist.c

clean:
	@rm -f test


$(OUT_DIR):
	mkdir -p $@

