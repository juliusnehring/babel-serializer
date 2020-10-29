#include "ply.hh"

#include <clean-core/from_string.hh>
#include <clean-core/string_view.hh>
#include <clean-core/vector.hh>

// TODO:
// * Performance optimization: When binary little endian AND no lists, then we can just copy the entire binary part

namespace
{
constexpr size_t size_of(babel::ply::type t)
{
    switch (t)
    {
    case babel::ply::type::invalid:
        return 0;
    case babel::ply::type::char_t:
        return 1;
    case babel::ply::type::uchar_t:
        return 1;
    case babel::ply::type::short_t:
        return 2;
    case babel::ply::type::ushort_t:
        return 2;
    case babel::ply::type::int_t:
        return 4;
    case babel::ply::type::uint_t:
        return 4;
    case babel::ply::type::float_t:
        return 4;
    case babel::ply::type::double_t:
        return 8;
    }
    return {};
}

size_t compute_data_size_in_bytes(babel::ply::geometry const& g)
{
    size_t size = 0;
    for (auto const& e : g.elements)
    {
        size_t element_size = 0;
        for (auto i = 0; i < e.properties_count; ++i)
        {
            auto const& p = g.properties[e.properties_start + i];
            if (p.list_size_type != babel::ply::type::invalid)
                element_size += sizeof(babel::ply::geometry::list_property_entry);
            else
                element_size += size_of(p.type);
        }
        size += e.count * element_size;
    }
    return size;
}
}

