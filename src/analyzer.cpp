#include <analyzer.h>

namespace analysis {

Verification BaseAnalyzer::check(
	const Problem& problem, const std::list<std::string>& answer)
{
	Verification v(problem, answer);
	v.answer.remove("");
	v.state = v.problem.solution.size() == v.answer.size()
			? MARK::RIGHT
			: MARK::INVALID_LINES_NUMBER;

	v.errors = std::map<int, int>();
	return v;
}

Verification EqualAnalyzer::check(
	const Problem& problem, const std::list<std::string>& answer)
{
	auto v = BaseAnalyzer::check(problem, answer);
	if (v.state != MARK::RIGHT) return v;

	int line_num = 0;
	auto answr_it = v.answer.cbegin();
	auto solut_it = v.problem.solution.cbegin();
	for (; answr_it != v.answer.cend() && solut_it != v.problem.solution.cend()
		 ; ++answr_it, ++solut_it, ++line_num)
	{
		for (size_t i = 0; i < solut_it->size(); i++) {
			if (i >= answr_it->size() || solut_it->at(i) != answr_it->at(i)) {
				v.errors[line_num] = i;
				break;
			}
		}

		// redundant symbols at the end of the answer
		if (v.errors.find(line_num) == v.errors.end() && answr_it->size() > solut_it->size())
			v.errors[line_num] = solut_it->size();
	}

	if (v.errors.size() != 0) v.state = MARK::ERROR;
	return v;
}

} // namespace analyze

