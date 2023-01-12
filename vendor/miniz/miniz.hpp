#pragma once

#include <string>
#include <iostream>
#include <vector>

struct zip_archive;

struct zip_info
{
    std::string filename;

    struct
    {
        int year = 1980;
        int month = 0;
        int day = 0;
        int hours = 0;
        int minutes = 0;
        int seconds = 0;
    } date_time;

    std::string comment;
    std::string extra;
    uint16_t create_system = 0;
    uint16_t create_version = 0;
    uint16_t extract_version = 0;
    uint16_t flag_bits = 0;
    std::size_t volume = 0;
    uint32_t internal_attr = 0;
    uint32_t external_attr = 0;
    std::size_t header_offset = 0;
    uint32_t crc = 0;
    std::size_t compress_size = 0;
    std::size_t file_size = 0;
};

class zip_file
{
public:
    zip_file();
    zip_file(const std::string &filename);
    zip_file(std::istream &stream);
    zip_file(const std::vector<unsigned char> &bytes);

    ~zip_file();

    void load(std::istream &stream);
    void load(const std::string &filename);
    void load(const std::vector<unsigned char> &bytes);

    void save(const std::string &filename);
    void save(std::ostream &stream);
    void save(std::vector<unsigned char> &bytes);

    void reset();

    bool has_file(const std::string &name);
    bool has_file(const zip_info &name);

    zip_info getinfo(const std::string &name);
    
    std::vector<zip_info> infolist();

    std::vector<std::string> namelist();

    void extract(const std::string &member, const std::string &path);
    void extract(const zip_info &member, const std::string &path);

    void extractall(const std::string &path);
    void extractall(const std::string &path, const std::vector<std::string> &members);
    void extractall(const std::string &path, const std::vector<zip_info> &members);
    
    void read(const zip_info& info, std::vector<uint8_t>& out);
    std::string read(const zip_info &info);
    std::string read(const std::string &name);
    
    void write(const std::string &filename);
    void write(const std::string &filename, const std::string &arcname);
    void write(const std::string& arcname, std::vector<uint8_t> const& bytes);
    void writestr(const std::string &arcname, const std::string &bytes);
    void writestr(const zip_info &info, const std::string &bytes);

    std::string get_filename() const;
    
    std::string comment;
    
private:
    void start_read();
    void start_write();

    void append_comment();
    void remove_comment();

    zip_info getinfo(int index);

    std::unique_ptr<zip_archive> archive_;
    std::vector<char> buffer_;
    std::string filename_;
};