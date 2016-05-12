// 
// Copyright (c) 2013, Benjamin Kaufmann
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
 * Supermacros for describing clasp's options.
 * An option consists of:
 *  - a key  (valid and unique c identifier in 'snake_case')
 *  - a key extension ("[!][,<alias>][,@<level>]") as understood by ProgramOptions::OptionInitHelper
 *  - an arg description (ARG macro) as understood by ProgramOptions::Value
 *  - a description (string)
 *  - a set action to be executed when a value (string) for the option is found in a source
 *  - a get action that converts the option's current value to a string
 *  . 
 * OPTION(key, "ext", ARG(...), "help", set, get).
 * 
 * \note In the implementation of ClaspCliConfig, each key is mapped to an enumeration constant and 
 * the stringified version of key (i.e. #key) is used to identify options. 
 * Furthermore, the key is also used for generating command-line option names.
 * As a convention, compound keys using 'snake_case' to separate words 
 * are mapped to dash-separated command-line option names. 
 * E.g. an <option_like_this> is mapped to the command-line option "option-like-this".
 *
 * \note ClaspCliConfig assumes a certain option order. In particular, context options shall
 * precede all solver/search options, which in turn shall precede global asp/solving options.
 *
 * \note The following set actions may be used:
 *  - STORE(obj): converts the string value to the type of obj and stores the result in obj.
 *  - STORE_LEQ(n, max): converts the string value to an unsinged int and stores the result in n if it is <= max.
 *  - STORE_FLAG(n): converts the string value to a bool and stores the result in n as either 0 or 1.
 *  - STORE_OR_FILL(n): converts the string value to an unsinged int t and sets n to std::min(t, maxValue(n)).
 *  - FUN(value): anonymous function of type bool (const char* value).
 *
 * \note The following primitives may be used in the set/get arguments:
 *  - stringTo(str, obj)   : function that converts string str into obj. Evaluates to true iff conversion was successful.
 *  - toString(x1, ...)    : converts the given arguments to a comma-separated string.
 *  - off                  : singleton object representing a valid off value ("no", "off", "false", "0")
 *  - ITE(C, x, y)         : if-then-else expression, i.e. behaves like (C ? x : y).
 *  - TO_STR_IF(C, x1, ...): shorthand for ITE(C, toString(x1,...), toString(off)).
 *  - SET(x, y)            : shorthand for (x=y) == y.
 *  - SET_LEQ(x, v, m)     : shorthand for (x <= m && SET(x, v)).
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
OPTION(stats, ",s"  , ARG(implicit("1")->arg("{0..2}")), "Maintain {0=no|1=basic|2=extended} statistics", STORE_LEQ(SELF.stats,2u), toString(SELF.stats))
OPTION(share, "!,@1", ARG(defaultsTo("auto")->state(Value::value_defaulted), DEFINE_ENUM_MAPPING(ContextParams::ShareMode, \
       MAP("no"  , ContextParams::share_no)  , MAP("all", ContextParams::share_all),\
       MAP("auto", ContextParams::share_auto), MAP("problem", ContextParams::share_problem),\
       MAP("learnt", ContextParams::share_learnt))),\
       "Configure physical sharing of constraints [%D]\n"\
       "      %A: {auto|problem|learnt|all}", FUN(str) {ContextParams::ShareMode arg; return stringTo(str, arg) && SET(SELF.shareMode, (uint32)arg);}, toString((ContextParams::ShareMode)SELF.shareMode))
OPTION(learn_explicit, ",@1" , ARG(flag()), "Do not use Short Implication Graph for learning\n", STORE_FLAG(SELF.shortMode), toString(SELF.shortMode))
OPTION(sat_prepro    , "!,@1", ARG(implicit("2")), "Run SatELite-like preprocessing (Implicit: %I)\n" \
       "      %A: <type>[,<limit>...(0=no limit)]\n"                \
       "        <type>: Run {1=VE|2=VE with BCE|3=BCE followed by VE}\n"\
       "        <x1>  : Set iteration limit to <x1>           [0]\n"\
       "        <x2>  : Set variable occurrence limit to <x2> [0]\n"\
       "        <x3>  : Set time limit to <x3> seconds        [0]\n"\
       "        <x4>  : Set frozen variables limit to <x4>%%   [0]\n"\
       "        <x5>  : Set clause limit to <x5>*1000      [4000]", FUN(str) {\
       SatPreParams arg; \
       return stringTo(str, arg | off) && (SELF.satPre = arg, true);}, TO_STR_IF(SELF.satPre.type, SELF.satPre))
GROUP_END(SELF)
#undef CLASP_CONTEXT_OPTIONS
#undef SELF
#endif

//! Solver options (see SolverParams).
#if defined(CLASP_SOLVER_OPTIONS)
#define SELF CLASP_SOLVER_OPTIONS
GROUP_BEGIN(SELF)
OPTION(opt_strategy , ""  , ARG(arg("<arg>")->implicit("1"), DEFINE_ENUM_MAPPING(MinimizeMode_t::Strategy,\
       MAP("bb", MinimizeMode_t::opt_bb), MAP("usc", MinimizeMode_t::opt_usc))),  "Configure optimization strategy\n" \
       "      %A: {bb|usc}[,<n>]\n" \
       "        bb : branch and bound based optimization with <n = {0..3}>\n"  \
       "          1: hierarchical steps\n"                                     \
       "          2: exponentially increasing steps\n"                         \
       "          3: exponentially decreasing steps\n"                         \
       "        usc: unsatisfiable-core based optimization with <n = {0..7}>\n"\
       "          1: disjoint-core preprocessing\n"                            \
       "          2: implications instead of equivalences\n"                   \
       "          4: clauses instead of cardinality constraints"               \
       , FUN(str) { ARG_T(MinimizeMode_t::Strategy, uint32) arg(MinimizeMode_t::opt_bb, 0); uint32 n;\
         return ITE(stringTo(str, arg), SET(SELF.optStrat, (uint32)arg.first) && SET(SELF.optParam, arg.second) && (arg.first != 0 || arg.second < 4),\
                    stringTo(str, n) && SET(SELF.optStrat, uint32(n>3)) && SET(SELF.optParam, (n > 3 ? n - 4:n)) && n <= 11u);}\
       , toString(static_cast<MinimizeMode_t::Strategy>(SELF.optStrat), SELF.optParam))
OPTION(opt_heuristic, ""  , ARG(implicit("1")->arg("{0..3}")), "Use opt. in {1=sign|2=model|3=both} heuristics", STORE_LEQ(SELF.optHeu,  3u), toString(SELF.optHeu))
OPTION(restart_on_model, "", ARG(flag()), "Restart after each model\n", STORE_FLAG(SELF.restartOnModel), toString(SELF.restartOnModel))
OPTION(lookahead    , "!", ARG(implicit("atom"), DEFINE_ENUM_MAPPING(Lookahead::Type, \
       MAP("atom", Lookahead::atom_lookahead), MAP("body", Lookahead::body_lookahead), MAP("hybrid", Lookahead::hybrid_lookahead))),\
       "Configure failed-literal detection (fld)\n" \
       "      %A: <type>[,<limit>] / Implicit: %I\n" \
       "        <type> : Run fld via {atom|body|hybrid} lookahead\n" \
       "        <limit>: Disable fld after <limit> applications ([0]=no limit)\n" \
       "      --lookahead=atom is default if --no-lookback is used\n", FUN(str) { \
       ARG_T(Lookahead::Type, uint32) arg(Lookahead::no_lookahead,0);\
       return stringTo(str, arg | off) && SET(SELF.lookType, (uint32)arg.first) && SET_OR_ZERO(SELF.lookOps, arg.second);},\
       TO_STR_IF(SELF.lookType, (Lookahead::Type)SELF.lookType, SELF.lookOps))
OPTION(heuristic, "", ARG(arg("<heu>"), DEFINE_ENUM_MAPPING(Heuristic_t::Type, \
       MAP("berkmin", Heuristic_t::heu_berkmin), MAP("vmtf"  , Heuristic_t::heu_vmtf), \
       MAP("vsids"  , Heuristic_t::heu_vsids)  , MAP("domain", Heuristic_t::heu_domain), \
       MAP("unit"   , Heuristic_t::heu_unit)   , MAP("none"  , Heuristic_t::heu_none))), \
       "Configure decision heuristic\n"  \
       "      %A: {Berkmin|Vmtf|Vsids|Domain|Unit|None}[,<n>]\n" \
       "        Berkmin: Use BerkMin-like heuristic (Check last <n> nogoods [0]=all)\n" \
       "        Vmtf   : Use Siege-like heuristic (Move <n> literals to the front [8])\n" \
       "        Vsids  : Use Chaff-like heuristic (Use 1.0/0.<n> as decay factor  [95])\n"\
       "        Domain : Use domain knowledge to Vsids-like heuristic\n"\
       "        Unit   : Use Smodels-like heuristic (Default if --no-lookback)\n" \
       "        None   : Select the first free variable", FUN(str) { ARG_T(Heuristic_t::Type, uint32) arg(Heuristic_t::heu_default,0); \
       return stringTo(str, arg) && SET(SELF.heuId, (uint32)arg.first) && (Heuristic_t::isLookback(arg.first) || !arg.second) && SET_OR_FILL(SELF.heuParam, arg.second);},\
       toString((Heuristic_t::Type)SELF.heuId, SELF.heuParam))
OPTION(init_moms  , "!,@2", ARG(flag())    , "Initialize heuristic with MOMS-score", STORE_FLAG(SELF.heuMoms), toString(SELF.heuMoms))
OPTION(score_res  , ",@2" , ARG(arg("<n>")), "Resolution score {0=default|1=min|2=set|3=multiset}", STORE_LEQ(SELF.heuScore,3u), toString(SELF.heuScore))
OPTION(score_other, ",@2" , ARG(arg("<n>")), "Score {0=no|1=loop|2=all} other learnt nogoods", STORE_LEQ(SELF.heuOther,2u), toString(SELF.heuOther))
OPTION(sign_def   , ""    , ARG(arg("<n>")), "Default sign: {0=asp|1=pos|2=neg|3=rnd|4=disj}", STORE_LEQ(SELF.signDef,4u), toString(SELF.signDef))
OPTION(sign_fix   , "!"   , ARG(flag())    , "Disable sign heuristics and use default signs only", STORE_FLAG(SELF.signFix), toString(SELF.signFix))
OPTION(berk_huang , "!,@2", ARG(flag())    , "Enable/Disable Huang-scoring in Berkmin", STORE_FLAG(SELF.berkHuang), toString(SELF.berkHuang))
OPTION(nant       , "!,@2", ARG(flag())    , "In Unit count only atoms in NAnt(P)", STORE_FLAG(SELF.unitNant), toString(SELF.unitNant))
OPTION(dom_mod    , ",@2" , NO_ARG         , "Default modification for domain heuristic\n"\
       "      %A: <mod>[,<pick>]\n"\
       "        <mod>  : Default modification {0=no|1=level|2=pos|3=true|4=neg|5=false}\n"\
       "        <pick> : Apply <mod> to {0=all|1=scc|2=hcc|4=disj|8=min|16=show} atoms",\
       FUN(str) { ARG_T(uint32, uint32) arg(0,0); return stringTo(str, arg) && SET_LEQ(SELF.domMod, arg.first, 5u) && SET(SELF.domPref, arg.second);},\
       toString(SELF.domMod, SELF.domPref))
OPTION(save_progress, "", ARG(implicit("1")->arg("<n>")), "Use RSat-like progress saving on backjumps > %A", STORE_OR_FILL(SELF.saveProgress), toString(SELF.saveProgress))
OPTION(init_watches , ",@2", ARG(arg("{0..2}")->defaultsTo("1")->state(Value::value_defaulted)),\
       "Configure watched literal initialization [%D]\n" \
       "      Watch {0=first|1=random|2=least watched} literals in nogoods", STORE_LEQ(SELF.initWatches, 2u), toString(SELF.initWatches))
OPTION(update_mode   , ",@2", ARG(arg("<n>")), "Process messages on {0=propagation|1=conflict)", STORE_LEQ(SELF.upMode, 1u), toString(SELF.upMode))
OPTION(seed          , ""   , ARG(arg("<n>")),"Set random number generator's seed to %A", STORE(SELF.seed), toString(SELF.seed))
OPTION(no_lookback   , ""   , ARG(flag()), "Disable all lookback strategies\n", STORE_FLAG(SELF.search),toString(static_cast<bool>(SELF.search == SolverStrategies::no_learning)))
OPTION(forget_on_step, ""   , ARG(arg("<bits>")), "Configure forgetting on (incremental) step\n"\
       "      Forget {1=heuristic|2=signs|4=nogood activities|8=learnt nogoods}\n", STORE_LEQ(SELF.forgetSet, 15u), toString(SELF.forgetSet))
OPTION(strengthen    , "!"  , ARG(arg("<X>"), DEFINE_ENUM_MAPPING(SolverStrategies::CCMinType, \
       MAP("local", SolverStrategies::cc_local), MAP("recursive", SolverStrategies::cc_recursive))), \
       "Use MiniSAT-like conflict nogood strengthening\n" \
       "      %A: <mode>[,<type>]\n" \
       "        <mode>: Use {local|recursive} self-subsumption check\n" \
       "        <type>: Follow {0=all|1=short|2=binary} antecedents  [0]", FUN(str) {\
       ARG_T(SolverStrategies::CCMinType, uint32) arg(SolverStrategies::cc_local, SolverParams::no_antes);\
       return stringTo(str, arg | off) && (arg.empty() || ++arg.second <= 3u) && SET(SELF.ccMinAntes, arg.second) && SET(SELF.ccMinRec, (uint32)arg.first);},\
       TO_STR_IF(SELF.ccMinAntes, (SolverStrategies::CCMinType)SELF.ccMinRec, SELF.ccMinAntes-uint32(1)))
OPTION(otfs        , ""   , ARG(implicit("1")->arg("{0..2}")), "Enable {1=partial|2=full} on-the-fly subsumption", STORE_LEQ(SELF.otfs, 2u), toString(SELF.otfs))
OPTION(update_lbd  , ",@2", ARG(implicit("1")->arg("{0..3}")), "Update LBDs of learnt nogoods {1=<|2=strict<|3=+1<}", STORE_LEQ(SELF.updateLbd, 3u),toString(SELF.updateLbd))
OPTION(update_act  , ",@2", ARG(flag()), "Enable LBD-based activity bumping", STORE_FLAG(SELF.bumpVarAct), toString(SELF.bumpVarAct))
OPTION(reverse_arcs, ""   , ARG(implicit("1")->arg("{0..3}")), "Enable ManySAT-like inverse-arc learning", STORE_LEQ(SELF.reverseArcs, 3u), toString(SELF.reverseArcs))
OPTION(contraction , "!"  , NO_ARG, "Configure handling of long learnt nogoods\n"
       "      %A: <n>[,<rep>]\n"\
       "        <n>  : Contract nogoods if size > <n> (0=disable)\n"\
       "        <rep>: Replace literal blocks with {1=decisions|2=uips} ([0]=disable)\n", FUN(str) {ARG_T(uint32, uint32) arg(0,0);\
       return stringTo(str, arg | off) && (arg.parsed() < 2 || arg.first != 0u) && SET_OR_FILL(SELF.compress, arg.first) && SET_LEQ(SELF.ccRepMode, arg.second,3u);},\
       TO_STR_IF(SELF.compress, SELF.compress, SELF.ccRepMode))
OPTION(loops, "" , ARG(arg("<type>"), DEFINE_ENUM_MAPPING(DefaultUnfoundedCheck::ReasonStrategy,\
       MAP("common"  , DefaultUnfoundedCheck::common_reason)  , MAP("shared", DefaultUnfoundedCheck::shared_reason), \
       MAP("distinct", DefaultUnfoundedCheck::distinct_reason), MAP("no", DefaultUnfoundedCheck::only_reason))),\
       "Configure learning of loop nogoods\n" \
       "      %A: {common|distinct|shared|no}\n" \
       "        common  : Create loop nogoods for atoms in an unfounded set\n" \
       "        distinct: Create distinct loop nogood for each atom in an unfounded set\n" \
       "        shared  : Create loop formula for a whole unfounded set\n" \
       "        no      : Do not learn loop formulas", FUN(str) {DefaultUnfoundedCheck::ReasonStrategy arg; return stringTo(str, arg) && SET(SELF.loopRep, (uint32)arg);},\
       toString(static_cast<DefaultUnfoundedCheck::ReasonStrategy>(SELF.loopRep)))
GROUP_END(SELF)
#undef CLASP_SOLVER_OPTIONS
#undef SELF
#endif

//! Search-related options (see SolveParams).
#if defined(CLASP_SEARCH_OPTIONS)
#define SELF CLASP_SEARCH_OPTIONS
GROUP_BEGIN(SELF)
OPTION(partial_check, "", ARG(implicit("50")), "Configure partial stability tests\n" \
       "      %A: <p>[,<h>][,<x>] / Implicit: %I\n" \
       "        <p>: Partial check percentage\n"    \
       "        <h>: Initial value for high bound (0 = umax)\n" \
       "        <x>: Increase (1) or keep (0) high bound once reached", FUN(str) {\
       ARG_T(uint32, uint32, uint32) arg(0,0,0);\
       return stringTo(str, arg | off) && (arg.parsed() < 2 || arg.first != 0u) && SET_LEQ(SELF.fwdCheck.highPct, arg.first, 100u) && SET_OR_ZERO(SELF.fwdCheck.initHigh, arg.second) && SET_LEQ(SELF.fwdCheck.incHigh, arg.third, 1u);},\
       TO_STR_IF(SELF.fwdCheck.highPct, SELF.fwdCheck.highPct, SELF.fwdCheck.initHigh, SELF.fwdCheck.incHigh))
OPTION(rand_freq, "", ARG(arg("<p>")), "Make random decisions with probability %A", FUN(str) {\
       double f = 0.0; \
       return stringTo(str, f | off) && SET_R(SELF.randProb, (float)f, 0.0f, 1.0f);}, toString(SELF.randProb))
OPTION(rand_prob, "!", ARG(implicit("10,100")), "Configure random probing (Implicit: %I)\n" \
       "      %A: <n1>[,<n2>]\n" \
       "        Run <n1> random passes with at most <n2> conflicts each", FUN(str) {\
       ARG_T(uint32, uint32) arg(0,100);\
       return stringTo(str, arg | off) && (arg.parsed() < 2 || arg.first != 0u) && SET_OR_FILL(SELF.randRuns, arg.first) && SET_OR_FILL(SELF.randConf, arg.second);},\
       TO_STR_IF(SELF.randRuns, SELF.randRuns,SELF.randConf))
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
       "      no|0       : Disable restarts", FUN(str) { return ITE(stringTo(str, off), (SELF.disable(),true), \
       stringTo(str, SELF.sched) && SET(SELF.dynRestart, uint32(SELF.sched.type == ScheduleStrategy::user_schedule)));}, toString(SELF.sched))
