#include "miniz.hpp"

#include "miniz.h"
#include "miniz.c"


#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#include <algorithm>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <limits>
#include <sstream>
#include <memory>


#ifdef _WIN32
constexpr auto directory_separator = '\\';
constexpr auto alt_directory_separator = '/';
#else
constexpr auto directory_separator = '/';
constexpr auto alt_directory_separator = '\\';
#endif


static std::string join_path(const std::vector<std::string> &parts)
{
    std::string joined;
    std::size_t i = 0;
    for(auto part : parts)
    {
        joined.append(part);
        
        if(i++ != parts.size() - 1)
        {
            joined.append(1, '/');
        }
    }
    return joined;
}
    
static std::vector<std::string> split_path(const std::string &path, char delim = directory_separator)
{
    std::vector<std::string> split;
    std::string::size_type previous_index = 0;
    auto separator_index = path.find(delim);
    
    while(separator_index != std::string::npos)
    {
        auto part = path.substr(previous_index, separator_index - previous_index);
        if(part != "..")
        {
            split.push_back(part);
        }
        else
        {
            split.pop_back();
        }
        previous_index = separator_index + 1;
        separator_index = path.find(delim, previous_index);
    }
    
    split.push_back(path.substr(previous_index));

    if(split.size() == 1 && delim == directory_separator)
    {
        auto alternative = split_path(path, alt_directory_separator);
        if(alternative.size() > 1)
        {
            return alternative;
        }
    }
    
    return split;
}

static tm safe_localtime(const time_t &t)
{
#ifdef _WIN32
    tm time;
    localtime_s(&time, &t);
    return time;
#else
    tm *time = localtime(&t);
    assert(time != nullptr);
    return *time;
#endif
}

static std::size_t write_callback(void *opaque, std::uint64_t file_ofs, const void *pBuf, std::size_t n)
{
    auto buffer = static_cast<std::vector<char> *>(opaque);
    
    if(file_ofs + n > buffer->size())
    {
        auto new_size = static_cast<std::vector<char>::size_type>(file_ofs + n);
        buffer->resize(new_size);
    }

    for(std::size_t i = 0; i < n; i++)
    {
        (*buffer)[static_cast<std::size_t>(file_ofs + i)] = (static_cast<const char *>(pBuf))[i];
    }

    return n;
}



extern "C" {
    mz_ulong mz_crc32(mz_ulong crc, const mz_uint8 *ptr, size_t buf_len);
};

struct zip_archive: public mz_zip_archive {};



zip_file::zip_file() : archive_(new zip_archive())
{
    reset();
}

zip_file::zip_file(const std::string &filename) : zip_file()
{
    load(filename);
}

zip_file::zip_file(std::istream &stream) : zip_file()
{
    load(stream);
}

zip_file::zip_file(const std::vector<unsigned char> &bytes) : zip_file()
{
    load(bytes);
}

zip_file::~zip_file()
{
    reset();
}

void zip_file::load(std::istream &stream)
{
    reset();
    buffer_.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    remove_comment();
    start_read();
}

void zip_file::load(const std::string &filename)
{
    filename_ = filename;
    std::ifstream stream(filename, std::ios::binary);
    load(stream);
}

void zip_file::load(const std::vector<unsigned char> &bytes)
{
    reset();
    buffer_.assign(bytes.begin(), bytes.end());
    remove_comment();
    start_read();
}

void zip_file::save(const std::string &filename)
{
    filename_ = filename;
    std::ofstream stream(filename, std::ios::binary);
    save(stream);
}

void zip_file::save(std::ostream &stream)
{
    if (archive_->m_zip_mode == MZ_ZIP_MODE_WRITING)
    {
        mz_zip_writer_finalize_archive(archive_.get());
    }

    if (archive_->m_zip_mode == MZ_ZIP_MODE_WRITING_HAS_BEEN_FINALIZED)
    {
        mz_zip_writer_end(archive_.get());
    }

    if (archive_->m_zip_mode == MZ_ZIP_MODE_INVALID)
    {
        start_read();
    }

    append_comment();
    stream.write(buffer_.data(), static_cast<long>(buffer_.size()));
}

