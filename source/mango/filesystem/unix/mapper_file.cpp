/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2017 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/exception.hpp>
#include <mango/core/string.hpp>
#include <mango/filesystem/mapper.hpp>
#include <mango/filesystem/path.hpp>

#define ID "[mapper.file] "

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

namespace
{
    using namespace mango;
    using namespace mango::filesystem;

    // -----------------------------------------------------------------
	// get_pagesize()
    // -----------------------------------------------------------------

	inline long get_pagesize()
	{
		// NOTE: could be _SC_PAGE_SIZE for some platforms according to Linux Programmer's Manual
		static long x = ::sysconf(_SC_PAGESIZE);
		return x;
	}

    // -----------------------------------------------------------------
    // FileMemory
    // -----------------------------------------------------------------

    class FileMemory : public VirtualMemory
    {
    protected:
        int m_file;
		size_t m_size;
		void* m_address;

    public:
        FileMemory(const std::string& filename, u64 x_offset, u64 x_size)
            : m_file(-1)
            , m_size(0)
            , m_address(nullptr)
        {
            m_file = open(filename.c_str(), O_RDONLY);

            if (m_file != -1)
            {
                struct stat sb;

                if (::fstat(m_file, &sb) == -1)
                {
                    ::close(m_file);
                    m_file = -1;
	                MANGO_EXCEPTION(ID"File \"%s\" cannot be read.", filename.c_str());
                }
                else
                {
					const size_t file_size = size_t(sb.st_size);
					const size_t file_offset = size_t(x_offset);

					size_t page_offset = 0;
					if (file_offset > 0)
					{
						const long page_size = get_pagesize();
						const long page_number = file_offset / page_size;
						page_offset = page_number * page_size;
					}

					m_size = file_size - file_offset;
					if (x_size > 0)
					{
						m_size = std::min(size_t(x_size), m_size);
					}

                    if (m_size > 0)
                    {
                        m_address = ::mmap(nullptr, m_size, PROT_READ, MAP_FILE | MAP_SHARED, m_file, page_offset);

                        if (m_address == MAP_FAILED)
                        {
                            MANGO_EXCEPTION(ID"Memory mapping \"%s\" failed.", filename.c_str());
                        }

                        m_memory.size = m_size;
                        m_memory.address = reinterpret_cast<u8*>(m_address) + (file_offset - page_offset);
                    }
                    else
                    {
                        m_memory.size = 0;
                        m_memory.address = nullptr;
                    }
                }
            }
            else
            {
                MANGO_EXCEPTION(ID"Opening \"%s\" failed.", filename.c_str());
            }
        }

        ~FileMemory()
        {
            if (m_address)
            {
                ::munmap(m_address, m_size);
            }

            if (m_file != -1)
            {
                ::close(m_file);
            }
        }
    };

    // -----------------------------------------------------------------
    // FileMapper
    // -----------------------------------------------------------------

    class FileMapper : public AbstractMapper
    {
    protected:
        std::string m_basepath;

        void emplace_helper(FileIndex& index, const std::string& pathname, std::string filename)
        {
            const std::string testname = pathname + filename;

            struct stat s;
            if (::stat(testname.c_str(), &s) != -1)
            {
                filename = removePrefix(filename, m_basepath);

                if ((s.st_mode & S_IFDIR) == 0)
                {
                    size_t size = size_t(s.st_size);
                    index.emplace(filename, size, 0);
                }
                else
                {
                    index.emplace(filename + "/", 0, FileInfo::DIRECTORY);
                }
            }
        }

    public:
        FileMapper(const std::string& basepath)
            : m_basepath(basepath)
        {
        }

        ~FileMapper()
        {
        }

        bool isFile(const std::string& filename) const override
        {
            bool is = false;
            std::string testname = m_basepath + filename;

            struct stat s;
            if (::stat(testname.c_str(), &s) == 0)
            {
                is = (s.st_mode & S_IFDIR) == 0;
            }

            return is;
        }

#if defined(MANGO_PLATFORM_OSX) || defined(MANGO_PLATFORM_IOS) || defined(MANGO_PLATFORM_BSD)

        void getIndex(FileIndex& index, const std::string& pathname) override
        {
            struct dirent** namelist = NULL;
            std::string fullname = m_basepath + pathname;

            const int n = ::scandir(fullname.c_str(), &namelist, [] (const struct dirent* e)
            {
                // filter out "." and ".."
                if (!std::strcmp(e->d_name, ".") || !std::strcmp(e->d_name, ".."))
                    return 0;
                return 1;
            }, 0);
            if (n < 0)
            {
                // Unable to open directory.
                ::free(namelist);
                return;
            }

            for (int i = 0; i < n; ++i)
            {
                const std::string filename(namelist[i]->d_name);
                ::free(namelist[i]);
                emplace_helper(index, fullname, filename);
            }

            ::free(namelist);
        }

#else

        void getIndex(FileIndex& index, const std::string& pathname) override
        {
            std::string fullname = m_basepath + pathname;
            DIR* dirp = ::opendir(fullname.c_str());
            if (!dirp)
            {
                // Unable to open directory.
                return;
            }

            dirent* dp;

            while ((dp = ::readdir(dirp)))
            {
                std::string filename(dp->d_name);

                // skip "." and ".."
                if (filename != "." && filename != "..")
                {
                    emplace_helper(index, fullname, filename);
                }
            }

            ::closedir(dirp);
        }

#endif

        VirtualMemory* mmap(const std::string& filename) override
        {
            VirtualMemory* memory = new FileMemory(m_basepath + filename, 0, 0);
            return memory;
        }
    };

} // namespace

namespace mango {
namespace filesystem {

    // -----------------------------------------------------------------
    // Mapper::createFileMapper()
    // -----------------------------------------------------------------

    AbstractMapper* Mapper::createFileMapper(const std::string& basepath)
    {
        AbstractMapper* mapper = new FileMapper(basepath);
        m_mappers.emplace_back(mapper);
        return mapper;
    }

} // namespace filesystem
} // namespace mango
