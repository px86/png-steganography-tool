#pragma once

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iomanip>
#include <ios>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>

namespace pr {

inline bool starts_with(const char *prefix, const char *arg)
{
  if (!memcmp(prefix, arg, strlen(prefix))) return true;
  else return false;
}

inline bool starts_with(char prefix, const char *arg)
{
  return arg[0] == prefix;
}

inline void verify_names(const char *long_name, char short_name)
{
  if (long_name) {
    int i = 0;
    while (long_name[i] != '\0') {
      if (isspace(long_name[i])) {
	std::cerr << "Error: ArgParser -> option names can not have whitespaces: '"
		  << long_name << '\'' << std::endl;
	std::exit(EXIT_FAILURE);
      }
      ++i;
    }
  } else if (short_name) return;
  else {
    std::cerr << "Error: ArgParser -> option name not provided" << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

inline auto has_equalsign(const char *arg) -> const char* {
  for (char c=*arg; c!='\0'; c=*(++arg))
    if (c == '=') return ++arg;
  return nullptr;
}

using func = std::function<void(const char *)>;

struct Option {
  bool requires_value = false;
  const char *help_message = nullptr;
  const char *long_name    = nullptr;
  char short_name = '\0';
  func accept_value = nullptr;

  Option() = default;
  Option(bool rv, const char *help, const char *lname, char sname, func f)
      : requires_value(rv), help_message(help), long_name(lname),
        short_name(sname), accept_value(f) {}
};

struct Argument {
  const char *help_message = nullptr;
  const char *name  = nullptr;
  func accept_value = nullptr;

  Argument() = default;
  Argument(const char *help, const char *name, func f)
    : help_message(help), name(name), accept_value(f) {}
};


class ArgParser {
public:
  ArgParser(const char *program_name)
    : m_program_name(program_name) {
    // Add the help option implicitely.
    m_options.push_back(Option(false, "Print this help message", "help", '\0',
                               [this](const char *) {
                                 print_help();
                                 std::exit(EXIT_SUCCESS);
                               }));
  }
  void parse(const int argc, char **argv);

  void add_option(Option &&);
  void add_option(bool &value, const char *help_msg, const char *long_name, char short_name);
  void add_option(int &value, const char *help_msg, const char *long_name, char short_name);
  void add_option(double &value, const char *help_msg, const char *long_name, char short_name);
  void add_option(const char *&value, const char *help_msg, const char *long_name, char short_name);

  void add_argument(Argument &&);
  void add_argument(int &value, const char *help_msg, const char *name);
  void add_argument(double &value, const char *help_msg, const char *name);
  void add_argument(const char *&value, const char *help_msg, const char *name);

  void print_help() const;
private:
  std::vector<Option> m_options;
  std::vector<Argument> m_positional_arguments;
  size_t m_curr_arg = 0;
  const char *m_program_name = nullptr;
  void unknown_option(const char *opt) const;
};

inline void ArgParser::add_option(bool &value, const char *help_msg,
				  const char *long_name, char short_name)
{
  verify_names(long_name, short_name);
  Option opt(false, help_msg, long_name, short_name, [&value](const char* arg=nullptr) { value = true; });
  m_options.push_back(std::move(opt));
}

inline void ArgParser::add_option(int &value, const char *help_msg,
				  const char *long_name, char short_name)
{
  verify_names(long_name, short_name);
  Option opt(true, help_msg, long_name, short_name,
             [&value](const char *arg) {
	       try {
		 value = std::stoi(arg);
	       } catch (std::invalid_argument &e) {
		 std::cerr << "Error: wrong argument type, integer expected." << std::endl;
		 std::exit(EXIT_FAILURE);
	       }
	     });

  m_options.push_back(std::move(opt));
}

inline void ArgParser::add_option(double &value, const char *help_msg,
				  const char *long_name, char short_name)
{
  verify_names(long_name, short_name);
  Option opt(true, help_msg, long_name, short_name,
	     [&value](const char* arg) {
	       try {
		 value = std::stod(arg);
	       } catch (std::invalid_argument &e) {
		 std::cerr << "Error: wrong argument type, double expected." << std::endl;
		 std::exit(EXIT_FAILURE);
	       }
	     });
  m_options.push_back(std::move(opt));
}

inline void ArgParser::add_option(const char *&value, const char *help_msg,
                                  const char *long_name, char short_name)
{
  verify_names(long_name, short_name);
  Option opt(true, help_msg, long_name, short_name, [&value](const char* arg) {value = arg;});
  m_options.push_back(std::move(opt));
}

inline void ArgParser::add_argument(int &value, const char *help_msg, const char *name)
{
  Argument arg(help_msg, name,
	       [&value](const char *parg) {
	       try {
		 value = std::stoi(parg);
	       } catch (std::invalid_argument &e) {
		 std::cerr << "Error: wrong argument type, integer expected." << std::endl;
		 std::exit(EXIT_FAILURE);
	       }
	       });

  m_positional_arguments.push_back(std::move(arg));
}

inline void ArgParser::add_argument(double &value, const char *help_msg, const char *name)
{
  Argument arg(help_msg, name,
	       [&value](const char *parg) {
		 try {
		   value = std::stod(parg);
		 } catch (std::invalid_argument &e) {
		 std::cerr << "Error: wrong argument type, double expected." << std::endl;
		 std::exit(EXIT_FAILURE);
		 }
	       });

  m_positional_arguments.push_back(std::move(arg));
}

inline void ArgParser::add_argument(const char *&value, const char *help_msg, const char *name)
{
  Argument arg(help_msg, name, [&value](const char *parg) { value = parg; });
  m_positional_arguments.push_back(std::move(arg));
}

inline void ArgParser::parse(const int argc, char **argv)
{
  for (int i=1; i<argc; ++i) {
    // Long named options.
    if (starts_with("--", argv[i])) {
      for (auto itr = m_options.begin(); itr!=m_options.end(); ++itr)
	{
	  if (itr->long_name && starts_with(itr->long_name, argv[i]+2)) {
	    if (itr->requires_value) {
	      auto tmp = has_equalsign(argv[i]);
	      tmp ? itr->accept_value(tmp) : itr->accept_value(argv[++i]);
	    } else itr->accept_value(nullptr);
	    break;
	  }
	  if (itr+1 == m_options.end()) unknown_option(argv[i]);
	}
    } // Short named options.
    else if (starts_with('-', argv[i])) {
      for (auto itr = m_options.begin(); itr!=m_options.end(); ++itr)
	{
	  if (itr->short_name && starts_with(itr->short_name, argv[i]+1)) {
	    if (itr->requires_value) {
	      ( argv[i][2] != '\0' ) ? itr->accept_value(argv[i]+2) : itr->accept_value(argv[++i]);
	    } else itr->accept_value(nullptr);
	    break;
	  }
	  if (itr+1 == m_options.end()) unknown_option(argv[i]);
	}
    } // Positional arguments.
    else {
       if (m_curr_arg < m_positional_arguments.size()) {
	 m_positional_arguments.at(m_curr_arg++).accept_value(argv[i]);
       }
    }
  }
}

// This may hurt your eyes. Sorry.
inline void ArgParser::print_help() const
{
#define FIELD_WIDTH 20
  // Usage: prgram-name [OPTIONS] arg1 arg2...
  std::cout << "Usage: " << m_program_name << " [OPTIONS]";
  for (auto &i: m_positional_arguments) std::cout << ' ' << i.name;
  std::cout << '\n';

  std::cout.setf(std::ios_base::left, std::ios_base::adjustfield);

  for (auto &arg: m_positional_arguments)
    std::cout << std::setw(FIELD_WIDTH) << arg.name << arg.help_message << '\n';

  std::cout << "\nOptions:\n";
  for (auto &opt: m_options) {
    std::stringstream ss;
    //  For Example
    //  --option, -o [VAL]          help-message for option
    if (opt.short_name && opt.long_name) ss << "  --" << opt.long_name << ", -" << opt.short_name;
    else if (opt.long_name) ss << "  --" << opt.long_name;
    else if (opt.short_name) ss << "  -" << opt.short_name;

    if (opt.requires_value) ss << " VAL";

    auto opt_str = ss.str();
    // If the names are too long, then print the help message on the next line
    // with proper indentation.
    if (opt_str.size() >= FIELD_WIDTH) {
      std::cout << std::setw(FIELD_WIDTH) << opt_str << '\n'
		<< std::setw(FIELD_WIDTH) << ' ' << opt.help_message << '\n';
    }
    else std::cout << std::setw(FIELD_WIDTH) << opt_str << opt.help_message << '\n';
  }
  std::cout.setf(std::ios_base::right, std::ios_base::adjustfield);

#undef FIELD_WIDTH
}

inline void ArgParser::unknown_option(const char *opt) const
{
  std::cerr << "Error: unknown option '" << opt << "'\n"
	    << "Try '" << m_program_name << " --help' for more information." << std::endl;
  std::exit(EXIT_FAILURE);
}
} // namespace pr