OPTION(reset_restarts  , ""   , ARG(arg("0..2")->implicit("1")), "{0=Keep|1=Reset|2=Disable} restart seq. after model", STORE_LEQ(SELF.upRestart, 2u), toString(SELF.upRestart))
OPTION(local_restarts  , ""   , ARG(flag()), "Use Ryvchin et al.'s local restarts", STORE_FLAG(SELF.cntLocal), toString(SELF.cntLocal))
OPTION(counter_restarts, ""   , ARG(arg("<n>")), "Do a counter implication restart every <n> restarts", STORE_OR_FILL(SELF.counterRestart),toString(SELF.counterRestart))
OPTION(counter_bump    , ",@2", ARG(arg("<n>"))    , "Set CIR bump factor to %A", STORE_OR_FILL(SELF.counterBump), toString(SELF.counterBump))
OPTION(shuffle         , "!"  , ARG(arg("<n1>,<n2>")), "Shuffle problem after <n1>+(<n2>*i) restarts\n", FUN(str) {ARG_T(uint32, uint32) arg(0,0);\
       return stringTo(str, arg|off) && (arg.parsed() < 2 || arg.first) && SET_OR_FILL(SELF.shuffle, arg.first) && SET_OR_FILL(SELF.shuffleNext, arg.second);},\
       TO_STR_IF(SELF.shuffle, SELF.shuffle, SELF.shuffleNext))
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
OPTION(deletion    , "!,d", ARG(defaultsTo("basic,75,0")->state(Value::value_defaulted), DEFINE_ENUM_MAPPING(ReduceStrategy::Algorithm,\
       MAP("basic", ReduceStrategy::reduce_linear), MAP("sort", ReduceStrategy::reduce_stable),\
       MAP("ipSort", ReduceStrategy::reduce_sort) , MAP("ipHeap", ReduceStrategy::reduce_heap))),
       "Configure deletion algorithm [%D]\n" \
       "      %A: <algo>[,<n {1..100}>][,<sc>]\n"  \
       "        <algo>: Use {basic|sort|ipSort|ipHeap} algorithm\n" \
       "        <n>   : Delete at most <n>%% of nogoods on reduction    [75]\n" \
       "        <sc>  : Use {0=activity|1=lbd|2=combined} nogood scores [0]\n" \
       "      no      : Disable nogood deletion", FUN(str){\
       ARG_T(ReduceStrategy::Algorithm, uint32, uint32) arg(ReduceStrategy::reduce_linear, 75, 0);\
       return stringTo(str, arg | off) && ITE(arg.empty(), (SELF.disable(), true),\
         SET(SELF.strategy.algo, (uint32)arg.first) && SET_R(SELF.strategy.fReduce, arg.second, 1, 100) && SET_LEQ(SELF.strategy.score, arg.third, 2));\
       }, TO_STR_IF(SELF.strategy.fReduce, (ReduceStrategy::Algorithm)SELF.strategy.algo, SELF.strategy.fReduce,SELF.strategy.score))
