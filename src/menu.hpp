#ifndef __CUBE_MENU_HPP__
#define __CUBE_MENU_HPP__

/*! Handle the stack of menus in the game */
namespace cube {
namespace menu {

/*! Render the menus */
bool render(void);
void set(int menu);
void manual(int m, int n, char *text);
void sort(int start, int num);
bool key(int code, bool isdown);
void newm(const char *name);

} /* namespace menu */
} /* namespace cube */

#endif /* __CUBE_MENU_HPP__ */

