// 
// Copyright (c) 2013-2016, Benjamin Kaufmann
// 
// This file is part of Clasp. See http://www.cs.uni-potsdam.de/clasp/ 
// 
// Clasp is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// Clasp is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Clasp; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
/*!
 * \file 
 * \brief Supermacros for describing clasp's options.
 * \code
 * OPTION(key, "ext", ARG(...), "help", set, get)
 * \endcode
 * An option consists of:
 *  - a key  (valid and unique c identifier in 'snake_case')
 *  - a key extension ("[!][,<alias>][,@<level>]") as understood by ProgramOptions::OptionInitHelper
 *  - an arg description (ARG macro) as understood by ProgramOptions::Value
 *  - a description (string)
 *  - a set action to be executed when a value (string) for the option is found in a source
 *  - a get action to be executed when the current value for an option is requested
 *  . 
 * 
 * \note In the implementation of ClaspCliConfig, each key is mapped to an enumeration constant and 
 * the stringified version of key (i.e. \#key) is used to identify options. 
 * Furthermore, the key is also used for generating command-line option names.
 * As a convention, compound keys using 'snake_case' to separate words 
 * are mapped to dash-separated command-line option names. 
 * E.g. an \<option_like_this\> is mapped to the command-line option "option-like-this".
 *
 * \note ClaspCliConfig assumes a certain option order. In particular, context options shall
 * precede all solver/search options, which in turn shall precede global asp/solving options.
 *
 * \note The following set actions may be used:
 *  - STORE(obj): converts the string value to the type of obj and stores the result in obj.
 *  - STORE_LEQ(n, max): converts the string value to an unsinged int and stores the result in n if it is <= max.
 *  - STORE_FLAG(n): converts the string value to a bool and stores the result in n as either 0 or 1.
 *  - STORE_OR_FILL(n): converts the string value to an unsinged int t and sets n to std::min(t, maxValue(n)).
 *  - FUN(arg): anonymous function of type bool (ArgStream& arg), where arg provides the following interface:
 *    - arg.ok()     : returns whether arg is still valid
 *    - arg.empty()  : returns whether arg is empty (i.e. all tokens were extracted)
 *    - arg.off()    : returns whether arg contains a single token representing a valid off value
 *    - arg >> x     : extracts and converts the current token and stores it in x. Sets failbit if conversion fails.
 *    - arg >> opt(x): extracts an optional argument, shorthand for (!arg.empty() ? arg>>obj : arg)
 *    .
 *
 * \note The following get actions may be used:
 *  - GET_FUN(str)  : anonymous function of type void (OutputStream& str)
 *  - GET(obj...)   : shorthand for GET_FUN(str) { (str << obj)...; }
 *  - GET_IF(C, obj): shorthand for GET_FUN(str) { ITE(C, str << obj, str << off); }
 *  .
 *
 * \note The following primitives may be used in the set/get arguments:
 *  - off                  : singleton object representing a valid off value ("no", "off", "false", "0")
 *  - ITE(C, x, y)         : if-then-else expression, i.e. behaves like (C ? x : y).
 *  - SET(x, y)            : shorthand for (x=y) == y.
 *  - SET_LEQ(x, v, m)     : shorthand for (x <= m && SET(x, v)).
 *  - SET_GEQ(x, v, m)     : shorthand for (x >= m && SET(x, v)).
 *  - SET_OR_FILL(x, v)    : behaves like SET(x, min(v, maxValue(x)))
 *  - SET_OR_ZERO(x,v)     : behaves like ITE(v <= maxValue(x), SET(x, v), SET(x, 0)).
 *  .
 */
#if !defined(OPTION) || defined(SELF)
#error Invalid include context
#endif

#if !defined(GROUP_BEGIN)
#define GROUP_BEGIN(X)
#endif

#if !defined(GROUP_END)
#define GROUP_END(X)
#endif

//! Options for configuring a SharedContext object stored in a Clasp::ContextParams object.
#if defined(CLASP_CONTEXT_OPTIONS)
#define SELF CLASP_CONTEXT_OPTIONS
GROUP_BEGIN(SELF)
OPTION(share, "!,@1", ARG_EXT(defaultsTo("auto")->state(Value::value_defaulted), DEFINE_ENUM_MAPPING(ContextParams::ShareMode, \
       MAP("no"  , ContextParams::share_no)  , MAP("all", ContextParams::share_all),\
       MAP("auto", ContextParams::share_auto), MAP("problem", ContextParams::share_problem),\
       MAP("learnt", ContextParams::share_learnt))),\
       "Configure physical sharing of constraints [%D]\n"\
       "      %A: {auto|problem|learnt|all}", FUN(arg) {ContextParams::ShareMode x; return arg>>x && SET(SELF.shareMode, (uint32)x);}, GET((ContextParams::ShareMode)SELF.shareMode))
OPTION(learn_explicit, ",@1" , ARG(flag()), "Do not use Short Implication Graph for learning\n", STORE_FLAG(SELF.shortMode), GET(SELF.shortMode))
OPTION(sat_prepro    , "!,@1", ARG(implicit("2")), "Run SatELite-like preprocessing (Implicit: %I)\n" \
       "      %A: <type>[,<limit>...(0=no limit)]\n"                \
       "        <type>: Run {1=VE|2=VE with BCE|3=BCE followed by VE}\n"\
       "        <x1>  : Set iteration limit to <x1>           [0]\n"\
       "        <x2>  : Set variable occurrence limit to <x2> [0]\n"\
       "        <x3>  : Set time limit to <x3> seconds        [0]\n"\
       "        <x4>  : Set frozen variables limit to <x4>%%   [0]\n"\
       "        <x5>  : Set clause limit to <x5>*1000      [4000]", FUN(arg) {\
       SatPreParams& pre = SELF.satPre; uint32 x;\
       return ITE(arg.off(), SET(pre.type, 0u), arg>>x && SET_GEQ(pre.type, x, 1u)\
         && arg >> opt(x = 0)    && SET_OR_ZERO(pre.limIters , x)\
         && arg >> opt(x = 0)    && SET_OR_ZERO(pre.limOcc   , x)\
         && arg >> opt(x = 0)    && SET_OR_ZERO(pre.limTime  , x)\
         && arg >> opt(x = 0)    && SET_OR_ZERO(pre.limFrozen, x)\
         && arg >> opt(x = 4000) && SET_OR_ZERO(pre.limClause, x));},\
       GET_FUN(str) { SatPreParams& pre = SELF.satPre;\
         ITE(pre.type, str<<pre.type<<pre.limIters<<pre.limOcc<<pre.limTime<<pre.limFrozen<<pre.limClause, str<<off);})