OPTION(del_grow    , "!", NO_ARG, "Configure size-based deletion policy\n" \
       "      %A: <f>[,<g>][,<sched>] (<f> >= 1.0)\n"          \
       "        <f>     : Keep at most T = X*(<f>^i) learnt nogoods with X being the\n"\
       "                  initial limit and i the number of times <sched> fired\n"     \
       "        <g>     : Stop growth once T > P*<g> (0=no limit)      [3.0]\n"        \
       "        <sched> : Set grow schedule (<type {F|L|x|+}>) [grow on restart]", FUN(str){\
       ARG_T(double, double, ScheduleStrategy) arg(1.0, 3.0, SELF.growSched);\
       return stringTo(str, off | arg) && ITE(arg.empty(), (SELF.growSched = ScheduleStrategy::none(), SELF.fGrow = 0.0f, true),\
         (arg.third.defaulted() || arg.third.type != ScheduleStrategy::user_schedule) && SET_R(SELF.fGrow, (float)arg.first, 1.0f, FLT_MAX) && SET_R(SELF.fMax, (float)arg.second, 0.0f, FLT_MAX) && (SELF.growSched = arg.third, true));\
       }, ITE(SELF.fGrow == 0.0f, toString("no"), ITE(SELF.growSched.disabled(), toString(SELF.fGrow, SELF.fMax), toString(SELF.fGrow, SELF.fMax, SELF.growSched))))
