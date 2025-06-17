#   __              
#  / /___ _____ ___ 
# / __/ // (_-</ _ \
# \__/\_, /___/ .__/
#    /___/   /_/    

objs = main.o
cxx = gcc
avoid = -Wno-switch
std = -std=c99
opt = -O0
flags = -Wall -Wpedantic -Wextra $(avoid) $(std) $(opt)
name = tysp

all: $(name)

$(name): $(objs)
	$(cxx)	-o $(name) $(objs)

%.o: %.c
	$(cxx)	-c $< $(flags)

clean:
	rm	-rf $(objs) *~ $(name)