GROUP_END(SELF)
#undef CLASP_CONTEXT_OPTIONS
#undef SELF
#endif

//! Global options only valid in facade.
#if defined(CLASP_GLOBAL_OPTIONS)
#define SELF CLASP_GLOBAL_OPTIONS
GROUP_BEGIN(SELF)
OPTION(stats, ",s", ARG(implicit("1")->arg("<n>[,<t>]")), "Enable {1=basic|2=full} statistics (<t> for tester)",\
    FUN(arg) { uint32 s = 0; uint32 t = 0;\
      return (arg.off() || (arg >> s && s > 0)) && arg >> opt(t) && arg.empty()
        && SET(SELF.stats, s) && ((!SELF.testerConfig() && t == 0) || SET(SELF.addTesterConfig()->stats, t));
    },\
    GET_FUN(str) { ITE(!SELF.testerConfig() || !SELF.testerConfig()->stats, str << SELF.stats, str << SELF.stats << SELF.testerConfig()->stats); })
OPTION(parse_ext, "!", ARG(flag()), "Enable extensions in smodels and dimacs input",\
    FUN(arg) { bool b = false; return (arg.off() || arg >> b) && SET(SELF.parse.ext, (b ? unsigned(ParserOptions::parse_full):0u)); }, \
    GET((SELF.parse.ext != 0)))
GROUP_END(SELF)
#undef CLASP_GLOBAL_OPTIONS
#undef SELF
#endif

//! Solver options (see SolverParams).
#if defined(CLASP_SOLVER_OPTIONS)
#define SELF CLASP_SOLVER_OPTIONS
GROUP_BEGIN(SELF)
OPTION(opt_strategy , ""  , ARG_EXT(arg("<arg>")->implicit("1"), DEFINE_ENUM_MAPPING(MinimizeMode_t::Strategy,\
       MAP("bb", MinimizeMode_t::opt_bb), MAP("usc", MinimizeMode_t::opt_usc))),  "Configure optimization strategy\n" \
       "      %A: {bb|usc}[,<n>]\n" \
       "        bb : branch and bound based optimization with <n = {0..3}>\n"  \
       "          1: hierarchical steps\n"                                     \
       "          2: exponentially increasing steps\n"                         \
       "          3: exponentially decreasing steps\n"                         \
       "        usc: unsatisfiable-core based optimization with <n = {0..15}>\n"\
       "          1: disjoint-core preprocessing\n"                            \
       "          2: implications instead of equivalences\n"                   \
       "          4: clauses instead of cardinality constraints (pmres)\n"     \
       "          8: stratification heuristic for handling weights"           ,\
       FUN(arg) { MinimizeMode_t::Strategy sc = MinimizeMode_t::opt_bb; uint32 n;\
         return ((SET_LEQ(n, uint32(arg.peek()-'0'), 9u) && arg >> n && (n < 4u || (SET(sc, MinimizeMode_t::opt_usc) && (n -= 4u) < 16u)))\
           || (arg>>sc>>opt(n = 0))) && SET(SELF.optStrat, (uint32)sc) && SET(SELF.optParam, n) && (n < 4u || sc == MinimizeMode_t::opt_usc);},\
       GET(static_cast<MinimizeMode_t::Strategy>(SELF.optStrat), SELF.optParam))
OPTION(opt_heuristic, ""  , ARG(implicit("1")->arg("{0..3}")), "Use opt. in {1=sign|2=model|3=both} heuristics", STORE_LEQ(SELF.optHeu,  3u), GET(SELF.optHeu))
OPTION(restart_on_model, "", ARG(flag()), "Restart after each model\n", STORE_FLAG(SELF.restartOnModel), GET(SELF.restartOnModel))
OPTION(lookahead    , "!", ARG_EXT(implicit("atom"), DEFINE_ENUM_MAPPING(VarType, \
       MAP("atom", Var_t::Atom), MAP("body", Var_t::Body), MAP("hybrid", Var_t::Hybrid))),\
       "Configure failed-literal detection (fld)\n" \
       "      %A: <type>[,<limit>] / Implicit: %I\n" \
       "        <type> : Run fld via {atom|body|hybrid} lookahead\n" \
       "        <limit>: Disable fld after <limit> applications ([0]=no limit)\n" \
       "      --lookahead=atom is default if --no-lookback is used\n", FUN(arg) { \
       VarType type; uint32 limit = (SELF.lookOps = 0u);\
       return ITE(arg.off(), SET(SELF.lookType, 0u), arg>>type>>opt(limit) && SET(SELF.lookType, (uint32)type)) && SET_OR_ZERO(SELF.lookOps, limit);},\
       GET_IF(SELF.lookType, (VarType)SELF.lookType, SELF.lookOps))
