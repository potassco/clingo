// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright (C) 2013  Benjamin Kaufmann
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

#ifndef _GRINGO_CLINGOAPP_HH
#define _GRINGO_CLINGOAPP_HH

#include "clasp/clasp_app.h"
#include "gringo/version.hh"
#include "clingo/clingocontrol.hh"

// Standalone clingo application.
class ClingoApp : public Clasp::Cli::ClaspAppBase {
    using StringVec   = std::vector<std::string>;
    using Output      = Clasp::Cli::Output;
    using ProblemType = Clasp::ProblemType;
    using BaseType    = Clasp::Cli::ClaspAppBase;
    enum class ConfigUpdate { KEEP, REPLACE };
public:
    ClingoApp();
    const char* getName()    const override { return "clingo"; }
    const char* getVersion() const override { return GRINGO_VERSION; }
    const char* getUsage()   const override { return "[number] [options] [files]"; }

    void shutdown() override;
protected:
    enum Mode { mode_clingo = 0, mode_clasp = 1, mode_gringo = 2 };
    void        initOptions(ProgramOptions::OptionContext& root) override;
    void        validateOptions(const ProgramOptions::OptionContext& root, const ProgramOptions::ParsedOptions& parsed, const ProgramOptions::ParsedValues& vals) override;

    ProblemType getProblemType() override;
    void        run(Clasp::ClaspFacade& clasp) override;
    Output*     createOutput(ProblemType f) override;
    void        printHelp(const ProgramOptions::OptionContext& root) override;
    void        printVersion() override;

    // -------------------------------------------------------------------------------------------
    // Event handler
    void onEvent(const Clasp::Event& ev) override;
    bool onModel(const Clasp::Solver& s, const Clasp::Model& m) override;
    // -------------------------------------------------------------------------------------------
private:
    ClingoApp(const ClingoApp&);
    ClingoApp& operator=(const ClingoApp&);
    ClingoOptions grOpts_;
    Mode mode_;
    DefaultGringoModule module;
    std::unique_ptr<ClingoControl> grd;
};

#endif // _GRINGO_CLINGOAPP_HH
