#include <input/statement.hh>

namespace Gringo::Input {

class ScannerImpl;

class Scanner {
  public:
    friend auto parse_stream(std::istream &in) -> Scanner;
    friend auto parse_file(char const *path) -> Scanner;
    friend auto parse_string(std::string content) -> Scanner;

    ~Scanner() noexcept;
    auto scan() -> std::optional<Statement>;

  private:
    Scanner(std::unique_ptr<ScannerImpl> impl);

    std::unique_ptr<ScannerImpl> impl_;
};

} // namespace Gringo::Input