babel::ply::geometry babel::ply::read(cc::span<const std::byte> data, const babel::ply::read_config& /*cfg*/, babel::error_handler on_error)
{
    auto const data_as_sv = cc::string_view(reinterpret_cast<char const*>(data.begin()), reinterpret_cast<char const*>(data.end()));

    size_t pos = 0;
    auto const next_line = [&]() -> cc::string_view {
        auto start = pos;
        while (pos < data_as_sv.size() && data_as_sv[pos] != '\n')
            ++pos;
        auto count = pos - start;
        if (pos < data_as_sv.size()) // skip '\n'
            ++pos;
        return data_as_sv.subview(start, count).trim();
    };
    auto const has_more_lines = [&]() { return pos < data_as_sv.size(); };

    enum class file_type
    {
        ascii,
        binary_big_endian,
        binary_little_endian
    } file_type;

    geometry geometry;

    auto const parse_type = [](cc::string_view s) -> type {
        if (s == "char")
            return type::char_t;
        if (s == "uchar")
            return type::uchar_t;
        if (s == "short")
            return type::short_t;
        if (s == "ushort")
            return type::ushort_t;
        if (s == "int")
            return type::int_t;
        if (s == "uint")
            return type::uint_t;
        if (s == "short")
            return type::short_t;
        if (s == "float")
            return type::float_t;
        if (s == "double")
            return type::double_t;
        return type::invalid;
    };

    // parse the header
    {
        auto const ply = next_line();
        if (ply != "ply")
        {
            on_error(data, cc::as_byte_span(ply), "Failed to parse ply header: Ply files must start with the letters 'ply'!", severity::error);
            return {};
        }
        auto const format_line = next_line();
        if (!format_line.starts_with("format"))
        {
            on_error(data, cc::as_byte_span(format_line), "Failed to parse ply header: Missing format line!", severity::error);
            return {};
        }
        auto const format = *format_line.subview(7).split().begin();
        if (format == "ascii")
            file_type = file_type::ascii;
        else if (format == "binary_little_endian")
            file_type = file_type::binary_little_endian;
        else if (format == "binary_big_endian")
            file_type = file_type::binary_big_endian;
        else
        {
            on_error(data, cc::as_byte_span(format), "Failed to parse ply header: Unkown format!", severity::error);
            return {};
        }
        if (!format_line.ends_with("1.0"))
        {
            on_error(data, cc::as_byte_span(format_line), "Failed to parse ply header: Only supports ply version 1.0", severity::error);
            return {};
        }

        { // parse elements and properties

            // keep track of the size to determine the start of each element in data
            auto total_data_size = 0;
            cc::string_view line = next_line();
            while (line != "end_header")
            {
                if (!has_more_lines())
                {
                    on_error(data, cc::as_byte_span(line), "Failed to parse ply header: Unexpected end of file", severity::error);
                    return {};
                }

                if (line.starts_with("comment"))
                    continue; // ignore comments

                if (line.starts_with("element"))
                {
                    cc::string_view name;
                    int count;
                    int idx = 0;
                    for (auto const token : line.subview(7).split())
                    {
                        switch (idx++)
                        {
                        case 0:
                            name = token;
                            break;
                        case 1:
                        {
                            if (!cc::from_string(token, count))
                            {
                                on_error(data, cc::as_byte_span(token), "Failed to parse ply header: Invalid element count", severity::error);
                                return {};
                            }
                        }

                        break;
                        default:;
                            on_error(data, cc::as_byte_span(token), "Failed to parse ply header: Unknown element data", severity::warning);
                        }
                    }
                    if (idx <= 1)
                    {
                        on_error(data, cc::as_byte_span(line), "Failed to parse ply header: Element name and/or size missing!", severity::error);
                        return {};
                    }
                    // add new element
                    auto& element = geometry.elements.emplace_back();
                    element.count = count;
                    element.name = name;
                    element.properties_start = int(geometry.properties.size());
                    element.properties_count = 0;
                }

                else if (line.starts_with("property"))
                {
                    if (geometry.elements.empty())
                    {
                        on_error(data, cc::as_byte_span(line), "Failed to parse ply header: Properties must be part of an element!", severity::error);
                        return {};
                    }

                    geometry.elements.back().properties_count++;
                    auto& property = geometry.properties.emplace_back();
                    property.data_start = total_data_size;
                    property.data_stride = 0;

                    bool is_list = false;
                    int idx = 0;
                    for (auto const token : line.subview(8).split())
                    {
                        switch (idx++)
                        {
                        case 0:
                            if (token == "list")
                            {
                                is_list = true;
                            }
                            else
                            {
                                property.type = parse_type(token);
                                if (property.type == type::invalid)
                                {
                                    on_error(data, cc::as_byte_span(token), "Failed to parse ply header: Invalid property type!", severity::error);
                                    return {};
                                }
                            }
                            break;
                        case 1:
                            if (is_list)
                            {
                                property.list_size_type = parse_type(token);
                                if (property.list_size_type == type::invalid)
                                {
                                    on_error(data, cc::as_byte_span(token), "Failed to parse ply header: Invalid list property index type!", severity::error);
                                    return {};
                                }
                                else if (property.list_size_type == type::float_t || property.list_size_type == type::double_t)
                                {
                                    on_error(data, cc::as_byte_span(token), "Failed to parse ply header: List size type must be an integer type!", severity::error);
                                    return {};
                                }
                            }
                            else
                            {
                                property.name = token;
                            }
                            break;
                        case 2:
                            if (is_list)
                            {
                                property.type = parse_type(token);
                                if (property.type == type::invalid)
                                {
                                    on_error(data, cc::as_byte_span(token), "Failed to parse ply header: Invalid property type!", severity::error);
                                    return {};
                                }
                            }
                            else
                            {
                                on_error(data, cc::as_byte_span(token), "Failed to parse ply header: Unknown property token!", severity::error);
                                return {};
                            }
                        case 3:
                            if (is_list)
                            {
                                property.name = token;
                            }
                            else
                            {
                                CC_UNREACHABLE("Properties have either 2 (type and name) or 4 (list, index type, data type, name) token.");
                            }
                        }
                    }
                    if ((is_list && idx <= 3) || (!is_list && idx <= 1))
                    {
                        on_error(data, cc::as_byte_span(line), "Failed to parse ply header: Invalid property!", severity::error);
                        return {};
                    }

                    // increase stride of all propertys that belong to the current element
                    auto const property_size = property.is_list() ? int(sizeof(geometry::list_property_entry)) : int(size_of(property.type));
                    auto const& element = geometry.elements.back();
                    for (auto i = 0; i < element.properties_count; ++i)
                        geometry.properties[element.properties_start + i].data_stride += property_size;
                }

                else
                {
                    on_error(data, cc::as_byte_span(line), "Failed to parse ply header: Unknown line!", severity::warning);
                }

                line = next_line();
            }
        }
    }

    { // parse the data
        auto const data_size = compute_data_size_in_bytes(geometry);
        geometry.data.resize(data_size);
        size_t data_idx = 0;
        if (file_type::ascii == file_type)
        {
            // ascii helper
            auto const parse_i8 = [&](cc::string_view s) -> cc::int8 {
                cc::int8 i = 0;
                if (!cc::from_string(s, i))
                {
                    on_error(data, cc::as_byte_span(s), "Failed to parse char!", severity::error);
                }
                return i;
            };
            auto const parse_u8 = [&](cc::string_view s) -> cc::uint8 {
                cc::uint8 u = 0;
                if (!cc::from_string(s, u))
                {
                    on_error(data, cc::as_byte_span(s), "Failed to parse uchar!", severity::error);
                }
                return u;
            };
            auto const parse_i16 = [&](cc::string_view s) -> cc::int16 {
                cc::int16 i = 0;
                if (!cc::from_string(s, i))
                {
                    on_error(data, cc::as_byte_span(s), "Failed to parse short!", severity::error);
                }
                return i;
            };
            auto const parse_u16 = [&](cc::string_view s) -> cc::uint16 {
                cc::uint16 u = 0;
                if (!cc::from_string(s, u))
                {
                    on_error(data, cc::as_byte_span(s), "Failed to parse ushort!", severity::error);
                }
                return u;
            };
            auto const parse_i32 = [&](cc::string_view s) -> cc::int32 {
                cc::int32 i = 0;
                if (!cc::from_string(s, i))
                {
                    on_error(data, cc::as_byte_span(s), "Failed to parse int!", severity::error);
                }
                return i;
            };
            auto const parse_u32 = [&](cc::string_view s) -> cc::uint32 {
                cc::uint32 u = 0;
                if (!cc::from_string(s, u))
                {
                    on_error(data, cc::as_byte_span(s), "Failed to parse uint!", severity::error);
                }
                return u;
            };
            auto const parse_f32 = [&](cc::string_view s) -> cc::float32 {
                cc::float32 f = 0;
                if (!cc::from_string(s, f))
                {
                    on_error(data, cc::as_byte_span(s), "Failed to parse float!", severity::error);
                }
                return f;
            };
            auto const parse_f64 = [&](cc::string_view s) -> cc::float64 {
                cc::float64 f = 0;
                if (!cc::from_string(s, f))
                {
                    on_error(data, cc::as_byte_span(s), "Failed to parse double!", severity::error);
                }
                return f;
            };

            auto const append_data = [&](auto const& v) {
                CC_ASSERT(data_idx + sizeof(v) < geometry.data.size());
                std::memcpy(&geometry.data[data_idx], &v, sizeof(v));
                data_idx += sizeof(v);
            };

            auto const append_data_from_string = [&](cc::string_view s, type t) {
                switch (t)
                {
                case type::char_t:
                {
                    auto const i = parse_i8(s);
                    append_data(i);
                    break;
                }
                case type::uchar_t:
                {
                    auto const i = parse_u8(s);
                    append_data(i);
                    break;
                }
                case type::short_t:
                {
                    auto const i = parse_i16(s);
                    append_data(i);
                    break;
                }
                case type::ushort_t:
                {
                    auto const i = parse_u16(s);
                    append_data(i);
                    break;
                }
                case type::int_t:
                {
                    auto const i = parse_i32(s);
                    append_data(i);
                    break;
                }
                case type::uint_t:
                {
                    auto const i = parse_u32(s);
                    append_data(i);
                    break;
                }
                case type::float_t:
                {
                    auto const f = parse_f32(s);
                    append_data(f);
                    break;
                }
                case type::double_t:
                {
                    auto const f = parse_f64(s);
                    append_data(f);
                    break;
                }
                default:
                    CC_UNREACHABLE("Invalid data type");
                }
            };


            for (auto const& e : geometry.elements)
            {
                for (auto i = 0; i < e.count; ++i)
                {
                    if (!has_more_lines())
                    {
                        on_error(data, data.last(0), "Failed to parse ply: Unexpected end of file!", severity::error);
                        return geometry;
                    }
                    auto const line = next_line();
                    auto it = line.split().begin();
                    auto const end = line.split().end();
                    auto const next_token = [&]() -> cc::string_view {
                        if (it != end)
                        {
                            auto const token = *it;
                            ++it;
                            return token;
                        }
                        else
                        {
                            on_error(data, cc::as_byte_span(line), "Failed to parse ply: Unexpected end of line!", severity::error);
                            return cc::string_view();
                        }
                    };

                    for (auto p_idx = 0; p_idx < e.properties_count; ++p_idx)
                    {
                        auto const& p = geometry.properties[e.properties_start + p_idx];
                        if (p.list_size_type != type::invalid)
                        {
                            auto const list_size_s = next_token();
                            cc::int64 list_size = 0; // large enough size
                            // this is probably unnecessary for ascii files as they should all be valid integer
                            switch (p.list_size_type)
                            {
                            case type::char_t:
                                list_size = parse_i8(list_size_s);
                                break;
                            case type::uchar_t:
                                list_size = parse_u8(list_size_s);
                                break;
                            case type::short_t:
                                list_size = parse_i16(list_size_s);
                                break;
                            case type::ushort_t:
                                list_size = parse_u16(list_size_s);
                                break;
                            case type::int_t:
                                list_size = parse_i32(list_size_s);
                                break;
                            case type::uint_t:
                                list_size = parse_u32(list_size_s);
                                break;
                            default:
                                CC_UNREACHABLE("List size type must be an integer type");
                            }
                            if (list_size < 0)
                            {
                                on_error(data, cc::as_byte_span(list_size_s), "Failed to parse ply: List size cannot be negative!", severity::error);
                                list_size = 0;
                            }
                            geometry::list_property_entry list_element;
                            list_element.start_idx = int(geometry.list_data.size());
                            for (auto l = 0; l < list_size; ++l)
                            {
                                auto const s = next_token();
                                switch (p.type)
                                {
                                case type::char_t:
                                {
                                    auto const v = parse_i8(s);
                                    geometry.list_data.push_back(std::byte(v));
                                    break;
                                }
                                case type::uchar_t:
                                {
                                    auto const v = parse_u8(s);
                                    geometry.list_data.push_back(std::byte(v));
                                    break;
                                }
                                case type::short_t:
                                {
                                    auto const v = parse_i16(s);
                                    geometry.list_data.push_back_range(cc::as_byte_span(v));
                                    break;
                                }
                                case type::ushort_t:
                                {
                                    auto const v = parse_u16(s);
                                    geometry.list_data.push_back_range(cc::as_byte_span(v));
                                    break;
                                }
                                case type::int_t:
                                {
                                    auto const v = parse_i32(s);
                                    geometry.list_data.push_back_range(cc::as_byte_span(v));
                                    break;
                                }
                                case type::uint_t:
                                {
                                    auto const v = parse_u32(s);
                                    geometry.list_data.push_back_range(cc::as_byte_span(v));
                                    break;
                                }
                                case type::float_t:
                                {
                                    auto const v = parse_f32(s);
                                    geometry.list_data.push_back_range(cc::as_byte_span(v));
                                    break;
                                }
                                case type::double_t:
                                {
                                    auto const v = parse_f64(s);
                                    geometry.list_data.push_back_range(cc::as_byte_span(v));
                                    break;
                                }
                                default:
                                    CC_UNREACHABLE("Invalid data type");
                                }
                            }
                            list_element.size = int(geometry.list_data.size()) - list_element.start_idx;
                            append_data(list_element);
                        }
                        else
                        {
                            // parse normal property
                            auto const t = next_token();
                            append_data_from_string(t, p.type);
                        }
                    }
                    if (it != end)
                    {
                        on_error(data, cc::as_byte_span(line), "Line contains unexpected extra data!", severity::warning);
                    }
                }
            }
        }
        else
        {
            CC_ASSERT(file_type::binary_big_endian == file_type || file_type::binary_little_endian == file_type);

            auto const copy = [&](cc::span<std::byte const> src, cc::span<std::byte> dst) {
                CC_ASSERT(src.size() == dst.size());
                if (file_type::binary_big_endian == file_type)
                    for (size_t i = 0; i < dst.size(); ++i)
                        dst[i] = src[dst.size() - 1 - i];
                else
                    src.copy_to(dst);
            };

            auto const read_next = [&](size_t n) -> cc::span<std::byte const> {
                if (pos + n >= data.size())
                {
                    on_error(data, data.last(0), "Failed to parse ply: Unexpected end of file!", severity::error);
                    return {};
                }
                else
                {
                    auto const start = pos;
                    pos += n;
                    return data.subspan(start, n);
                }
            };

            auto const append_data = [&](cc::span<std::byte const> d) {
                CC_ASSERT(data_idx + d.size() < data.size());
                copy(d, cc::span(&geometry.data[data_idx], d.size()));
                data_idx += d.size();
            };

            for (auto const& e : geometry.elements)
            {
                for (auto i = 0; i < e.count; ++i)
                {
                    auto const& p = geometry.properties[e.properties_start + i];

                    if (p.is_list())
                    {
                        geometry::list_property_entry list_property;
                        list_property.start_idx = int(geometry.list_data.size());
                        list_property.size = 0;
                        auto const raw = read_next(size_of(p.list_size_type));
                        switch (p.list_size_type)
                        {
                        case type::char_t:
                            list_property.size = cc::int8(raw[0]);
                            break;
                        case type::uchar_t:
                            list_property.size = cc::uint8(raw[0]);
                            break;
                        case type::short_t:
                        {
                            cc::int16 i;
                            copy(raw, cc::as_byte_span(i));
                            list_property.size = i;
                        }
                        case type::ushort_t:
                        {
                            cc::uint16 i;
                            copy(raw, cc::as_byte_span(i));
                            list_property.size = i;
                        }
                        case type::int_t:
                        {
                            cc::int32 i;
                            copy(raw, cc::as_byte_span(i));
                            list_property.size = i;
                        }
                        case type::uint_t:
                        {
                            cc::uint32 i;
                            copy(raw, cc::as_byte_span(i));
                            list_property.size = i;
                        }
                        default:
                            CC_UNREACHABLE("List size type must be integer");
                        }

                        if (list_property.size < 0)
                        {
                            on_error(data, raw, "List size cannot be negative!", severity::error);
                            list_property.size = 0;
                        }
                        geometry.list_data.resize(geometry.list_data.size() + list_property.size * size_of(p.type));
                        for (auto l = 0; l < list_property.size; ++l)
                        {
                            auto const src = read_next(size_of(p.type));
                            auto const dst = cc::span(&geometry.list_data[list_property.start_idx], size_of(p.type));
                            copy(src, dst);
                        }
                        list_property.size = int(geometry.list_data.size()) - list_property.start_idx;
                        append_data(cc::as_byte_span(list_property));
                    }
                    else
                    {
                        append_data(read_next(size_of(p.type)));
                    }
                }
            }
        }
    }
    return geometry;
}