void zip_file::save(std::vector<unsigned char> &bytes)
{
    if (archive_->m_zip_mode == MZ_ZIP_MODE_WRITING)
    {
        mz_zip_writer_finalize_archive(archive_.get());
    }

    if (archive_->m_zip_mode == MZ_ZIP_MODE_WRITING_HAS_BEEN_FINALIZED)
    {
        mz_zip_writer_end(archive_.get());
    }

    if (archive_->m_zip_mode == MZ_ZIP_MODE_INVALID)
    {
        start_read();
    }

    append_comment();
    bytes.assign(buffer_.begin(), buffer_.end());
}

void zip_file::reset()
{
    switch (archive_->m_zip_mode)
    {
    case MZ_ZIP_MODE_READING:
        mz_zip_reader_end(archive_.get());
        break;
    case MZ_ZIP_MODE_WRITING:
        mz_zip_writer_finalize_archive(archive_.get());
        mz_zip_writer_end(archive_.get());
        break;
    case MZ_ZIP_MODE_WRITING_HAS_BEEN_FINALIZED:
        mz_zip_writer_end(archive_.get());
        break;
    case MZ_ZIP_MODE_INVALID:
        break;
    }

    if (archive_->m_zip_mode != MZ_ZIP_MODE_INVALID)
    {
        throw std::runtime_error("");
    }

    buffer_.clear();
    comment.clear();

    start_write();
    mz_zip_writer_finalize_archive(archive_.get());
    mz_zip_writer_end(archive_.get());
}

bool zip_file::has_file(const std::string &name)
{
    if (archive_->m_zip_mode != MZ_ZIP_MODE_READING)
    {
        start_read();
    }

    int index = mz_zip_reader_locate_file(archive_.get(), name.c_str(), nullptr, 0);

    return index != -1;
}

bool zip_file::has_file(const zip_info &name)
{
    return has_file(name.filename);
}

zip_info zip_file::getinfo(const std::string &name)
{
    if (archive_->m_zip_mode != MZ_ZIP_MODE_READING)
    {
        start_read();
    }

    int index = mz_zip_reader_locate_file(archive_.get(), name.c_str(), nullptr, 0);

    if (index == -1)
    {
        throw std::runtime_error("not found");
    }

    return getinfo(index);
}

std::vector<zip_info> zip_file::infolist()
{
    if (archive_->m_zip_mode != MZ_ZIP_MODE_READING)
        start_read();
    
    std::vector<zip_info> info;

    for (std::size_t i = 0; i < mz_zip_reader_get_num_files(archive_.get()); i++)
        info.push_back(getinfo(static_cast<int>(i)));
    
    return info;
}

std::vector<std::string> zip_file::namelist()
{
    std::vector<std::string> names;

    for (auto &info : infolist())
        names.push_back(info.filename);

    return names;
}


void zip_file::extract(const std::string &member, const std::string &path)
{
    extract(getinfo(member), path);
}

void zip_file::extract(const zip_info &member, const std::string &path)
{
    std::fstream stream(path + directory_separator + member.filename, std::ios::binary | std::ios::out);
    std::vector<uint8_t> data;
    read(member, data);
    stream.write((const char*)data.data(), data.size());
}

void zip_file::extractall(const std::string &path)
{
    extractall(path, infolist());
}

void zip_file::extractall(const std::string &path, const std::vector<std::string> &members)
{
    for (auto &member : members)
        extract(member, path);
}

void zip_file::extractall(const std::string &path, const std::vector<zip_info> &members)
{
    for (auto &member : members)
        extract(member, path);
}

void zip_file::read(const zip_info &info, std::vector<uint8_t> &out)
{
    std::size_t size;
    char *data = static_cast<char *>(mz_zip_reader_extract_file_to_heap(archive_.get(), info.filename.c_str(), &size, 0));
    if (data == nullptr)
        throw std::runtime_error("file couldn't be read");
    
    out.resize(size);
    std::memcpy(out.data(), data, size);

    mz_free(data);
}

