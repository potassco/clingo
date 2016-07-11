// {{{ GPL License

// This file is part of reify - a grounder for logic programs.
// Copyright (C) 2013  Roland Kaminski

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// }}}

#include <fstream>
#include <program_opts/application.h>
#include <program_opts/typed_value.h>
#include "reify/program.hh"
#include "gringo/version.hh"

struct ReifyOptions {
    bool calculateSCCs = false;
    bool reifyStep     = false;
};

class ReifyApp : public ProgramOptions::Application {
public:
    virtual const char* getName() const    { return "reify"; }

    virtual const char* getVersion() const { return GRINGO_VERSION; }

protected:
    virtual void initOptions(ProgramOptions::OptionContext& root) {
        using namespace ProgramOptions;
        OptionGroup reify("Reify Options");
        reify.addOptions()
            ("calculate-sccs,c", flag(opts_.calculateSCCs), "calculate strongly connected components\n")
            ("reify-step,s", flag(opts_.reifyStep), "attach current step number to generated facts\n");
        root.add(reify);
        OptionGroup basic("Basic Options");
        basic.addOptions()
            ("file,f,@2", storeTo(input_), "Input files")
            ;
        root.add(basic);
    }

    virtual void validateOptions(const ProgramOptions::OptionContext&, const ProgramOptions::ParsedOptions&, const ProgramOptions::ParsedValues&) { }

    virtual void setup() { }

    static bool parsePositional(std::string const &, std::string& out) {
        out = "file";
        return true;
    }

    virtual ProgramOptions::PosOption getPositional() const { return parsePositional; }

    virtual void printHelp(const ProgramOptions::OptionContext& root) {
        printf("%s version %s\n", getName(), getVersion());
        printUsage();
        ProgramOptions::FileOut out(stdout);
        root.description(out);
        printf("\n");
        printUsage();
    }

    virtual void printVersion() {
        Application::printVersion();
        printf(
            "Copyright (C) Roland Kaminski\n"
            "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
            "Reify is free software: you are free to change and redistribute it.\n"
            "There is NO WARRANTY, to the extent permitted by law.\n");
        fflush(stdout);
    }

    virtual void run() {
        Reify::Reifier reify(std::cout, opts_.calculateSCCs, opts_.reifyStep);
        if (input_.empty() || input_ == "-") {
            reify.parse(std::cin);
        }
        else {
            std::ifstream ifs(input_);
            reify.parse(ifs);
        }
    }
private:
    std::string input_;
    ReifyOptions opts_;
};

int main(int argc, char **argv) {
    ReifyApp app;
    return app.main(argc, argv);
}