OPTION(heuristic, "", ARG_EXT(arg("<heu>"), DEFINE_ENUM_MAPPING(Heuristic_t::Type, \
       MAP("berkmin", Heuristic_t::Berkmin), MAP("vmtf"  , Heuristic_t::Vmtf), \
       MAP("vsids"  , Heuristic_t::Vsids)  , MAP("domain", Heuristic_t::Domain), \
       MAP("unit"   , Heuristic_t::Unit)   , MAP("auto", Heuristic_t::Default), MAP("none"  , Heuristic_t::None))), \
       "Configure decision heuristic\n"  \
       "      %A: {Berkmin|Vmtf|Vsids|Domain|Unit|None}[,<n>]\n" \
       "        Berkmin: Use BerkMin-like heuristic (Check last <n> nogoods [0]=all)\n" \
       "        Vmtf   : Use Siege-like heuristic (Move <n> literals to the front [8])\n" \
       "        Vsids  : Use Chaff-like heuristic (Use 1.0/0.<n> as decay factor  [95])\n"\
       "        Domain : Use domain knowledge in Vsids-like heuristic\n"\
       "        Unit   : Use Smodels-like heuristic (Default if --no-lookback)\n" \
       "        None   : Select the first free variable", FUN(arg) { Heuristic_t::Type h; uint32 n = 0u; \
       return arg>>h>>opt(n) && SET(SELF.heuId, (uint32)h) && (Heuristic_t::isLookback(h) || !n) && SET_OR_FILL(SELF.heuristic.param, n);},\
       GET((Heuristic_t::Type)SELF.heuId, SELF.heuristic.param))
OPTION(init_moms  , "!,@2", ARG(flag())    , "Initialize heuristic with MOMS-score", STORE_FLAG(SELF.heuristic.moms), GET(SELF.heuristic.moms))
OPTION(score_res  , ",@2" , ARG(arg("<n>")), "Resolution score {0=default|1=min|2=set|3=multiset}", STORE_LEQ(SELF.heuristic.score,3u), GET(SELF.heuristic.score))
OPTION(score_other, ",@2" , ARG(arg("<n>")), "Score {0=no|1=loop|2=all} other learnt nogoods", STORE_LEQ(SELF.heuristic.other,2u), GET(SELF.heuristic.other))
OPTION(sign_def   , ""    , ARG(arg("<n>")), "Default sign: {0=asp|1=pos|2=neg|3=rnd}", STORE_LEQ(SELF.signDef,3u), GET(SELF.signDef))
OPTION(sign_fix   , "!"   , ARG(flag())    , "Disable sign heuristics and use default signs only", STORE_FLAG(SELF.signFix), GET(SELF.signFix))
OPTION(berk_huang , "!,@2", ARG(flag())    , "Enable/Disable Huang-scoring in Berkmin", STORE_FLAG(SELF.heuristic.huang), GET(SELF.heuristic.huang))
OPTION(vsids_acids, "!,@2", ARG(flag())    , "Enable/Disable acids-scheme in Vsids/Domain", STORE_FLAG(SELF.heuristic.acids), GET(SELF.heuristic.acids))
OPTION(vsids_progress, ",@2", NO_ARG, "Enable dynamic decaying scheme in Vsids/Domain\n"\
       "      %A: <n>[,<i {1..100}>][,<c>]|(0=disable)\n"\
       "        <n> : Set initial decay factor to 1.0/0.<n>\n"\
       "        <i> : Set decay update to <i>/100.0      [1]\n"\
       "        <c> : Decrease decay every <c> conflicts [5000]", \
       FUN(arg) { uint32 n = 80; uint32 i = 1; uint32 c = 5000; \
       return ITE(arg.off(), SET(SELF.heuristic.decay.init, 0), (arg >> n >> opt(i) >> opt(c))\
         && SET(SELF.heuristic.decay.init, n) && SET_LEQ(SELF.heuristic.decay.bump, i, 100) && SET(SELF.heuristic.decay.freq, c));}, \
       GET_IF(SELF.heuristic.decay.init, SELF.heuristic.decay.init, SELF.heuristic.decay.bump, SELF.heuristic.decay.freq))
OPTION(nant       , "!,@2", ARG(flag())    , "Prefer negative antecedents of P in heuristic", STORE_FLAG(SELF.heuristic.nant), GET(SELF.heuristic.nant))
OPTION(dom_mod    , ",@2" , NO_ARG         , "Default modification for domain heuristic\n"\
       "      %A: <mod>[,<pick>]\n"\
       "        <mod>  : Modifier {1=level|2=pos|3=true|4=neg|5=false|6=init|7=factor}\n"\
       "        <pick> : Apply <mod> to {0=all|1=scc|2=hcc|4=disj|8=min|16=show} atoms",\
       FUN(arg) { uint32 n; return arg>>n && SET_LEQ(SELF.heuristic.domMod, n, 7u) && arg>>opt(n = 0) && SET(SELF.heuristic.domPref, n);},\
       GET(SELF.heuristic.domMod, SELF.heuristic.domPref))
OPTION(save_progress, "", ARG(implicit("1")->arg("<n>")), "Use RSat-like progress saving on backjumps > %A", STORE_OR_FILL(SELF.saveProgress), GET(SELF.saveProgress))
OPTION(init_watches , ",@2", ARG(arg("{0..2}")->defaultsTo("1")->state(Value::value_defaulted)),\
       "Configure watched literal initialization [%D]\n" \
       "      Watch {0=first|1=random|2=least watched} literals in nogoods", STORE_LEQ(SELF.initWatches, 2u), GET(SELF.initWatches))
OPTION(update_mode   , ",@2", ARG(arg("<n>")), "Process messages on {0=propagation|1=conflict)", STORE_LEQ(SELF.upMode, 1u), GET(SELF.upMode))
OPTION(acyc_prop, ",@2", ARG(implicit("1")->arg("{0..1}")), "Acyc propagate with {0=inc|1=inc+back}", \
       FUN(arg) { uint32 x; return arg>>x && SET_LEQ(SELF.acycFwd, (1u-x), 1u); }, GET(1u-SELF.acycFwd))
OPTION(seed          , ""   , ARG(arg("<n>")),"Set random number generator's seed to %A", STORE(SELF.seed), GET(SELF.seed))
OPTION(no_lookback   , ""   , ARG(flag()), "Disable all lookback strategies\n", STORE_FLAG(SELF.search),GET(static_cast<bool>(SELF.search == SolverStrategies::no_learning)))
OPTION(forget_on_step, ""   , ARG(arg("<bits>")), "Configure forgetting on (incremental) step\n"\
       "      Forget {1=heuristic|2=signs|4=nogood activities|8=learnt nogoods}\n", STORE_LEQ(SELF.forgetSet, 15u), GET(SELF.forgetSet))
