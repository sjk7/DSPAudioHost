#pragma once
// win32_app_settings.h

#include <string>
#include <fstream>
#include <type_traits>
#include <cassert>
#include <vector>

#include "LoadRes.h"

namespace win32_steve {
namespace filesys {
    static constexpr const char BACKSLASH = '\\';
    static constexpr const char DOT = '.';
    static std::string user_app_path() {
        static std::string ret;
        if (ret.empty()) {
            ret.resize(1024);
            // a better choice might have been: CSIDL_LOCAL_APPDATA
            SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, &ret[0]);
            ret.resize(ret.find_first_of('\0'));
        }
        return ret;
    }

    static std::string filename_from_path(const std::string& path, int incl_extension) {
        size_t backslashpos = path.find_last_of(BACKSLASH);
        std::string ret;
        if (backslashpos != std::string::npos && path.length() > backslashpos + 1) {
            ret = path.substr(backslashpos + 1);
        }
        if (!incl_extension) {
            ret = ret.substr(0, ret.find_last_of(DOT));
        }
        return ret;
    }

    // gets an application folder path in a writeable location,
    // creating the folder if it needs to along the way.
    static std::string app_folder_path(const std::string& appid) {
        static std::string ret;
        std::string up = user_app_path();
        up += BACKSLASH + appid + BACKSLASH;

        if (ret.empty()) {

            if (PathFileExistsA(up.c_str()) != FALSE) {
                // folder exists
            } else {
                if (SHCreateDirectoryExA(NULL, up.c_str(), NULL) != 0) {
                    assert(0);
                }
            }
            // assume success here:
            ret = up; //-V820
        }
        return ret;
    }

    static std::string settings_path(
        const std::string& appname, const std::string& settingsid) {
        std::string folder = app_folder_path(appname);
        std::string path = folder + settingsid;
        return path;
    }

    static void replace_all(
        std::string& str, const std::string& find, const std::string& replace) {
        if (find.empty()) return;
        std::string wsRet;
        wsRet.reserve(str.length());
        size_t start_pos = 0, pos;
        while ((pos = str.find(find, start_pos)) != std::string::npos) {
            wsRet += str.substr(start_pos, pos - start_pos);
            wsRet += replace;
            pos += find.length();
            start_pos = pos;
        }
        wsRet += str.substr(start_pos);
        str.swap(wsRet); // faster than str = wsRet;
    }

    static std::string replace_illegals(
        const std::string& app_path, std::string replace_with = "_") { //-V813
        static std::vector<std::string> illegals{
            ".", ",", "\\", "/", "?", ";", "<", ">", ":", "\"", "|", "*"};
        std::string ret = app_path;

        for (auto& s : illegals) {
            replace_all(ret, s, replace_with);
        }

        return ret;
    }

