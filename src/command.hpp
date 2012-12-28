#ifndef __QBE_COMMAND_HPP__
#define __QBE_COMMAND_HPP__

/*! Mini-scripting language implemented in qbe (mostly cvompatible with quake
 *  script engine)
 */
namespace cmd
{
  /*! Register a console variable (done through globals) */
  int variable(const char *name, int min, int cur, int max, int *storage, void (*fun)(), bool persist);
  /*! Set the integer value for a variable */
  void setvar(const char *name, int i);
  /*! Set the value of a variable */
  int getvar(const char *name);
  /*! Says if the variable exists (i.e. is registered) */
  bool identexists(const char *name);
  /*! Register a new command */
  bool addcommand(const char *name, void (*fun)(), int narg);
  /*! Execute a given string */
  int execute(char *p, bool down = true);
  /*! Execute a given file and print any error */
  void exec(const char *cfgfile);
  /*! Execute a file and says if this succeeded */
  bool execfile(const char *cfgfile);
  /*! Stop completion */
  void resetcomplete(void);
  /*! Complete the given string */
  void complete(char *s);
  /*! Set an alias with given action */
  void alias(const char *name, const char *action);
  /*! Get the action string for the given variable */
  char *getalias(const char *name);
  /*! Write all commands, variables and alias to config.cfg */
  void writecfg(void);
}

/*! Function signatures for script functions */
enum
{
  ARG_1INT, ARG_2INT, ARG_3INT, ARG_4INT,
  ARG_NONE,
  ARG_1STR, ARG_2STR, ARG_3STR, ARG_5STR,
  ARG_DOWN, ARG_DWN1,
  ARG_1EXP, ARG_2EXP,
  ARG_1EST, ARG_2EST,
  ARG_VARI
};

///////////////////////////////////////////////////////////////////////////
// Handle command and Cvar registrations
///////////////////////////////////////////////////////////////////////////

/*! Register a command with a given name */
#define COMMANDN(name, fun, nargs) \
  static bool __dummy_##fun = cmd::addcommand(#name, (void (*)())fun, nargs)

/*! Register a command with a name given by the function name */
#define COMMAND(name, nargs) COMMANDN(name, name, nargs)

/*! A persistent variable */
#define VARP(name, min, cur, max) \
  int name = cmd::variable(#name, min, cur, max, &name, NULL, true)

/*! A non-persistent variable */
#define VAR(name, min, cur, max) \
  int name = cmd::variable(#name, min, cur, max, &name, NULL, false)

/*! A non-persistent variable with custom code to run when changed */
#define VARF(name, min, cur, max, body) \
  void var_##name(); \
  static int name = cmd::variable(#name, min, cur, max, &name, var_##name, false); \
  void var_##name() { body; }

/*! A persistent variable with custom code to run when changed */
#define VARFP(name, min, cur, max, body) \
  void var_##name(); \
  static int name = cmd::variable(#name, min, cur, max, &name, var_##name, true); \
  void var_##name() { body; }

#endif /* __QBE_COMMAND_HPP__ */



