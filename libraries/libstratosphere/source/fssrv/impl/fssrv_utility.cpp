/*
 * Copyright (c) Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stratosphere.hpp>
#include "fssrv_utility.hpp"

#if defined(ATMOSPHERE_OS_LINUX)
#include <unistd.h>
#elif defined(ATMOSPHERE_OS_MACOS)
#include  <mach-o/dyld.h>
#endif

namespace ams::fssystem {

    class PathOnExecutionDirectory {
        private:
            char m_path[fs::EntryNameLengthMax + 1];
        public:
            PathOnExecutionDirectory() {
                #if defined(ATMOSPHERE_OS_WINDOWS)
                {
                    /* Get the module file name. */
                    wchar_t module_file_name[fs::EntryNameLengthMax + 1];
                    if (::GetModuleFileNameW(0, module_file_name, util::size(module_file_name)) == 0) {
                        AMS_FS_R_ABORT_UNLESS(fs::ResultUnexpectedInPathOnExecutionDirectoryA());
                    }

                    /* Split the path. */
                    wchar_t drive_name[3];
                    wchar_t dir_name[fs::EntryNameLengthMax + 1];
                    ::_wsplitpath_s(module_file_name, drive_name, util::size(drive_name), dir_name, util::size(dir_name), nullptr, 0, nullptr, 0);

                    /* Print the drive and directory. */
                    wchar_t path[fs::EntryNameLengthMax + 1];
                    ::swprintf_s(path, util::size(path), L"%s%s", drive_name, dir_name);

                    /* Convert to utf-8. */
                   const auto res = ::WideCharToMultiByte(CP_UTF8, 0, path, -1, m_path, util::size(m_path), nullptr, nullptr);
                   if (res == 0) {
                       AMS_FS_R_ABORT_UNLESS(fs::ResultUnexpectedInPathOnExecutionDirectoryB());
                   }
                }
                #elif defined(ATMOSPHERE_OS_LINUX)
                {
                    char full_path[PATH_MAX] = {};
                    if (::readlink("/proc/self/exe", full_path, sizeof(full_path)) == -1) {
                        AMS_FS_R_ABORT_UNLESS(fs::ResultUnexpectedInPathOnExecutionDirectoryA());
                    }

                    const int len = std::strlen(full_path);
                    std::memcpy(m_path, full_path, len + 1);

                    for (int i = len - 1; i >= 0; --i) {
                        if (m_path[i] == '/') {
                            m_path[i + 1] = 0;
                            break;
                        }
                    }
                }
                #elif defined(ATMOSPHERE_OS_MACOS)
                {
                    char full_path[PATH_MAX + 1] = {};
                    uint32_t size = sizeof(full_path);
                    AMS_ABORT_UNLESS(_NSGetExecutablePath(full_path, std::addressof(size)) == 0);

                    const int len = std::strlen(full_path);
                    std::memcpy(m_path, full_path, len + 1);

                    for (int i = len - 1; i >= 0; --i) {
                        if (m_path[i] == '/') {
                            m_path[i + 1] = 0;
                            break;
                        }
                    }
                }
                #else
                AMS_ABORT("TODO: Unknown OS for PathOnExecutionDirectory");
                #endif
            }

            const char *Get() const {
                return m_path;
            }
    };

}

namespace ams::fssrv::impl {

    const char *GetExecutionDirectoryPath() {
        AMS_FUNCTION_LOCAL_STATIC(fssystem::PathOnExecutionDirectory, s_path_on_execution_directory);
        return s_path_on_execution_directory.Get();
    }

}
