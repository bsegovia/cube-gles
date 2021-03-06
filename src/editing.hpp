#ifndef __CUBE_EDITING_HPP__
#define __CUBE_EDITING_HPP__

namespace cube {

// XXX move that
extern bool editmode;

namespace edit {

void cursorupdate(void);
void toggleedit(void);
void editdrag(bool isdown);
void setvdeltaxy(int delta, block &sel);
void editequalisexy(bool isfloor, block &sel);
void edittypexy(int type, block &sel);
void edittexxy(int type, int t, block &sel);
void editheightxy(bool isfloor, int amount, block &sel);
bool noteditmode(void);
void pruneundos(int maxremain = 0);

} /* namespace edit */
} /* namespace cube */

#endif /* __CUBE_EDITING_HPP__ */