OPTION(del_cfl     , "!", ARG(arg("<sched>")), "Configure conflict-based deletion policy\n" \
       "      %A:   <type {F|L|x|+}>,<args>... (see restarts)", FUN(str){\
       return ITE(stringTo(str, off), (SELF.cflSched=ScheduleStrategy::none()).disabled(), stringTo(str, SELF.cflSched) && SELF.cflSched.type != ScheduleStrategy::user_schedule);}, toString(SELF.cflSched))
OPTION(del_init  , ""  , ARG(defaultsTo("3.0")->state(Value::value_defaulted)), "Configure initial deletion limit\n"\
       "      %A: <f>[,<n>,<o>] (<f> > 0)\n" \
       "        <f>    : Set initial limit to P=estimated problem size/<f> [%D]\n" \
       "        <n>,<o>: Clamp initial limit to the range [<n>,<n>+<o>]" , FUN(str) {\
       ARG_T(double, uint32, uint32) arg(3.0, SELF.initRange.lo, SELF.initRange.hi);\
       return stringTo(str, arg) && arg.first > 0 && (SELF.fInit = float(1.0 / arg.first)) > 0 && SET(SELF.initRange.lo, arg.second) && SET_OR_FILL(SELF.initRange.hi, (uint64(SELF.initRange.lo)+arg.third));\
       }, TO_STR_IF(SELF.fInit, 1.0/SELF.fInit, SELF.initRange.lo, SELF.initRange.hi - SELF.initRange.lo))