OPTION(strengthen    , "!"  , ARG_EXT(arg("<X>"), DEFINE_ENUM_MAPPING(SolverStrategies::CCMinType, \
       MAP("local", SolverStrategies::cc_local), MAP("recursive", SolverStrategies::cc_recursive))), \
       "Use MiniSAT-like conflict nogood strengthening\n" \
       "      %A: <mode>[,<type>][,<keep>]\n" \
       "        <mode>: Use {local|recursive} self-subsumption check\n" \
       "        <type>: Follow {0=all|1=short|2=binary} antecedents   [0]\n"\
       "        <keep>: {0=Increase|1=Keep} activities of antecedents [0]", FUN(arg) {\
       SolverStrategies::CCMinType m = SolverStrategies::cc_local; uint32 t; uint32 k = 0; \
       return ITE(arg.off(), SET(t, 0u), arg>>m>>opt(t = 0u)>>opt(k) && ++t) && SET(SELF.ccMinAntes, t) && SET(SELF.ccMinRec, (uint32)m) && SET(SELF.ccMinKeepAct, k);},\
       GET_IF(SELF.ccMinAntes, (SolverStrategies::CCMinType)SELF.ccMinRec, SELF.ccMinAntes-uint32(1), SELF.ccMinKeepAct))
OPTION(otfs        , ""   , ARG(implicit("1")->arg("{0..2}")), "Enable {1=partial|2=full} on-the-fly subsumption", STORE_LEQ(SELF.otfs, 2u), GET(SELF.otfs))
OPTION(update_lbd  , ",@2", ARG(implicit("1")->arg("{0..3}")), "Update LBDs of learnt nogoods {1=<|2=strict<|3=+1<}", STORE_LEQ(SELF.updateLbd, 3u),GET(SELF.updateLbd))
OPTION(update_act  , ",@2", ARG(flag()), "Enable LBD-based activity bumping", STORE_FLAG(SELF.bumpVarAct), GET(SELF.bumpVarAct))
OPTION(reverse_arcs, ""   , ARG(implicit("1")->arg("{0..3}")), "Enable ManySAT-like inverse-arc learning", STORE_LEQ(SELF.reverseArcs, 3u), GET(SELF.reverseArcs))
OPTION(contraction , "!"  , NO_ARG, "Configure handling of long learnt nogoods\n"
       "      %A: <n>[,<rep>]\n"\
       "        <n>  : Contract nogoods if size > <n> (0=disable)\n"\
       "        <rep>: Replace literal blocks with {1=decisions|2=uips} ([0]=disable)\n", FUN(arg) { uint32 n = 0; uint32 r = 0;\
       return (arg.off() || (arg>>n>>opt(r) && n != 0u)) && SET_OR_FILL(SELF.compress, n) && SET_LEQ(SELF.ccRepMode, r,3u);},\
       GET_IF(SELF.compress, SELF.compress, SELF.ccRepMode))
OPTION(loops, "" , ARG_EXT(arg("<type>"), DEFINE_ENUM_MAPPING(DefaultUnfoundedCheck::ReasonStrategy,\
       MAP("common"  , DefaultUnfoundedCheck::common_reason)  , MAP("shared", DefaultUnfoundedCheck::shared_reason), \
       MAP("distinct", DefaultUnfoundedCheck::distinct_reason), MAP("no", DefaultUnfoundedCheck::only_reason))),\
       "Configure learning of loop nogoods\n" \
       "      %A: {common|distinct|shared|no}\n" \
       "        common  : Create loop nogoods for atoms in an unfounded set\n" \
       "        distinct: Create distinct loop nogood for each atom in an unfounded set\n" \
       "        shared  : Create loop formula for a whole unfounded set\n" \
       "        no      : Do not learn loop formulas\n", FUN(arg) {DefaultUnfoundedCheck::ReasonStrategy x; return arg>>x && SET(SELF.loopRep, (uint32)x);},\
       GET(static_cast<DefaultUnfoundedCheck::ReasonStrategy>(SELF.loopRep)))
GROUP_END(SELF)
#undef CLASP_SOLVER_OPTIONS
#undef SELF
#endif

//! Search-related options (see SolveParams).
#if defined(CLASP_SEARCH_OPTIONS)
#define SELF CLASP_SEARCH_OPTIONS
GROUP_BEGIN(SELF)
OPTION(partial_check, "", ARG(implicit("50")), "Configure partial stability tests\n" \
       "      %A: <p>[,<h>] / Implicit: %I\n" \
       "        <p>: Partial check skip percentage\n"    \
       "        <h>: Init/update value for high bound (0 = umax)", FUN(arg) {\
       uint32 p = 0; uint32 h = 0; \
       return (arg.off() || (arg>>p>>opt(h) && p)) && SET_LEQ(SELF.fwdCheck.highPct, p, 100u) && SET_OR_ZERO(SELF.fwdCheck.highStep, h);},\
       GET_IF(SELF.fwdCheck.highPct, SELF.fwdCheck.highPct, SELF.fwdCheck.highStep))
OPTION(sign_def_disj, "", ARG(arg("<n>")), "Default sign for atoms in disjunctions", STORE_LEQ(SELF.fwdCheck.signDef,3u), GET(SELF.fwdCheck.signDef))
OPTION(rand_freq, "!", ARG(arg("<p>")), "Make random decisions with probability %A", FUN(arg) {\
       double f = 0.0; \
       return (arg.off() || arg>>f) && SET_R(SELF.randProb, (float)f, 0.0f, 1.0f);}, GET(SELF.randProb))
