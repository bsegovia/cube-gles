#ifndef __QBE_CONSOLE_HPP__
#define __QBE_CONSOLE_HPP__

/*! Handle user-typed commands */
namespace console
{
  /*! Handle a pressed key when console is up */
  void keypress(int code, bool isdown, int cooked);
  /*! Render buffer taking into account time & scrolling */
  void render(void);
  /*! Output a formatted string in the console */
  void out(const char *s, ...);
  /*! Get the string for the command currently typed */
  char *getcurcommand(void);
  /*! Write all the bindings in the given file */
  void writebinds(FILE *f);
}

#endif /* __QBE_CONSOLE_HPP__ */

