OPTION(del_estimate, "", ARG(arg("0..3")->implicit("1")), "Use estimated problem complexity in limits", STORE_LEQ(SELF.strategy.estimate, 3u), toString(SELF.strategy.estimate))
OPTION(del_max     , "", ARG(arg("<n>,<X>")), "Keep at most <n> learnt nogoods taking up to <X> MB", FUN(str) { ARG_T(uint32, uint32) arg(UINT32_MAX,0); \
       return stringTo(str, off | arg) && SET_R(SELF.maxRange, arg.first, 1u, UINT32_MAX) && SET(SELF.memMax, arg.second);}, toString(SELF.maxRange, SELF.memMax))
OPTION(del_glue    , "", NO_ARG, "Configure glue clause handling\n" \
       "      %A: <n {0..127}>[,<m {0|1}>]\n"                                    \
       "        <n>: Do not delete nogoods with LBD <= <n>\n"                    \
       "        <m>: Count (0) or ignore (1) glue clauses in size limit [0]", FUN(str) {ARG_T(uint32, uint32) arg(0, 0); \
       return stringTo(str, arg) && SET_LEQ(SELF.strategy.glue, arg.first, (uint32)Activity::MAX_LBD) && SET(SELF.strategy.noGlue, arg.second);}, toString(SELF.strategy.glue, SELF.strategy.noGlue))