OPTION(rand_prob, "!", ARG(implicit("10,100")), "Configure random probing (Implicit: %I)\n" \
       "      %A: <n1>[,<n2>]\n" \
       "        Run <n1> random passes with at most <n2> conflicts each", FUN(arg) {\
       uint32 n1 = 0; uint32 n2 = 100;\
       return (arg.off() || (arg>>n1>>opt(n2) && n1)) && SET_OR_FILL(SELF.randRuns, n1) && SET_OR_FILL(SELF.randConf, n2);},\
       GET_IF(SELF.randRuns, SELF.randRuns,SELF.randConf))
#undef SELF
//! Options for configuring the restart strategy of a solver.
#define SELF CLASP_SEARCH_RESTART_OPTIONS
#if defined(NOTIFY_SUBGROUPS)
GROUP_BEGIN(SELF)
#endif
OPTION(restarts, "!,r", ARG(arg("<sched>")), "Configure restart policy\n" \
       "      %A: <type {D|F|L|x|+}>,<n {1..umax}>[,<args>][,<lim>]\n"                    \
       "        F,<n>    : Run fixed sequence of <n> conflicts\n"                         \
       "        L,<n>    : Run Luby et al.'s sequence with unit length <n>\n"             \
       "        x,<n>,<f>: Run geometric seq. of <n>*(<f>^i) conflicts  (<f> >= 1.0)\n"   \
       "        +,<n>,<m>: Run arithmetic seq. of <n>+(<m>*i) conflicts (<m {0..umax}>)\n"\
       "        ...,<lim>: Repeat seq. every <lim>+j restarts           (<type> != F)\n"  \
       "        D,<n>,<f>: Restart based on moving LBD average over last <n> conflicts\n" \
       "                   Mavg(<n>,LBD)*<f> > avg(LBD)\n"                                \
       "                   use conflict level average if <lim> > 0 and avg(LBD) > <lim>\n"\
       "      no|0       : Disable restarts", FUN(arg) { return ITE(arg.off(), (SELF.disable(),true), \
       arg>>SELF.sched && SET(SELF.dynRestart, uint32(SELF.sched.type == ScheduleStrategy::User)));}, GET(SELF.sched))
OPTION(reset_restarts  , ""   , ARG(arg("0..2")->implicit("1")), "{0=Keep|1=Reset|2=Disable} restart seq. after model", STORE_LEQ(SELF.upRestart, 2u), GET(SELF.upRestart))
OPTION(local_restarts  , ""   , ARG(flag()), "Use Ryvchin et al.'s local restarts", STORE_FLAG(SELF.cntLocal), GET(SELF.cntLocal))
OPTION(counter_restarts, ""   , ARG(arg("<n>")), "Do a counter implication restart every <n> restarts", STORE_OR_FILL(SELF.counterRestart),GET(SELF.counterRestart))
OPTION(counter_bump    , ",@2", ARG(arg("<n>"))    , "Set CIR bump factor to %A", STORE_OR_FILL(SELF.counterBump), GET(SELF.counterBump))
OPTION(block_restarts  , ""   , ARG(arg("<args>")), "Use glucose-style blocking restarts\n" \
       "      %A: <n>[,<R {1.0..5.0}>][,<x>]\n" \
       "        <n>: Window size for moving average (0=disable blocking)\n" \
       "        <R>: Block restart if assignment > average * <R>  [1.4]\n"             \
       "        <x>: Disable blocking for the first <c> conflicts [10000]\n", FUN(arg) { \
       uint32 n = 0; double R = 0.0; uint32 x = 0;\
       return (arg.off() || (arg>>n>>opt(R = 1.4)>>opt(x = 10000) && n && R >= 1.0 && R <= 5.0))\
         && SET(SELF.blockWindow, n) && SET(SELF.blockScale, (float)R) && SET_OR_FILL(SELF.blockFirst, x);},\
       GET_IF(SELF.blockWindow, SELF.blockWindow, SELF.blockScale, SELF.blockFirst))
OPTION(shuffle         , "!"  , ARG(arg("<n1>,<n2>")), "Shuffle problem after <n1>+(<n2>*i) restarts\n", FUN(arg) { uint32 n1 = 0; uint32 n2 = 0;\
       return (arg.off() || (arg>>n1>>opt(n2) && n1)) && SET_OR_FILL(SELF.shuffle, n1) && SET_OR_FILL(SELF.shuffleNext, n2);},\
       GET_IF(SELF.shuffle, SELF.shuffle, SELF.shuffleNext))
#if defined(NOTIFY_SUBGROUPS)
GROUP_END(SELF)
#endif
#undef SELF
#undef CLASP_SEARCH_RESTART_OPTIONS
//! Options for configuring the deletion strategy of a solver.
#define SELF CLASP_SEARCH_REDUCE_OPTIONS
#if defined(NOTIFY_SUBGROUPS)
GROUP_BEGIN(SELF)
#endif
OPTION(deletion    , "!,d", ARG_EXT(defaultsTo("basic,75,0")->state(Value::value_defaulted), DEFINE_ENUM_MAPPING(ReduceStrategy::Algorithm,\
       MAP("basic", ReduceStrategy::reduce_linear), MAP("sort", ReduceStrategy::reduce_stable),\
       MAP("ipSort", ReduceStrategy::reduce_sort) , MAP("ipHeap", ReduceStrategy::reduce_heap))),
       "Configure deletion algorithm [%D]\n" \
       "      %A: <algo>[,<n {1..100}>][,<sc>]\n"  \
       "        <algo>: Use {basic|sort|ipSort|ipHeap} algorithm\n" \
       "        <n>   : Delete at most <n>%% of nogoods on reduction    [75]\n" \
       "        <sc>  : Use {0=activity|1=lbd|2=combined} nogood scores [0]\n" \
       "      no      : Disable nogood deletion", FUN(arg){\
       ReduceStrategy::Algorithm algo; uint32 n; uint32 sc;\
       return ITE(arg.off(), (SELF.disable(), true), arg>>algo>>opt(n = 75)>>opt(sc = 0)\
         && SET(SELF.strategy.algo, (uint32)algo) && SET_R(SELF.strategy.fReduce, n, 1, 100) && SET_LEQ(SELF.strategy.score, sc, 2));},\
       GET_IF(SELF.strategy.fReduce, (ReduceStrategy::Algorithm)SELF.strategy.algo, SELF.strategy.fReduce,SELF.strategy.score))
