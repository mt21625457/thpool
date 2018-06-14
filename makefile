src = $(wildcard ./*.c)
obj = $(patsubst %.c,%.o,$(src))
appname = pool


$(appname):$(obj)
	gcc $(obj) -o $(appname) -lpthread

%.o:%.c
	gcc -g  -c $< -o $@

.PHONY:clean
clean:
	rm $(appname) $(obj) -f
