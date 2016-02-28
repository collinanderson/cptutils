# Makefile for svgx
#
# J.J. Green 2016

NAME = agscpt

OBJ = main.o options.o agscpt.o
CFLAGS += `pkg-config --cflags libmdb`
LDLIBS += -lcpt -lfill -lbt `pkg-config --libs libmdb`
RUBBISH += $(NAME) $(OBJ)

default : all

clean :
	$(RM) $(RUBBISH)

$(NAME) : $(OBJ)

include ../Common.mk

all : $(NAME)