OPTION(del_grow    , "!", NO_ARG, "Configure size-based deletion policy\n" \
       "      %A: <f>[,<g>][,<sched>] (<f> >= 1.0)\n"          \
       "        <f>     : Keep at most T = X*(<f>^i) learnt nogoods with X being the\n"\
       "                  initial limit and i the number of times <sched> fired\n"     \
       "        <g>     : Stop growth once T > P*<g> (0=no limit)      [3.0]\n"        \
       "        <sched> : Set grow schedule (<type {F|L|x|+}>) [grow on restart]", FUN(arg){ double f; double g; ScheduleStrategy sc = ScheduleStrategy::def();\
       return ITE(arg.off(), (SELF.growSched = ScheduleStrategy::none(), SELF.fGrow = 0.0f, true),\
         arg>>f>>opt(g = 3.0)>>opt(sc) && SET_R(SELF.fGrow, (float)f, 1.0f, FLT_MAX) && SET_R(SELF.fMax, (float)g, 0.0f, FLT_MAX)\
           && (sc.defaulted() || sc.type != ScheduleStrategy::User) && (SELF.growSched=sc, true));},\
       GET_FUN(str) { if (SELF.fGrow == 0.0f) str<<off; else { str<<SELF.fGrow<<SELF.fMax; if (!SELF.growSched.disabled()) str<<SELF.growSched;}})
OPTION(del_cfl     , "!", ARG(arg("<sched>")), "Configure conflict-based deletion policy\n" \
       "      %A:   <type {F|L|x|+}>,<args>... (see restarts)", FUN(arg){\
       return ITE(arg.off(), (SELF.cflSched=ScheduleStrategy::none()).disabled(), arg>>SELF.cflSched && SELF.cflSched.type != ScheduleStrategy::User);}, GET(SELF.cflSched))
OPTION(del_init  , ""  , ARG(defaultsTo("3.0")->state(Value::value_defaulted)), "Configure initial deletion limit\n"\
       "      %A: <f>[,<n>,<o>] (<f> > 0)\n" \
       "        <f>    : Set initial limit to P=estimated problem size/<f> [%D]\n" \
       "        <n>,<o>: Clamp initial limit to the range [<n>,<n>+<o>]" , FUN(arg) { double f; uint32 lo = 10; uint32 hi = UINT32_MAX;\
       return arg>>f>>opt(lo)>>opt(hi) && f > 0.0 && (SELF.fInit = float(1.0 / f)) > 0 && SET_OR_FILL(SELF.initRange.lo, lo) && SET_OR_FILL(SELF.initRange.hi, (uint64(hi)+SELF.initRange.lo));},\
       GET_IF(SELF.fInit, 1.0/SELF.fInit, SELF.initRange.lo, SELF.initRange.hi - SELF.initRange.lo))
OPTION(del_estimate, "", ARG(arg("0..3")->implicit("1")), "Use estimated problem complexity in limits", STORE_LEQ(SELF.strategy.estimate, 3u), GET(SELF.strategy.estimate))
OPTION(del_max     , "!", ARG(arg("<n>,<X>")), "Keep at most <n> learnt nogoods taking up to <X> MB", FUN(arg) { uint32 n = UINT32_MAX; uint32 mb = 0; \
       return (arg.off() || arg>>n>>opt(mb)) && SET_GEQ(SELF.maxRange, n, 1u) && SET(SELF.memMax, mb);}, GET(SELF.maxRange, SELF.memMax))
OPTION(del_glue    , "", NO_ARG, "Configure glue clause handling\n" \
       "      %A: <n {0..15}>[,<m {0|1}>]\n"                                    \
       "        <n>: Do not delete nogoods with LBD <= <n>\n"                    \
       "        <m>: Count (0) or ignore (1) glue clauses in size limit [0]", FUN(arg) {uint32 lbd; uint32 m = 0; \
       return arg>>lbd>>opt(m) && SET(SELF.strategy.glue, lbd) && SET(SELF.strategy.noGlue, m);}, GET(SELF.strategy.glue, SELF.strategy.noGlue))
OPTION(del_protect , "", ARG(arg("<n>")), "Protect recently updated nogoods if new LBD <= <n>", STORE_LEQ(SELF.strategy.protect, uint32(Clasp::LBD_MAX)), GET(SELF.strategy.protect))
OPTION(del_on_restart, "", ARG(arg("<n>")->implicit("33")), "Delete %A%% of learnt nogoods on each restart", STORE_LEQ(SELF.strategy.fRestart, 100u), GET(SELF.strategy.fRestart))
#if defined(NOTIFY_SUBGROUPS)
GROUP_END(SELF)
#endif
#undef SELF
#undef CLASP_SEARCH_REDUCE_OPTIONS
GROUP_END(CLASP_SEARCH_OPTIONS)
#undef CLASP_SEARCH_OPTIONS
#endif

//! ASP-front-end options stored in an Clasp::Asp::LogicProgram::AspOptions object.
#if defined(CLASP_ASP_OPTIONS)
#define SELF CLASP_ASP_OPTIONS
GROUP_BEGIN(SELF)
OPTION(supp_models, ",@1" , ARG(flag())    , "Compute supported models", STORE_FLAG(SELF.suppMod), GET(SELF.suppMod))
OPTION(no_ufs_check, ",@1" , ARG(flag())   , "Disable unfounded set check", STORE_FLAG(SELF.noSCC), GET(SELF.noSCC))
OPTION(eq         , ""    , ARG(arg("<n>")), "Configure equivalence preprocessing\n" \
       "      Run for at most %A iterations (-1=run to fixpoint)", STORE_OR_FILL(SELF.iters), GET(SELF.iters))
