#pragma once

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

#ifdef _WIN32
#include <windows.h>
#else
#include <cstring>
#include <unistd.h>
#endif

namespace Clingo::Test {

class TempFile {
  public:
    explicit TempFile(std::string_view content) : filename_{create_()} {
        auto ofs = std::ofstream{filename_};
        if (!ofs) {
            throw std::runtime_error("failed to open temp file for writing");
        }
        ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
        if (!ofs) {
            throw std::runtime_error("failed to write content to temp file");
        }
        ofs.close();
    }

    TempFile(TempFile const &other) = delete;
    auto operator=(TempFile const &other) -> TempFile & = delete;

    TempFile(TempFile &&other) noexcept : filename_(std::move(other.filename_)) { other.filename_.clear(); }
    auto operator=(TempFile &&other) noexcept -> TempFile & {
        if (this != &other) {
            if (!filename_.empty()) {
                std::error_code ec;
                std::filesystem::remove(filename_, ec);
            }
            filename_ = std::move(other.filename_);
            other.filename_.clear();
        }
        return *this;
    }

    ~TempFile() {
        if (!filename_.empty()) {
            auto ec = std::error_code{};
            std::filesystem::remove(filename_, ec);
        }
    }

    [[nodiscard]] auto path() const -> std::filesystem::path const & { return filename_; }

  private:
    std::filesystem::path filename_;

#ifdef _WIN32
    static auto create_() -> std::filesystem::path {
        auto temp_dir = std::filesystem::temp_directory_path();
        auto temp_dir_w = temp_dir.wstring();
        wchar_t temp_file_name[MAX_PATH] = {0};
        if (GetTempFileNameW(temp_dir_w.c_str(), L"tmp", 0, temp_file_name) == 0) {
            throw std::runtime_error("GetTempFileName failed");
        }
        return std::filesystem::path{temp_file_name};
    }
#else
    static auto create_() -> std::filesystem::path {
        auto temp_dir = std::filesystem::temp_directory_path();
        auto template_str = (temp_dir / "tempfile_XXXXXX").string();
        auto tmpl = std::string{template_str.begin(), template_str.end()};
        auto fd = mkstemp(tmpl.data());
        if (fd == -1) {
            throw std::runtime_error("mkstemp failed");
        }
        close(fd);
        return std::filesystem::path{tmpl};
    }
#endif
};

} // namespace Clingo::Test
