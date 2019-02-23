# PocketSNES

Just to collect the patches for running PocketSNES on PocketCHIP and CHIP.

# Installation Instructions

```
sudo apt install libsdl1.2-dev build-essential  git --no-install-recommends
git clone https://github.com/nicmorais/PocketSNES
cd PocketSNES
make
```

# Run Instructions

```
./PocketSNES /path/to/rom.sfc
```

# Keyboard Controls

Menu:
* cursor keys = UP/DOWN - ENABLE/DISABLE (option)
* i = SELECT (option)
* j = BACK

## In-game:
![](https://i.imgur.com/mHJeC0O.png)
* cursor keys = UP/DOWN/LEFT/RIGHT
* i = A
* j = B
* u = X
* h = Y 
* v = START
* c = SELECT
* o = L
* k = R
* c+v (at once) = MENU

# Change the Keyboard Controls

The keyboard controls are currently hard-coded in `sal/linux/sal.c`. Find the following lines, change as desired and re-compile with make. 

```
switch(event.key.keysym.sym) {
		CASE(LCTRL, A);
		CASE(LALT, B);
		CASE(SPACE, X);
		CASE(LSHIFT, Y);
		CASE(TAB, L);
		CASE(BACKSPACE, R);
		CASE(RETURN, START);
		CASE(ESCAPE, SELECT);
		CASE(UP, UP);
		CASE(DOWN, DOWN);
		CASE(LEFT, LEFT);
		CASE(RIGHT, RIGHT);
		CASE(HOME, MENU);
    default: break;
}
```