OPTION(backprop   , "!,@1", ARG(flag())    , "Use backpropagation in ASP-preprocessing", STORE_FLAG(SELF.backprop), GET(SELF.backprop))
OPTION(no_gamma   , ",@1" , ARG(flag())    , "Do not add gamma rules for non-hcf disjunctions", STORE_FLAG(SELF.noGamma), GET(SELF.noGamma))
OPTION(eq_dfs     , ",@2" , ARG(flag())    , "Enable df-order in eq-preprocessing", STORE_FLAG(SELF.dfOrder), GET(SELF.dfOrder))
OPTION(dlp_old_map, ",@3" , ARG(flag())    , "Enable old mapping for disjunctive LPs", STORE_FLAG(SELF.oldMap), GET(SELF.oldMap))
OPTION(trans_ext  , "!", ARG_EXT(arg("<mode>"), DEFINE_ENUM_MAPPING(Asp::LogicProgram::ExtendedRuleMode,\
       MAP("no"    , Asp::LogicProgram::mode_native)          , MAP("all" , Asp::LogicProgram::mode_transform),\
       MAP("choice", Asp::LogicProgram::mode_transform_choice), MAP("card", Asp::LogicProgram::mode_transform_card),\
       MAP("weight", Asp::LogicProgram::mode_transform_weight), MAP("scc" , Asp::LogicProgram::mode_transform_scc),\
       MAP("integ" , Asp::LogicProgram::mode_transform_integ) , MAP("dynamic", Asp::LogicProgram::mode_transform_dynamic))),\
       "Configure handling of extended rules\n"\
       "      %A: {all|choice|card|weight|integ|dynamic}\n"\
       "        all    : Transform all extended rules to basic rules\n"\
       "        choice : Transform choice rules, but keep cardinality and weight rules\n"\
       "        card   : Transform cardinality rules, but keep choice and weight rules\n"\
       "        weight : Transform cardinality and weight rules, but keep choice rules\n"\
       "        scc    : Transform \"recursive\" cardinality and weight rules\n"\
       "        integ  : Transform cardinality integrity constraints\n"\
       "        dynamic: Transform \"simple\" extended rules, but keep more complex ones", STORE(SELF.erMode), GET((Asp::LogicProgram::ExtendedRuleMode)SELF.erMode))
GROUP_END(SELF)
#undef CLASP_ASP_OPTIONS
#undef SELF
#endif

//! Options for the solving algorithm (see Clasp::SolveOptions)
#if defined(CLASP_SOLVE_OPTIONS)
#define SELF CLASP_SOLVE_OPTIONS
GROUP_BEGIN(SELF)
OPTION(solve_limit , "", ARG(arg("<n>[,<m>]")), "Stop search after <n> conflicts or <m> restarts\n", FUN(arg) {\
       uint32 n = UINT32_MAX; uint32 m = UINT32_MAX;\
       return ((arg.off() && arg.peek() != '0') || arg>>n>>opt(m)) && (SELF.limit=SolveLimits(n == UINT32_MAX ? UINT64_MAX : n, m == UINT32_MAX ? UINT64_MAX : m), true);},\
       GET((uint32)Range<uint64>(0u,UINT32_MAX).clamp(SELF.limit.conflicts),(uint32)Range<uint64>(0u,UINT32_MAX).clamp(SELF.limit.restarts)))
#if defined(WITH_THREADS) && WITH_THREADS == 1
OPTION(parallel_mode, ",t", ARG_EXT(arg("<arg>"), DEFINE_ENUM_MAPPING(SolveOptions::Algorithm::SearchMode,\
       MAP("compete", SolveOptions::Algorithm::mode_compete), MAP("split", SolveOptions::Algorithm::mode_split))),\
       "Run parallel search with given number of threads\n" \
       "      %A: <n {1..64}>[,<mode {compete|split}>]\n"   \
       "        <n>   : Number of threads to use in search\n"\
       "        <mode>: Run competition or splitting based search [compete]\n", FUN(arg){\
       uint32 n; SolveOptions::Algorithm::SearchMode mode;\
       return arg>>n>>opt(mode = SolveOptions::Algorithm::mode_compete) && SET_R(SELF.algorithm.threads, n, 1u, 64u) && SET(SELF.algorithm.mode, mode);},\
       GET(SELF.algorithm.threads, (SolveOptions::Algorithm::SearchMode)SELF.algorithm.mode))
OPTION(global_restarts, ",@1", ARG(implicit("5")->arg("<X>")), "Configure global restart policy\n" \
       "      %A: <n>[,<sched>] / Implicit: %I\n"                         \
       "        <n> : Maximal number of global restarts (0=disable)\n"    \
       "     <sched>: Restart schedule [x,100,1.5] (<type {F|L|x|+}>)\n", FUN(arg) {\
       return ITE(arg.off(), (SELF.restarts = SolveOptions::GRestarts(), true), arg>>SELF.restarts.maxR>>opt(SELF.restarts.sched = ScheduleStrategy())\
         && SELF.restarts.maxR && SELF.restarts.sched.type != ScheduleStrategy::User);},\
       GET_IF(SELF.restarts.maxR, SELF.restarts.maxR, SELF.restarts.sched))
