all:
	gcc -o gtk-notepad gtk-notepad.c `pkg-config --libs --cflags gtk+-2.0`
