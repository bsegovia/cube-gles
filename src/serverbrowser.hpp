#ifndef __QBE_BROWSER_HPP__
#define __QBE_BROWSER_HPP__

namespace browser
{
  void addserver(const char *servername);
  const char *getservername(int n);
  void refreshservers(void);
  void writeservercfg(void);
} /* namespace browser */

#endif /* __QBE_BROWSER_HPP__ */









