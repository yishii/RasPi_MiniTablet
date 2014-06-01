#
# Makefile for RaspberryPi mini tablet tablet worker
# Yasuhiro ISHII (@yishii)
#

CC		= gcc
CFLAGS		=
OBJS		= smcu_handler.o
TARGET		= rpitw

.PHONY:		clean purge all

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS)

all:	$(TARGET)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -fr $(TARGET) $(OBJS)

purge:
	rm -f *~ *bak

