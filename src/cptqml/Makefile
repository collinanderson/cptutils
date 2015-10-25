# Makefile for cptqml
#
# (c) J.J.Green 2015

NAME = cptqml

OBJ      =  cptqml.o main.o options.o
RUBBISH += $(OBJ) $(NAME)

LDLIBS += -lcpt -lfill -lbt -lqml

default : all

all : $(NAME)

# $(NAME).1

install :
	$(INSTALL_DATA) $(NAME).1 $(DESTDIR)$(mandir)/man1/$(NAME).1
	$(INSTALL_PROGRAM) $(NAME) $(DESTDIR)$(bindir)/$(NAME)

clean :
	$(RM) $(RUBBISH)

veryclean :

.PHONEY    : all install clean veryclean

$(NAME) : $(OBJ)

include ../Common.mk

test check : accept
