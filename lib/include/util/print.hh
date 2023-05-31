#pragma once

#include <memory>
#include <ostream>

template <class T> struct p_elem {
    p_elem(T const &elem) : elem(elem) {}
    friend auto operator<<(std::ostream &out, p_elem const &elem) -> std::ostream & {
        out << elem.elem;
        return out;
    }

    T const &elem;
};

template <class T> struct p_elem<std::unique_ptr<T>> {
    p_elem(std::unique_ptr<T> const &elem) : elem(elem) {}
    friend auto operator<<(std::ostream &out, p_elem const &elem) -> std::ostream & {
        out << *elem.elem;
        return out;
    }

    std::unique_ptr<T> const &elem;
};

template <class T> struct p_elem<std::shared_ptr<T>> {
    p_elem(std::shared_ptr<T> const &elem) : elem(elem) {}
    friend auto operator<<(std::ostream &out, p_elem const &elem) -> std::ostream & {
        out << *elem.elem;
        return out;
    }

    std::shared_ptr<T> const &elem;
};

template <class T> struct p_range {
    p_range(T const &rng, char const *sep = ",") : rng(rng), sep(sep) {}
    friend auto operator<<(std::ostream &out, p_range const &rng) -> std::ostream & {
        bool comma = false;
        for (auto &elem : rng.rng) {
            if (comma) {
                out << rng.sep;
            }
            comma = true;
            out << p_elem{elem};
        }
        return out;
    }
    T const &rng;
    char const *sep;
};

template <class T, class F> struct p_range_with {
    p_range_with(T const &rng, char const *sep, F &&f) : rng(rng), f(std::forward<F>(f)), sep(sep) {}
    p_range_with(T const &rng, F &&f) : p_range_with(rng, ",", std::forward<F>(f)) {}
    friend auto operator<<(std::ostream &out, p_range_with const &rng) -> std::ostream & {
        bool comma = false;
        for (auto &elem : rng.rng) {
            if (comma) {
                out << rng.sep;
            }
            comma = true;
            rng.f(out, elem);
        }
        return out;
    }
    T const &rng;
    F f;
    char const *sep;
};

inline void print_quoted(std::ostream &out, std::string const &str) {
    // TODO: in principle there are the codepoints too...
    out << '"';
    for (auto c : str) {
        if (c == '\\') {
            out << "\\\\";
        } else if (c == '\n') {
            out << "\\n";
        } else if (c == '\t') {
            out << "\\t";
        } else {
            out << c;
        }
    }
    out << '"';
}
