#include <log.h>
#include <options.h>

#include <list>
#include <map>

using namespace cmd;

std::map<char, Options::Flags> Options::args_lookup_table =
{
	{ 'c', Flags::ANALYSIS_CASE_UNSENSITIVE },
	{ 'e', Flags::ACCEPT_BY_ENTER },
	{ 'h', Flags::SHOW_CMD_HELP },
	{ 'i', Flags::QS_INVERTED },
	{ 'l', Flags::AUTO_LANGUAGE },
	{ 'm', Flags::QS_MIXED },
	{ 'p', Flags::PLAY_SOLUTION },
	{ 'r', Flags::REPEAT_ERRORS_ONLY },
	{ 'q', Flags::HIDE_QUESTION },
	{ 's', Flags::SHOW_STATISTICS },
	{ 't', Flags::USE_TOPICS },
	{ 'u', Flags::ANALYSIS_PUNTCTUATION_UNSENSITIVE },
	{ 'w', Flags::READ_QUESTION },
	{ 'z', Flags::ANALYSIS_TOTAL_RECALL }
};

bool Options::get(Flags flag) const
{
	return _args.find(flag) != _args.end();
}

std::list<std::string> Options::args(Flags flag) const
{
	if (!get(flag))
		return std::list<std::string>();

	return _args.at(flag);
}

bool Options::parse_arguments(int argc, char* argv[])
{
	if (argc < 2) {
		logging::Info() << "no command line arguments" << logging::endl;
		return true;
	}

	static const char* help_option = "-h";
	if (argv[1] == help_option) {
		_show_help = true;
		return true;
	}

	_filename = argv[1];

	// split params to keys and values
	Flags last_option_flag = Flags::NONE;
	for (int i = 2; i < argc; i++) {
		std::string arg = argv[i];
		if (arg[0] == '-') {
			if (arg[1] == '-') {
				logging::Error() << "supported only single dash letters as cmd arguments";
				return false;
			}

			// in "-abc" check 'a', 'b' and 'c'
			last_option_flag = Flags::NONE;
			for (size_t j = 1; j < arg.size(); ++j) {
				auto flag_it = args_lookup_table.find(arg[j]);
				if (flag_it == args_lookup_table.end())
				{
					logging::Error() << "unsupported option flag: " << arg[j];
					return false;
				}

				last_option_flag = flag_it->second;
				_args.insert({ last_option_flag, std::list<std::string>() });
			}
			continue;
		}

		if (last_option_flag == Flags::NONE) {
			logging::Error() << "arguments without an option flag: " << arg;
			return false;
		}

		_args.at(last_option_flag).push_back(arg);
	}

	return true;
}