bool babel::ply::geometry::has_element(cc::string_view name) const
{
    for (auto const& e : elements)
        if (e.name == name)
            return true;
    return false;
}

babel::ply::geometry::element& babel::ply::geometry::get_element(cc::string_view name)
{
    for (auto& e : elements)
        if (e.name == name)
            return e;

    CC_UNREACHABLE("element does not exist");
}

const babel::ply::geometry::element& babel::ply::geometry::get_element(cc::string_view name) const
{
    for (auto const& e : elements)
        if (e.name == name)
            return e;

    CC_UNREACHABLE("element does not exist");
}

bool babel::ply::geometry::has_property(babel::ply::geometry::element const& element, cc::string_view name) const
{
    for (auto i = 0; i < element.properties_count; ++i)
        if (properties[i + element.properties_start].name == name)
            return true;
    return false;
}

cc::span<babel::ply::geometry::property> babel::ply::geometry::get_properties(cc::string_view element_name)
{
    auto const& e = get_element(element_name);
    return cc::span(&properties[e.properties_start], e.properties_count);
}

cc::span<const babel::ply::geometry::property> babel::ply::geometry::get_properties(cc::string_view element_name) const
{
    auto const& e = get_element(element_name);
    return cc::span(&properties[e.properties_start], e.properties_count);
}

babel::ply::geometry::property& babel::ply::geometry::get_property(babel::ply::geometry::element const& element, cc::string_view name)
{
    for (auto i = 0; i < element.properties_count; ++i)
        if (properties[i + element.properties_start].name == name)
            return properties[i + element.properties_start];

    CC_UNREACHABLE("property does not exist");
}

const babel::ply::geometry::property& babel::ply::geometry::get_property(babel::ply::geometry::element const& element, cc::string_view name) const
{
    for (auto i = 0; i < element.properties_count; ++i)
        if (properties[i + element.properties_start].name == name)
            return properties[i + element.properties_start];

    CC_UNREACHABLE("property does not exist");
}
