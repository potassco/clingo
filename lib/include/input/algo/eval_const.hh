#include <map>

#include <input/statement.hh>

namespace Gringo::Input {

auto evaluate_const(std::vector<StatementConst> const &stms) -> std::map<std::string, Symbol>;

}