OPTION(del_on_restart, "", ARG(arg("<n>")->implicit("33")), "Delete %A%% of learnt nogoods on each restart", STORE_LEQ(SELF.strategy.fRestart, 100u), toString(SELF.strategy.fRestart))
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
OPTION(supp_models, ",@1" , ARG(flag())    , "Compute supported models (no unfounded set check)", STORE_FLAG(SELF.suppMod), toString(SELF.suppMod))
OPTION(eq         , ""    , ARG(arg("<n>")), "Configure equivalence preprocessing\n" \
       "      Run for at most %A iterations (-1=run to fixpoint)", STORE_OR_FILL(SELF.iters), toString(SELF.iters))
OPTION(backprop   , "!,@1", ARG(flag())    , "Use backpropagation in ASP-preprocessing", STORE_FLAG(SELF.backprop), toString(SELF.backprop))
OPTION(no_gamma   , ",@1" , ARG(flag())    , "Do not add gamma rules for non-hcf disjunctions", STORE_FLAG(SELF.noGamma), toString(SELF.noGamma))
OPTION(eq_dfs     , ",@2" , ARG(flag())    , "Enable df-order in eq-preprocessing", STORE_FLAG(SELF.dfOrder), toString(SELF.dfOrder))
OPTION(freeze_shown,",@3" , ARG(flag())    , "Freeze all shown atoms", STORE_FLAG(SELF.freezeShown), toString(SELF.freezeShown))
OPTION(trans_ext  , "!", ARG(arg("<mode>"), DEFINE_ENUM_MAPPING(Asp::LogicProgram::ExtendedRuleMode,\
       MAP("no"    , Asp::LogicProgram::mode_native)          , MAP("all" , Asp::LogicProgram::mode_transform),\
       MAP("choice", Asp::LogicProgram::mode_transform_choice), MAP("card", Asp::LogicProgram::mode_transform_card),\
       MAP("weight", Asp::LogicProgram::mode_transform_weight), MAP("scc" , Asp::LogicProgram::mode_transform_scc),\
       MAP("integ" , Asp::LogicProgram::mode_transform_integ) , MAP("dynamic", Asp::LogicProgram::mode_transform_dynamic))),\
       "Configure handling of Lparse-like extended rules\n"\
       "      %A: {all|choice|card|weight|integ|dynamic}\n"\
       "        all    : Transform all extended rules to basic rules\n"\
       "        choice : Transform choice rules, but keep cardinality and weight rules\n"\
       "        card   : Transform cardinality rules, but keep choice and weight rules\n"\
       "        weight : Transform cardinality and weight rules, but keep choice rules\n"\
       "        scc    : Transform \"recursive\" cardinality and weight rules\n"\
       "        integ  : Transform cardinality integrity constraints\n"\
       "        dynamic: Transform \"simple\" extended rules, but keep more complex ones", STORE(SELF.erMode), toString((Asp::LogicProgram::ExtendedRuleMode)SELF.erMode))
