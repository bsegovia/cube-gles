#ifndef __CUBE_CONSOLE_HPP__
#define __CUBE_CONSOLE_HPP__

/* handle user-typed commands */
namespace cube {
namespace console {

/* handle a pressed key when console is up */
void keypress(int code, bool isdown, int cooked);
/* render buffer taking into account time & scrolling */
void render(void);
/* output a formatted string in the console */
void out(const char *s, ...);
/* get the string for the command currently typed */
char *getcurcommand(void);
/* write all the bindings in the given file */
void writebinds(FILE *f);

} /* namespace console */
} /* namespace cube */

#endif /* __CUBE_CONSOLE_HPP__ */