std::string zip_file::read(const zip_info &info)
{
    std::size_t size;
    char *data = static_cast<char *>(mz_zip_reader_extract_file_to_heap(archive_.get(), info.filename.c_str(), &size, 0));
    if (data == nullptr)
    {
        throw std::runtime_error("file couldn't be read");
    }
    std::string extracted(data, data + size);
    mz_free(data);
    return extracted;
}

std::string zip_file::read(const std::string &name)
{
    return read(getinfo(name));
}

void zip_file::write(const std::string &filename)
{
    auto split = split_path(filename);
    if (split.size() > 1)
    {
        split.erase(split.begin());
    }
    auto arcname = join_path(split);
    write(filename, arcname);
}

void zip_file::write(const std::string &filename, const std::string &arcname)
{
    std::fstream file(filename, std::ios::binary | std::ios::in);
    std::stringstream ss;
    ss << file.rdbuf();
    std::string bytes = ss.str();

    writestr(arcname, bytes);
}

void zip_file::write(const std::string &arcname, std::vector<uint8_t> const &bytes)
{
    if (archive_->m_zip_mode != MZ_ZIP_MODE_WRITING)
    {
        start_write();
    }

    if (!mz_zip_writer_add_mem(archive_.get(), arcname.c_str(), bytes.data(), bytes.size(), MZ_BEST_COMPRESSION))
    {
        throw std::runtime_error("write error");
    }
}

void zip_file::writestr(const std::string &arcname, const std::string &bytes)
{
    if (archive_->m_zip_mode != MZ_ZIP_MODE_WRITING)
    {
        start_write();
    }

    if (!mz_zip_writer_add_mem(archive_.get(), arcname.c_str(), bytes.data(), bytes.size(), MZ_BEST_COMPRESSION))
    {
        throw std::runtime_error("write error");
    }
}

void zip_file::writestr(const zip_info &info, const std::string &bytes)
{
    if (info.filename.empty() || info.date_time.year < 1980)
    {
        throw std::runtime_error("must specify a filename and valid date (year >= 1980");
    }

    if (archive_->m_zip_mode != MZ_ZIP_MODE_WRITING)
    {
        start_write();
    }

    mz_ulong crc = mz_crc32(0, (const mz_uint8 *)bytes.c_str(), bytes.size());

    if (!mz_zip_writer_add_mem_ex(archive_.get(), info.filename.c_str(), bytes.data(), bytes.size(), info.comment.c_str(), static_cast<mz_uint16>(info.comment.size()), MZ_BEST_COMPRESSION, 0, crc))
    {
        throw std::runtime_error("write error");
    }
}

std::string zip_file::get_filename() const { return filename_; }

void zip_file::start_read()
{
    if (archive_->m_zip_mode == MZ_ZIP_MODE_READING)
        return;

    if (archive_->m_zip_mode == MZ_ZIP_MODE_WRITING)
    {
        mz_zip_writer_finalize_archive(archive_.get());
    }

    if (archive_->m_zip_mode == MZ_ZIP_MODE_WRITING_HAS_BEEN_FINALIZED)
    {
        mz_zip_writer_end(archive_.get());
    }

    if (!mz_zip_reader_init_mem(archive_.get(), buffer_.data(), buffer_.size(), 0))
    {
        throw std::runtime_error("bad zip");
    }
}

