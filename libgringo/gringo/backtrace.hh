// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
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

#ifndef _GRINGO_BACKTRACE_HH
#define _GRINGO_BACKTRACE_HH

#include <libunwind.h>
#include <cstring>
#include <cxxabi.h>
#include <cstdio>
#include <unistd.h>
#include <unordered_map>
#include <string>
#include <cstdlib>

// TODO: given a little time this could be cleaned up
//       to remove the ugly fixed size strings
//       anyway this is just debug code...

namespace Gringo {

inline const char* getExecutableName() {
    static char* exe = 0;
    if (!exe) {
        char link[4096];
        static char _exe[4096];
        _exe[0] = '\0';
        snprintf(link, sizeof(link), "/proc/%d/exe", getpid());
        readlink(link, _exe, sizeof(link));
        exe = _exe;
    }
    return exe;
}

inline int getFileAndLine (unw_word_t addr, char *file, int *line) {
    char buf[4096];
    static std::unordered_map<unw_word_t, std::string> cache;
    std::string &s = cache[addr];
    if (!s.empty()) {
        strncpy(buf, s.c_str(), sizeof(buf));
    }
    else {
        snprintf (buf, sizeof(buf), "/home/wv/bin/linux/64/binutils-2.23.1/bin/addr2line -C -e %s -f -i %lx", getExecutableName(), addr);
        FILE* f = popen (buf, "r");
        if (f == NULL) {
            perror (buf);
            return 0;
        }
        fgets (buf, sizeof(buf), f);
        fgets (buf, sizeof(buf), f);
        pclose(f);
        s = buf;
        const char *pref = "/home/kaminski/svn/wv/Programming/gringo/trunk/";
        if (strncmp(pref, s.c_str(), strlen(pref)) == 0) {
            s = buf + strlen(pref);
            strncpy(buf, s.c_str(), sizeof(buf));
        }
    }
    if (buf[0] != '?') {
        char *p = buf;
        while (*p != ':') { p++; }
        *p++ = 0;
        strncpy (file , buf, 4096);
        sscanf (p,"%d", line);
    }
    else {
        strncpy (file,"unkown", 4096);
        *line = 0;
    }
    return 1;
}

inline void showBacktrace (void) {
    char name[4096];
    int  status;
    unw_cursor_t cursor; unw_context_t uc;
    unw_word_t ip, sp, offp;

    unw_getcontext(&uc);
    unw_init_local(&cursor, &uc);

    while (unw_step(&cursor) > 0) {
        char file[4096];
        int line = 0;

        name[0] = '\0';
        unw_get_proc_name(&cursor, name, sizeof(name), &offp);
        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        unw_get_reg(&cursor, UNW_REG_SP, &sp);

        if (strcmp(name, "__libc_start_main") == 0 || strcmp(name, "_start") == 0) { continue; }

        char *realname = abi::__cxa_demangle(name, 0, 0, &status);
        getFileAndLine((long)ip, file, &line);
        printf("%s in %s:%d\n", !status ? realname : name, file, line);
        free(realname);
    }
}

} // namespace Gringo

#endif // _GRINGO_BACKTRACE_HH