GROUP_END(SELF)
#undef CLASP_ASP_OPTIONS
#undef SELF
#endif

//! Options for the solving algorithm (see Clasp::SolveOptions)
#if defined(CLASP_SOLVE_OPTIONS)
#define SELF CLASP_SOLVE_OPTIONS
GROUP_BEGIN(SELF)
OPTION(solve_limit , "", ARG(arg("<n>[,<m>]")), "Stop search after <n> conflicts or <m> restarts\n", FUN(str) {\
       ARG_T(uint32, uint32) arg(UINT32_MAX, UINT32_MAX);\
       return stringTo(str, arg | off) && (SELF.limit=SolveLimits(arg.first == UINT32_MAX ? UINT64_MAX : arg.first, arg.second == UINT32_MAX ? UINT64_MAX : arg.second), true);},\
       toString((uint32)Range<uint64>(0u,UINT32_MAX).clamp(SELF.limit.conflicts),(uint32)Range<uint64>(0u,UINT32_MAX).clamp(SELF.limit.restarts)))
#if defined(WITH_THREADS) && WITH_THREADS == 1
OPTION(parallel_mode, ",t", ARG(arg("<arg>"), DEFINE_ENUM_MAPPING(SolveOptions::Algorithm::SearchMode,\
       MAP("compete", SolveOptions::Algorithm::mode_compete), MAP("split", SolveOptions::Algorithm::mode_split))),\
       "Run parallel search with given number of threads\n" \
       "      %A: <n {1..64}>[,<mode {compete|split}>]\n"   \
       "        <n>   : Number of threads to use in search\n"\
       "        <mode>: Run competition or splitting based search [compete]\n", FUN(str){\
       ARG_T(uint32, SolveOptions::Algorithm::SearchMode) arg(1,SolveOptions::Algorithm::mode_compete);\
       return stringTo(str, arg) && SET_R(SELF.algorithm.threads, arg.first, 1u, 64u) && SET(SELF.algorithm.mode, arg.second);},\
       toString(SELF.algorithm.threads, (SolveOptions::Algorithm::SearchMode)SELF.algorithm.mode))
OPTION(global_restarts, ",@1", ARG(implicit("5")->arg("<X>")), "Configure global restart policy\n" \
       "      %A: <n>[,<sched>] / Implicit: %I\n"                         \
       "        <n> : Maximal number of global restarts (0=disable)\n"    \
       "     <sched>: Restart schedule [x,100,1.5] (<type {F|L|x|+}>)\n", FUN(str) {\
       ARG_T(uint32, Clasp::ScheduleStrategy) arg;\
       return stringTo(str, arg | off) && (arg.parsed() < 2 || arg.first != 0u) && (SELF.restarts.sched=arg.second).type != ScheduleStrategy::user_schedule && SET(SELF.restarts.maxR, arg.first);},\
       TO_STR_IF(SELF.restarts.maxR, SELF.restarts.maxR, SELF.restarts.sched))
OPTION(dist_mode  , ",@2" , ARG(defaultsTo("0")->state(Value::value_defaulted)), "Use {0=global|1=thread} distribution", STORE_LEQ(SELF.distribute.mode,1u), toString(SELF.distribute.mode))
OPTION(distribute, "!,@1", ARG(defaultsTo("conflict,4"), DEFINE_ENUM_MAPPING(Distributor::Policy::Types,\
       MAP("all", Distributor::Policy::all), MAP("short", Distributor::Policy::implicit),\
       MAP("conflict", Distributor::Policy::conflict), MAP("loop" , Distributor::Policy::loop))),\
       "Configure nogood distribution [%D]\n" \
       "      %A: <type>[,<lbd {0..127}>][,<size>]\n"                     \
       "        <type> : Distribute {all|short|conflict|loop} nogoods\n"  \
       "        <lbd>  : Distribute only if LBD  <= <lbd>  [4]\n"         \
       "        <size> : Distribute only if size <= <size> [-1]", FUN(str) {\
       ARG_T(Distributor::Policy::Types, uint32, uint32) arg(Distributor::Policy::all, 4, UINT32_MAX);\
       return stringTo(str, arg | off) && ITE(arg.empty(), (SELF.distribute.policy()=Distributor::Policy(0,0,0), true),\
         SET(SELF.distribute.types, (uint32)arg.first) && SET(SELF.distribute.lbd, arg.second) && SET_OR_FILL(SELF.distribute.size, arg.third));\
       }, TO_STR_IF(SELF.distribute.types, (Distributor::Policy::Types)SELF.distribute.types, SELF.distribute.lbd,SELF.distribute.size))