void zip_file::start_write()
{
    if (archive_->m_zip_mode == MZ_ZIP_MODE_WRITING)
        return;

    switch (archive_->m_zip_mode)
    {
    case MZ_ZIP_MODE_READING:
    {
        mz_zip_archive archive_copy;
        std::memset(&archive_copy, 0, sizeof(mz_zip_archive));
        std::vector<char> buffer_copy(buffer_.begin(), buffer_.end());

        if (!mz_zip_reader_init_mem(&archive_copy, buffer_copy.data(), buffer_copy.size(), 0))
        {
            throw std::runtime_error("bad zip");
        }

        mz_zip_reader_end(archive_.get());

        archive_->m_pWrite = &write_callback;
        archive_->m_pIO_opaque = &buffer_;
        buffer_ = std::vector<char>();

        if (!mz_zip_writer_init(archive_.get(), 0))
        {
            throw std::runtime_error("bad zip");
        }

        for (unsigned int i = 0; i < static_cast<unsigned int>(archive_copy.m_total_files); i++)
        {
            if (!mz_zip_writer_add_from_zip_reader(archive_.get(), &archive_copy, i))
            {
                throw std::runtime_error("fail");
            }
        }

        mz_zip_reader_end(&archive_copy);
        return;
    }
    case MZ_ZIP_MODE_WRITING_HAS_BEEN_FINALIZED:
        mz_zip_writer_end(archive_.get());
        break;
    case MZ_ZIP_MODE_INVALID:
    case MZ_ZIP_MODE_WRITING:
        break;
    }

    archive_->m_pWrite = &write_callback;
    archive_->m_pIO_opaque = &buffer_;

    if (!mz_zip_writer_init(archive_.get(), 0))
    {
        throw std::runtime_error("bad zip");
    }
}

void zip_file::append_comment()
{
    if (!comment.empty())
    {
        auto comment_length = std::min(static_cast<uint16_t>(comment.length()), std::numeric_limits<uint16_t>::max());
        buffer_[buffer_.size() - 2] = static_cast<char>(comment_length);
        buffer_[buffer_.size() - 1] = static_cast<char>(comment_length >> 8);
        std::copy(comment.begin(), comment.end(), std::back_inserter(buffer_));
    }
}

void zip_file::remove_comment()
{
    if (buffer_.empty())
        return;

    std::size_t position = buffer_.size() - 1;

    for (; position >= 3; position--)
    {
        if (buffer_[position - 3] == 'P' && buffer_[position - 2] == 'K' && buffer_[position - 1] == '\x05' && buffer_[position] == '\x06')
        {
            position = position + 17;
            break;
        }
    }

    if (position == 3)
    {
        throw std::runtime_error("didn't find end of central directory signature");
    }

    uint16_t length = static_cast<uint16_t>(buffer_[position + 1]);
    length = static_cast<uint16_t>(length << 8) + static_cast<uint16_t>(buffer_[position]);
    position += 2;

    if (length != 0)
    {
        comment = std::string(buffer_.data() + position, buffer_.data() + position + length);
        buffer_.resize(buffer_.size() - length);
        buffer_[buffer_.size() - 1] = 0;
        buffer_[buffer_.size() - 2] = 0;
    }
}

zip_info zip_file::getinfo(int index)
{
    if (archive_->m_zip_mode != MZ_ZIP_MODE_READING)
    {
        start_read();
    }

    mz_zip_archive_file_stat stat;
    mz_zip_reader_file_stat(archive_.get(), static_cast<mz_uint>(index), &stat);

    zip_info result;

    result.filename = std::string(stat.m_filename, stat.m_filename + std::strlen(stat.m_filename));
    result.comment = std::string(stat.m_comment, stat.m_comment + stat.m_comment_size);
    result.compress_size = static_cast<std::size_t>(stat.m_comp_size);
    result.file_size = static_cast<std::size_t>(stat.m_uncomp_size);
    result.header_offset = static_cast<std::size_t>(stat.m_local_header_ofs);
    result.crc = stat.m_crc32;
    auto time = safe_localtime(stat.m_time);
    result.date_time.year = 1900 + time.tm_year;
    result.date_time.month = 1 + time.tm_mon;
    result.date_time.day = time.tm_mday;
    result.date_time.hours = time.tm_hour;
    result.date_time.minutes = time.tm_min;
    result.date_time.seconds = time.tm_sec;
    result.flag_bits = stat.m_bit_flag;
    result.internal_attr = stat.m_internal_attr;
    result.external_attr = stat.m_external_attr;
    result.extract_version = stat.m_version_needed;
    result.create_version = stat.m_version_made_by;
    result.volume = stat.m_file_index;
    result.create_system = stat.m_method;

    return result;
}
