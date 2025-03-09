#include <clingo/app.h>

#include <utility>

#include "control.hh"
#include "util.hh"

namespace Clingo::Python {

struct Flag {
    bool value = false;
};

class Options {
  public:
    Options(clingo_options_t *opts) : opts_{opts} {}

    void add_flag(char const *group, char const *option, char const *description, Annotation<Flag> const &flag) {
        auto &cflag = flag.cast<Flag &>();
        handle_error(clingo_options_add_flag(opts_, group, option, description, &cflag.value));
    }

  private:
    clingo_options_t *opts_;
};

class App {
  public:
    App(std::string program_name, std::string version)
        : program_name_{std::move(program_name)}, version_{std::move(version)} {}
    void main(Control control, std::span<char const *> files) {
        PYBIND11_OVERRIDE_NAME(void, App, "main", no_op, control, files);
    }
    void print_model(Model model, std::function<void()> printer) {
        PYBIND11_OVERRIDE_NAME(void, App, "print_model", no_op, model, printer);
    }
    void register_options(Options options) { PYBIND11_OVERRIDE_NAME(void, App, "register_options", no_op, options); }
    void validate_options() { PYBIND11_OVERRIDE_NAME(void, App, "validate_options", no_op); }

  private:
    template <class... Args> void no_op([[maybe_unused]] Args const &...args) {}
    template <class... Args> auto no_op_null([[maybe_unused]] Args const &...args) -> nullptr_t { return nullptr; }

    std::string program_name_;
    std::string version_;
};

void register_app(pybind11::module &m) {
    using namespace Clingo::Python;

    auto app = m.def_submodule("app", R"(
TODO
)"_d);

    py::class_<Flag>(app, "Flag", R"(
TODO
)"_d)
        .def(py::init<bool>(), py::arg("value") = false, R"(
TODO
)"_d)
        .def_readwrite("value", &Flag::value, "Get/set the value of the flag.");

    py::class_<Options>(app, "AppOptions", R"(
TODO
)"_d)
        .def("add_flag", &Options::add_flag, py::arg("group"), py::arg("option"), py::arg("description"),
             py::arg("flag"), R"(
TODO
)"_d);

    py::class_<App>(app, "App", R"(
TODO
)"_d)
        .def(py::init<char const *, char const *>(), py::arg("program_name"), py::arg("version"), R"(
TODO
)"_d);
}

} // namespace Clingo::Python