OPTION(integrate, ",@1", ARG(defaultsTo("gp")->state(Value::value_defaulted), COMBINE_2(\
       DEFINE_ENUM_MAPPING(SolveOptions::Integration::Filter, \
       MAP("all", SolveOptions::Integration::filter_no), MAP("gp", SolveOptions::Integration::filter_gp),\
       MAP("unsat", SolveOptions::Integration::filter_sat), MAP("active", SolveOptions::Integration::filter_heuristic)),\
       DEFINE_ENUM_MAPPING(SolveOptions::Integration::Topology, \
       MAP("all" , SolveOptions::Integration::topo_all) , MAP("ring" , SolveOptions::Integration::topo_ring),\
       MAP("cube", SolveOptions::Integration::topo_cube), MAP("cubex", SolveOptions::Integration::topo_cubex)))),\
       "Configure nogood integration [%D]\n" \
       "      %A: <pick>[,<n>][,<topo>]\n"                                           \
       "        <pick>: Add {all|unsat|gp(unsat wrt guiding path)|active} nogoods\n" \
       "        <n>   : Always keep at least last <n> integrated nogoods   [1024]\n" \
       "        <topo>: Accept nogoods from {all|ring|cube|cubex} peers    [all]\n", FUN(str) {\
       ARG_T(SolveOptions::Integration::Filter, uint32, SolveOptions::Integration::Topology) arg(SolveOptions::Integration::filter_no, 1024, SolveOptions::Integration::topo_all);\
       return stringTo(str, arg)&& SET(SELF.integrate.filter, (uint32)arg.first) && SET_OR_FILL(SELF.integrate.grace, arg.second) && SET(SELF.integrate.topo, (uint32)arg.third);\
       }, toString((SolveOptions::Integration::Filter)SELF.integrate.filter, SELF.integrate.grace, (SolveOptions::Integration::Topology)SELF.integrate.topo))
#endif
OPTION(enum_mode   , ",e", ARG(defaultsTo("auto")->state(Value::value_defaulted), DEFINE_ENUM_MAPPING(SolveOptions::EnumType,\
       MAP("bt", SolveOptions::enum_bt), MAP("record", SolveOptions::enum_record), MAP("domRec", SolveOptions::enum_dom_record),\
       MAP("brave", SolveOptions::enum_brave), MAP("cautious", SolveOptions::enum_cautious),\
       MAP("auto", SolveOptions::enum_auto), MAP("user", SolveOptions::enum_user))),\
       "Configure enumeration algorithm [%D]\n" \
       "      %A: {bt|record|brave|cautious|auto}\n" \
       "        bt      : Backtrack decision literals from solutions\n" \
       "        record  : Add nogoods for computed solutions\n" \
       "        domRec  : Add nogoods over true domain atoms\n" \
       "        brave   : Compute brave consequences (union of models)\n" \
       "        cautious: Compute cautious consequences (intersection of models)\n" \
       "        auto    : Use bt for enumeration and record for optimization", STORE(SELF.enumMode), toString(SELF.enumMode))
OPTION(opt_mode   , "", ARG(arg("<mode>"), DEFINE_ENUM_MAPPING(MinimizeMode_t::Mode,\
       MAP("opt" , MinimizeMode_t::optimize), MAP("enum"  , MinimizeMode_t::enumerate),\
       MAP("optN", MinimizeMode_t::enumOpt) , MAP("ignore", MinimizeMode_t::ignore))),\
       "Configure optimization algorithm\n"\
       "      %A: {opt|enum|optN|ignore}\n" \
       "        opt   : Find optimal model\n" \
       "        enum  : Find models with costs <= initial bound\n" \
       "        optN  : Find optimum, then enumerate optimal models\n"\
       "        ignore: Ignore optimize statements", STORE(SELF.optMode), toString((MinimizeMode_t::Mode)SELF.optMode))
OPTION(opt_bound, "!" , ARG(arg("<opt>...")), "Initialize objective function(s)", FUN(str) {\
       SumVec B; \
       return stringTo(str, B | off) && (SELF.optBound.swap(B), true); },\
       TO_STR_IF(!SELF.optBound.empty(), SELF.optBound))
OPTION(opt_sat  , ""  , ARG(flag())         , "Treat DIMACS input as MaxSAT optimization problem", STORE(SELF.maxSat), toString(SELF.maxSat))
OPTION(project , ""  , ARG(implicit("6"))  , "Project models to named atoms", STORE_LEQ(SELF.project,7u), toString(SELF.project))
OPTION(models  , ",n", ARG(arg("<n>"))     , "Compute at most %A models (0 for all)\n", STORE(SELF.numModels), toString(SELF.numModels))
GROUP_END(SELF)
#undef CLASP_SOLVE_OPTIONS
#undef SELF
#endif

#undef GROUP_BEGIN
#undef GROUP_END
#undef OPTION
#undef NOTIFY_SUBGROUPS
#undef ARG
#undef NO_ARG
