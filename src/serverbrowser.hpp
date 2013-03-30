#ifndef __CUBE_BROWSER_HPP__
#define __CUBE_BROWSER_HPP__

namespace cube {
namespace browser {

  void addserver(const char *servername);
  const char *getservername(int n);
  void refreshservers(void);
  void writeservercfg(void);

} /* namespace browser */
} /* namespace cube */

#endif /* __CUBE_BROWSER_HPP__ */