OPTION(dist_mode  , ",@2" , ARG(defaultsTo("0")->state(Value::value_defaulted)), "Use {0=global|1=thread} distribution", STORE_LEQ(SELF.distribute.mode,1u), GET(SELF.distribute.mode))
OPTION(distribute, "!,@1", ARG_EXT(defaultsTo("conflict,4"), DEFINE_ENUM_MAPPING(Distributor::Policy::Types,\
       MAP("all", Distributor::Policy::all), MAP("short", Distributor::Policy::implicit),\
       MAP("conflict", Distributor::Policy::conflict), MAP("loop" , Distributor::Policy::loop))),\
       "Configure nogood distribution [%D]\n" \
       "      %A: <type>[,<lbd {0..127}>][,<size>]\n"                     \
       "        <type> : Distribute {all|short|conflict|loop} nogoods\n"  \
       "        <lbd>  : Distribute only if LBD  <= <lbd>  [4]\n"         \
       "        <size> : Distribute only if size <= <size> [-1]", FUN(arg) {\
       Distributor::Policy::Types type; uint32 lbd; uint32 size;
       return ITE(arg.off(), (SELF.distribute.policy()=Distributor::Policy(0,0,0), true),\
         arg>>type>>opt(lbd = 4)>>opt(size = UINT32_MAX) && SET(SELF.distribute.types, (uint32)type) && SET(SELF.distribute.lbd, lbd) && SET_OR_FILL(SELF.distribute.size, size));},\
       GET_IF(SELF.distribute.types, (Distributor::Policy::Types)SELF.distribute.types, SELF.distribute.lbd,SELF.distribute.size))
OPTION(integrate, ",@1", ARG_EXT(defaultsTo("gp")->state(Value::value_defaulted),\
       DEFINE_ENUM_MAPPING(SolveOptions::Integration::Filter, \
       MAP("all", SolveOptions::Integration::filter_no), MAP("gp", SolveOptions::Integration::filter_gp),\
       MAP("unsat", SolveOptions::Integration::filter_sat), MAP("active", SolveOptions::Integration::filter_heuristic))\
       DEFINE_ENUM_MAPPING(SolveOptions::Integration::Topology, \
       MAP("all" , SolveOptions::Integration::topo_all) , MAP("ring" , SolveOptions::Integration::topo_ring),\
       MAP("cube", SolveOptions::Integration::topo_cube), MAP("cubex", SolveOptions::Integration::topo_cubex))),\
       "Configure nogood integration [%D]\n" \
       "      %A: <pick>[,<n>][,<topo>]\n"                                           \
       "        <pick>: Add {all|unsat|gp(unsat wrt guiding path)|active} nogoods\n" \
       "        <n>   : Always keep at least last <n> integrated nogoods   [1024]\n" \
       "        <topo>: Accept nogoods from {all|ring|cube|cubex} peers    [all]\n", FUN(arg) {\
       SolveOptions::Integration::Filter pick; uint32 n; SolveOptions::Integration::Topology topo;
       return arg>>pick>>opt(n = 1024)>>opt(topo = SolveOptions::Integration::topo_all) && SET(SELF.integrate.filter, (uint32)pick) && SET_OR_FILL(SELF.integrate.grace, n) && SET(SELF.integrate.topo, (uint32)topo);},\
       GET((SolveOptions::Integration::Filter)SELF.integrate.filter, SELF.integrate.grace, (SolveOptions::Integration::Topology)SELF.integrate.topo))
#endif
OPTION(enum_mode   , ",e", ARG_EXT(defaultsTo("auto")->state(Value::value_defaulted), DEFINE_ENUM_MAPPING(SolveOptions::EnumType,\
       MAP("bt", SolveOptions::enum_bt), MAP("record", SolveOptions::enum_record), MAP("domRec", SolveOptions::enum_dom_record),\
       MAP("brave", SolveOptions::enum_brave), MAP("cautious", SolveOptions::enum_cautious), MAP("query", SolveOptions::enum_query),\
       MAP("auto", SolveOptions::enum_auto), MAP("user", SolveOptions::enum_user))),\
       "Configure enumeration algorithm [%D]\n" \
       "      %A: {bt|record|brave|cautious|auto}\n" \
       "        bt      : Backtrack decision literals from solutions\n" \
       "        record  : Add nogoods for computed solutions\n" \
       "        domRec  : Add nogoods over true domain atoms\n" \
       "        brave   : Compute brave consequences (union of models)\n" \
       "        cautious: Compute cautious consequences (intersection of models)\n" \
       "        auto    : Use bt for enumeration and record for optimization", STORE(SELF.enumMode), GET(SELF.enumMode))
OPTION(opt_mode   , "", ARG_EXT(arg("<mode>"), DEFINE_ENUM_MAPPING(MinimizeMode_t::Mode,\
       MAP("opt" , MinimizeMode_t::optimize), MAP("enum"  , MinimizeMode_t::enumerate),\
       MAP("optN", MinimizeMode_t::enumOpt) , MAP("ignore", MinimizeMode_t::ignore))),\
       "Configure optimization algorithm\n"\
       "      %A: {opt|enum|optN|ignore}\n" \
       "        opt   : Find optimal model\n" \
       "        enum  : Find models with costs <= initial bound\n" \
       "        optN  : Find optimum, then enumerate optimal models\n"\
       "        ignore: Ignore optimize statements", STORE(SELF.optMode), GET((MinimizeMode_t::Mode)SELF.optMode))
OPTION(opt_bound, "!" , ARG(arg("<opt>...")), "Initialize objective function(s)", FUN(arg) {\
       SumVec B; \
       return (arg.off() || arg>>B) && (SELF.optBound.swap(B), true);},\
       GET_IF(!SELF.optBound.empty(), SELF.optBound))
OPTION(opt_sat    , ""   , ARG(flag())         , "Treat DIMACS input as MaxSAT optimization problem", STORE(SELF.maxSat), GET(SELF.maxSat))
OPTION(project    , ""   , ARG(implicit("6"))  , "Project models to named atoms", STORE_LEQ(SELF.project,7u), GET(SELF.project))
OPTION(models     , ",n", ARG(arg("<n>"))      , "Compute at most %A models (0 for all)\n", STORE(SELF.numModels), GET(SELF.numModels))
GROUP_END(SELF)
#undef CLASP_SOLVE_OPTIONS
#undef SELF
#endif

#undef GROUP_BEGIN
#undef GROUP_END
#undef OPTION
#undef NOTIFY_SUBGROUPS
#undef ARG
#undef ARG_EXT
#undef NO_ARG
