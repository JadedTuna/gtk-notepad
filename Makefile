all:
	gcc `pkg-config --libs --cflags gtk+-2.0` \
	-o gtk-notepad gtk-notepad.c
