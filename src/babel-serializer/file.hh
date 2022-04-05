#pragma once

#include <cstdio>

#include <clean-core/alloc_array.hh>
#include <clean-core/array.hh>
#include <clean-core/macros.hh>
#include <clean-core/native/win32_fwd.hh>
#include <clean-core/range_ref.hh>
#include <clean-core/stream_ref.hh>
#include <clean-core/string_view.hh>

#include <babel-serializer/errors.hh>

namespace babel::file
{
/// returns true if the file exists and can be read
bool exists(cc::string_view filename);

/// reads a file and writes all bytes in the provided stream (buffered)
void read(cc::stream_ref<std::byte> out, cc::string_view filename, error_handler on_error = default_error_handler);

/// reads a file and returns the content as a string (null terminated)
cc::string read_all_text(cc::string_view filename, error_handler on_error = default_error_handler);
cc::alloc_array<char> read_all_text(cc::string_view filename, cc::allocator* alloc, error_handler on_error = default_error_handler);

/// reads a file and returns the content as an array of bytes
cc::array<std::byte> read_all_bytes(cc::string_view filename, error_handler on_error = default_error_handler);
/// reads a file and returns the content as an allocator-backed array of chars
cc::alloc_array<std::byte> read_all_bytes(cc::string_view filename, cc::allocator* alloc, error_handler on_error = default_error_handler);

/// writes the given binary data to a file
void write(cc::string_view filename, cc::span<std::byte const> data, error_handler on_error = default_error_handler);

/// writes the given string data to a file
void write(cc::string_view filename, cc::string_view data, error_handler on_error = default_error_handler);

/// writes the given lines to a file
void write_lines(cc::string_view filename, cc::range_ref<cc::string_view> lines, cc::string_view line_ending = "\n", error_handler on_error = default_error_handler);

/// an output file stream that can be used as cc::stream_ref<char>, cc::stream_ref<std::byte>, cc::string_stream_ref
/// NOTE: overwrites existing files
struct file_output_stream
{
    explicit file_output_stream(cc::string_view filename);
    file_output_stream() = default;
    file_output_stream(file_output_stream const&) = delete;
    file_output_stream& operator=(file_output_stream const&) = delete;
    file_output_stream(file_output_stream&& rhs) noexcept
    {
        _file = rhs._file;
        rhs._file = nullptr;
    }
    file_output_stream& operator=(file_output_stream&& rhs) noexcept
    {
        if (_file)
        {
            std::fclose(_file);
        }
        _file = rhs._file;
        rhs._file = nullptr;
        return *this;
    }

    ~file_output_stream()
    {
        if (_file)
        {
            std::fclose(_file);
            _file = nullptr;
        }
    }

    void operator()(cc::string_view content) { fwrite(content.data(), 1, content.size(), _file); }
    void operator()(cc::span<char const> content) { fwrite(content.data(), 1, content.size(), _file); }
    void operator()(cc::span<std::byte const> content) { fwrite(content.data(), 1, content.size(), _file); }

    bool valid() const { return _file != nullptr; }

private:
    FILE* _file = nullptr;
};

namespace detail
{
struct mmap_info
{
#if defined(CC_OS_WINDOWS)
    HANDLE file_handle = nullptr;
    HANDLE file_mapping_handle = nullptr;
#else
    int file_descriptor = -1;
#endif
    size_t byte_size = 0;
    void* data = nullptr;
};
mmap_info impl_map_file_to_memory(cc::string_view filepath, bool is_readonly);

#if defined(CC_OS_WINDOWS)
void impl_unmap(HANDLE file_handle, HANDLE file_mapping_handle, void* file_view);
#else
void impl_unmap(void* data, size_t size, int file_descriptor);
#endif
}

template <class T>
struct memory_mapped_file : public cc::span<T>
{
public:
    memory_mapped_file() = default;
    memory_mapped_file(memory_mapped_file const&) = delete;
    memory_mapped_file& operator=(memory_mapped_file const&) = delete;
    memory_mapped_file(memory_mapped_file&& rhs) noexcept
    {
#if defined(CC_OS_WINDOWS)
        detail::impl_unmap(_file_handle, _file_mapping_handle, const_cast<void*>(static_cast<void const*>(this->data())));
#else
        detail::impl_unmap(const_cast<void*>(static_cast<void const*>(this->data())), this->size(), _file_descriptor);
#endif
        *static_cast<cc::span<T>*>(this) = rhs;
        *static_cast<cc::span<T>*>(&rhs) = {};
#if defined(CC_OS_WINDOWS)
        _file_handle = rhs._file_handle;
        rhs._file_handle = nullptr;
        _file_mapping_handle = rhs._file_mapping_handle;
        rhs._file_mapping_handle = nullptr;
#else
        _file_descriptor = rhs._file_descriptor;
        rhs._file_descriptor = -1;
#endif
    }
    memory_mapped_file& operator=(memory_mapped_file&& rhs) noexcept
    {
        *static_cast<cc::span<T>*>(this) = rhs;
        *static_cast<cc::span<T>*>(&rhs) = {};
#if defined(CC_OS_WINDOWS)
        _file_handle = rhs._file_handle;
        rhs._file_handle = nullptr;
        _file_mapping_handle = rhs._file_mapping_handle;
        rhs._file_mapping_handle = nullptr;
#else
        _file_descriptor = rhs._file_descriptor;
        rhs._file_descriptor = -1;
#endif
        return *this;
    }

    explicit memory_mapped_file(cc::string_view filepath)
    {
        auto const info = detail::impl_map_file_to_memory(filepath, std::is_const_v<T>);
#if defined(CC_OS_WINDOWS)
        _file_handle = info.file_handle;
        _file_mapping_handle = info.file_mapping_handle;
#else
        _file_descriptor = info.file_descriptor;
#endif
        CC_ASSERT(info.byte_size % sizeof(T) == 0 && "Filesize does not match T");
        *static_cast<cc::span<T>*>(this) = cc::span<T>{static_cast<T*>(info.data), info.byte_size / sizeof(T)};
    }

    ~memory_mapped_file()
    {
        // the const cast is ok here because mmap() / MapViewOfFile() returned a void* in the first place
#if defined(CC_OS_WINDOWS)
        detail::impl_unmap(_file_handle, _file_mapping_handle, const_cast<void*>(static_cast<void const*>(this->data())));
#else
        detail::impl_unmap(const_cast<void*>(static_cast<void const*>(this->data())), this->size(), _file_descriptor);
#endif
    }

private:
#if defined(CC_OS_WINDOWS)
    HANDLE _file_handle = nullptr;
    HANDLE _file_mapping_handle = nullptr;
#else
    int _file_descriptor = -1;
#endif
};
}
