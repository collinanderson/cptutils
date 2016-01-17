# Makefile for cptpg
# (c) J.J.Green 2016

NAME = cptpg

OBJ      = $(NAME).o main.o options.o
RUBBISH += $(OBJ) $(NAME)

CFLAGS  +=
LDLIBS  += -lcpt -lfill -lbt

default : all

all : $(NAME) $(NAME).1

install :
	$(INSTALL_PROGRAM) $(NAME) $(DESTDIR)$(bindir)/$(NAME)
	$(INSTALL_DATA) $(NAME).1 $(DESTDIR)$(mandir)/man1/$(NAME).1

clean :
	$(RM) $(RUBBISH)

veryclean :

.PHONEY    : all install clean veryclean

$(NAME) : $(OBJ)

include ../Common.mk

check test : accept