    static int file_exists(const std::string_view path) {
        DWORD dwAttrib = GetFileAttributesA(path.data());

        return (dwAttrib != INVALID_FILE_ATTRIBUTES
            && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
    }

    static int folder_exists(const std::string& path) {
        DWORD dwAttrib = GetFileAttributesA(path.c_str());

        return (
            dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
    }
} // namespace filesys

namespace binary {

    template <typename T>
    bool settings_save_fullpath(const T& settings, std::string& path) {
        std::fstream f(path, std::ios::binary | std::ios::out);
        assert(f);
        f.write((char*)&settings, sizeof(settings));
        assert(f);
        f.flush();
        assert(f);
        auto tell = f.tellp();
        size_t put = size_t(tell);
        assert(put == sizeof(settings));
        if (f) return true;

        return false;
    }
    template <typename T>
    static bool settings_save(
        const T& settings, const std::string& appname, const std::string& settingsid) {

        // static_assert (std::is_trivially_default_constructible<T>::value, "settings
        // must be trivially constructible");
        std::string path
            = filesys::settings_path(appname, settingsid) + std::string(".bin");

        return settings_save_fullpath(settings, path);
    }

    template <typename T>
    static bool settings_load(T& settings, const std::string& path) {
        if (filesys::file_exists(path)) {
            std::ifstream f(path, std::ios::binary | std::ios::in);
            assert(f);
            f.read((char*)&settings, sizeof(settings));

            return true;
        } else {
            return false;
        }
    }
    template <typename T>
    static bool settings_load(
        T& settings, const std::string& appname, const std::string& settingsid) {

        // static_assert (std::is_trivially_default_constructible<T>::value, "settings
        // must be trivially constructible"); settings path is something like:
        // "C:\\Users\\coolie\\AppData\\Roaming\\V__v3Final_CapCard_SoundRouter_exe\\Maximod\\test12345.bin"
        std::string path
            = filesys::settings_path(appname, settingsid) + std::string(".bin");
        // Final path is something like:
        // "C:\\Users\\coolie\\AppData\\Roaming\\V__v3Final_CapCard_SoundRouter_exe\\Maximod\\preset_name_last_loaded.bin"
        return settings_load(settings, path);
    }

    static bool save_string(const std::string& s, const std::string& path) {
        std::fstream f(path, std::ios::binary | std::ios::out);
        assert(f);
        f.write(s.data(), s.size());
        assert(f);
        return size_t(f.tellp()) == s.size();
    }

    static bool read_string(std::string& s, const std::string& path) {
        if (!filesys::file_exists(path)) return false;
        std::fstream f(path, std::ios::binary | std::ios::in);
        assert(f);
        s.resize(101);
        f.read(&s[0], 100); // 100 so we always 0-terminated
        auto fnd = s.find('\0');
        if (fnd == 0) return false;
        size_t read = size_t(f.gcount());

        s.resize(read);
        return read == fnd;
    }
} // namespace binary

namespace settings {
    void create_sample_presets();
    // quick n dirty class to save binary app data
    struct app_settings {

        std::string m_dll_path;
        typedef std::vector<std::string> strvec;

        app_settings(const std::string& appid, HMODULE hmod)
            : m_dllhmod(hmod), m_dirty(0) {
            assert(appid.length()); // you must give you app a name, so I can be sure I
                                    // don't get confused if more than one thing inherits
                                    // from me in your app.
            app_name_path = discover_appname_path();
            app_name_path += appid;
            m_dll_path = this->dll_path(hmod);
            // assert that we can write some stuff in this place:
            struct test_t {
                test_t() { strcpy_s(buftest, "test12345"); }
                char buftest[10];
            };
            test_t test;
            assert(settings::app_settings::save_by_only_name(test, "test12345"));
            memset(&test, 0, sizeof(test));

            assert(load_by_name_only(test, "test12345"));
            std::string bak(test.buftest); //-V808
            assert(bak == "test12345");
            std::string s = settings_folder(); //-V808
            assert(!s.empty());
            assert(settings_exist("test12345"));

            create_sample_presets();
        }
        HMODULE m_dllhmod = {0};

        std::string dll_location(bool slash_on_end = true) const {
            std::string s(m_dll_path);
            auto f = s.find_last_of(filesys::BACKSLASH);
            assert(f != std::string::npos);

            if (slash_on_end) {
                return s.substr(0, f + 1);
            } else {
                return s.substr(0, f);
            }
        }
        bool settings_delete(const std::string& name) {
            if (settings_exist(name)) {
                std::string path = settings_folder() + name + ".bin";
                errno = 0;
                remove(path.c_str());
                return errno == 0;
            }
            return false;
        }

        bool settings_exist(const std::string& settings_name) const {
            return filesys::file_exists(settings_folder() + settings_name + ".bin") > 0;
        }
        template <typename T>
        bool save_by_only_name(const T& settings, const std::string& name) {
            assert(!name.empty());
            bool b = binary::settings_save<T>(settings, app_name_path, name);
            assert(b);
            return b;
        }

        template <typename T>
        bool load_by_name_only(T& settings, const std::string& name) {
            assert(!name.empty());
            memset(&settings, 0, sizeof(settings));
            bool b = binary::settings_load<T>(settings, app_name_path, name);
            assert(b);
            return b;
        }

        template <typename T>
        bool load_by_full_path(T& settings, const std::string& path) {
            assert(!path.empty());
            memset(&settings, 0, sizeof(settings));
            bool b = binary::settings_load<T>(settings, path);
            assert(b);
            return b;
        }
        // some path-friendly application id, that forms part of where the data is saved.
        // See also settings_folder()
        // NO slash on the end!
        std::string app_name_path;

        // this one has a slash on the end, and is the folder where all the data is saved.
        std::string settings_folder() const {
            std::string s = filesys::settings_path(app_name_path, "");
            return s;
        }
        // where filename really is just the filename (and extn if you want)
        bool save_string(const std::string& what, std::string& filename) {
            auto tmp = filename;
            filename = settings_folder() + tmp;
            return binary::save_string(what, filename);
        }

        // where filename really is just the filename (and extn if you want)
        bool read_string(std::string& what, std::string& filename) {
            auto tmp = filename;
            filename = settings_folder() + tmp;
            return binary::read_string(what, filename);
        }

        // get the current settings filenames.
        // Use filenames_only if you want to display just the filenames
        strvec enum_settings() {
            strvec ret;

            assert(!app_name_path.empty());
            const std::string folder = settings_folder();
            std::string fpath = folder;
            fpath += "*.bin";
            WIN32_FIND_DATAA fd;
            HANDLE h = ::FindFirstFileA(fpath.c_str(), &fd);
            BOOL b = FALSE;
            if (h == INVALID_HANDLE_VALUE) return ret;
            ret.push_back(folder + std::string(fd.cFileName));
            do {
                b = ::FindNextFileA(h, &fd);
                if (b) {
                    ret.push_back(folder + std::string(fd.cFileName));
                }
            } while (b);

            FindClose(h);
            return ret;
        }

        strvec filenames_only(const strvec& full_paths, int incl_extension = 0) {
            strvec ret;

            for (auto s : full_paths) {
                ret.emplace_back(filesys::filename_from_path(s, incl_extension));
            }

            return ret;
        }

        std::string dll_path(HMODULE hmod = NULL) {
            std::string s(1024, '\0');
            ::GetModuleFileNameA(hmod, &s[0], 1024);
            return s;
        }

        // returns a file-system-friendly version of the app's path
        // with a trailing slash
        std::string discover_appname_path() {
            auto s = dll_path();
            s.resize(s.find_first_of('\0'));
            s = filesys::replace_illegals(s);
            if (s.find_last_of("\\") != s.length() - 1) {
                s += "\\";
            }
            return s;
        }

        void dirty_set(int is_dirty) {
            if (m_dirty != is_dirty) {
                on_dirty_changed(is_dirty);
            }
            m_dirty = is_dirty;
        }
        int dirty() const { return m_dirty; }

        protected:
        virtual void on_dirty_changed(const int /* newdirty */) {}

        private:
        int m_dirty;

        inline void create_sample_presets() {
            // const auto s = this->app
            // app_name_path, name
            const auto f = this->settings_folder();
            const auto e = filesys::folder_exists(f);
            assert(e);
            (void)e;
            const std::vector<size_t> v{IDR_RCDATA1, IDR_RCDATA2, IDR_RCDATA3, //-V826
                IDR_RCDATA4, IDR_RCDATA5, IDR_RCDATA6, IDR_RCDATA7, IDR_RCDATA8,
                IDR_RCDATA9, IDR_RCDATA10, IDR_RCDATA11, IDR_RCDATA12, IDR_RCDATA13};
            size_t extracted = 0;

            while (extracted < v.size()) {
                auto idr_preset = v[extracted];
                DWORD size;
                const BYTE* data
                    = my::GetResourcePreset(idr_preset, this->m_dllhmod, size); //-V107
                if (!data) {
                    extracted++;
                    continue;
                }
                auto snum = std::to_string(extracted + 1);
                if (snum.size() == 1) {
                    snum = "0" + snum;
                }
                std::string path = f + "example-preset" + snum + std::string(".bin");

                if (!filesys::file_exists(path)) {
                    std::ofstream file(path, std::ios::out | std::ios::binary);
                    assert(file);
                    file.write((char*)data, size);
                    assert(file);
                    file.close();
                }
                extracted++;
            }
        }
    };

} // namespace settings
} // namespace win32_steve
