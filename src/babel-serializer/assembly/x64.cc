#include "x64.hh"

#include <babel-serializer/detail/log.hh>

#include <clean-core/flags.hh>
#include <clean-core/format.hh>

#include <rich-log/log.hh>

// explanations:
// - https://www-user.tu-chemnitz.de/~heha/hsn/chm/x86.chm/x64.htm
// - https://www.systutorials.com/beginners-guide-x86-64-instruction-encoding/
// - https://pyokagan.name/blog/2019-09-20-x86encoding/

// references:
// - https://wiki.osdev.org/X86-64_Instruction_Encoding
//   (contains register mapping at the beginning)
//   (contains parts of the instruction)
// - http://ref.x86asm.net/geek64.html
//   (field description in http://ref.x86asm.net/#column_flds )
// - https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html (waaay too long)
// - https://www.felixcloutier.com/x86/index.html

namespace babel::x64
{
namespace
{
constexpr reg64 reg64_from_op(std::byte op, bool extended) { return reg64((uint8_t(op) & 0b111) + (extended ? 8 : 0)); }
constexpr reg32 reg32_from_op(std::byte op) { return reg32((uint8_t(op) & 0b111)); }

//
// REX
//

// see https://wiki.osdev.org/X86-64_Instruction_Encoding#Encoding
constexpr bool is_rex(std::byte b)
{
    auto v = uint8_t(b);
    return (v & 0b11110000) == 0b01000000;
}
// precondition: is rex
// aka is REX.B set
constexpr bool is_rex_b(std::byte b)
{
    auto v = uint8_t(b);
    return v & 0b0001;
}
// precondition: is rex
// aka is REX.X set
constexpr bool is_rex_x(std::byte b)
{
    auto v = uint8_t(b);
    return v & 0b0010;
}
// precondition: is rex
// aka is REX.R set
constexpr bool is_rex_r(std::byte b)
{
    auto v = uint8_t(b);
    return v & 0b0100;
}
// precondition: is rex
// aka is REX.W set
constexpr bool is_rex_w(std::byte b)
{
    auto v = uint8_t(b);
    return v & 0b1000;
}

//
// ModR/M
//

enum class modrm_mode : uint8_t
{
    register_indirect = 0b00,
    memory_disp8 = 0b01,
    memory_disp32_64 = 0b10,
    register_direct = 0b11,
};
modrm_mode modrm_mode_of(std::byte b) { return modrm_mode(uint8_t(b) >> 6); }
uint8_t modrm_reg_of(std::byte b) { return (uint8_t(b) >> 3) & 0b111; }
uint8_t modrm_rm_of(std::byte b) { return uint8_t(b) & 0b111; }

//
// ModR/M
//

uint8_t sib_scale_of(std::byte b) { return 1 << (uint8_t(b) >> 6); }
uint8_t sib_index_of(std::byte b, std::byte rex) { return ((uint8_t(b) >> 3) & 0b111) + (is_rex_x(rex) ? 8 : 0); }
uint8_t sib_base_of(std::byte b, std::byte rex) { return (uint8_t(b) & 0b111) + (is_rex_b(rex) ? 8 : 0); }

//
// Tables
//

struct x64_op_info_t
{
    x64::mnemonic mnemonic[256] = {};
    arg_format args[256] = {};
    bool is_always_64bit[256] = {};

    // for stuff like add
    x64::mnemonic mnemonic_opext[256 * 8] = {};

    constexpr void set_op(uint8_t code, x64::mnemonic m, arg_format args)
    {
        CC_ASSERT(this->mnemonic[code] == x64::mnemonic::invalid && "already set");
        this->mnemonic[code] = m;
        this->args[code] = args;
    }
    constexpr void set_op64(uint8_t code, x64::mnemonic m, arg_format args)
    {
        CC_ASSERT(this->mnemonic[code] == x64::mnemonic::invalid && "already set");
        this->mnemonic[code] = m;
        this->args[code] = args;
        this->is_always_64bit[code] = true;
    }
};

static constexpr x64_op_info_t x64_op_info = []
{
    // TODO: generate me
    x64_op_info_t info;

    // NOTE: rex is handled separately

    // push / pop
    for (auto r = 0; r < 8; ++r)
        info.set_op64(0x50 + r, mnemonic::push, arg_format::opreg);
    for (auto r = 0; r < 8; ++r)
        info.set_op64(0x58 + r, mnemonic::pop, arg_format::opreg);

    // move
    info.set_op(0x89, mnemonic::mov, arg_format::modm_modr);
    info.set_op(0x8B, mnemonic::mov, arg_format::modr_modm);
    info.set_op(0x8D, mnemonic::lea, arg_format::modr_modm);

    // add/or/...
    {
        info.set_op(0x83, mnemonic::extended_resolve, arg_format::modm_imm8);
        info.mnemonic_opext[0x83 * 8 + 0] = mnemonic::add;
        info.mnemonic_opext[0x83 * 8 + 1] = mnemonic::or_;
        info.mnemonic_opext[0x83 * 8 + 2] = mnemonic::adc;
        info.mnemonic_opext[0x83 * 8 + 3] = mnemonic::sbb;
        info.mnemonic_opext[0x83 * 8 + 4] = mnemonic::and_;
        info.mnemonic_opext[0x83 * 8 + 5] = mnemonic::sub;
        info.mnemonic_opext[0x83 * 8 + 6] = mnemonic::xor_;
        info.mnemonic_opext[0x83 * 8 + 7] = mnemonic::cmp;
    }

    // add
    info.set_op(0x03, mnemonic::add, arg_format::modr_modm);

    // call
    info.set_op(0xE8, mnemonic::call, arg_format::imm32);

    // ret
    info.set_op(0xC3, mnemonic::ret, arg_format::none);

    return info;
}();

void add_opreg_to_string(cc::string& s, babel::x64::instruction const& in)
{
    auto has_rex = in.rex != std::byte(0);

    if (x64_op_info.is_always_64bit[int(in.opcode)])
        s += x64::to_string(reg64_from_op(in.opcode, false));
    else if (has_rex && is_rex_w(in.rex))
    {
        LOG_WARN("TODO: which rex byte is used here?");
        CC_UNREACHABLE("TODO");
        // s += x64::to_string(reg64_from_op(opcode, is_rex_b(rex)));
    }
    else
        s += x64::to_string(reg32_from_op(in.opcode));
}

void add_modr_to_string(cc::string& s, babel::x64::instruction const& in)
{
    auto has_rex = in.rex != std::byte(0);

    CC_ASSERT(!x64_op_info.is_always_64bit[int(in.opcode)] && "TODO");

    auto const modrm = in.data[in.format.offset_modrm];
    // auto const mode = modrm_mode_of(modrm);
    // CC_ASSERT(mode == modrm_mode::register_direct && "TODO"); -- is always register, right?

    auto regi = modrm_reg_of(modrm);

    // 64bit
    if (has_rex && is_rex_w(in.rex))
    {
        if (is_rex_r(in.rex))
            regi += 8;

        s += x64::to_string(reg64(regi));
    }
    else // 32bit
    {
        s += x64::to_string(reg32(regi));
    }
}

void add_disp8_to_string(cc::string& s, std::byte d)
{
    auto v = int8_t(d);
    if (v >= 0)
    {
        s += " + 0x";
        s += cc::to_string(d);
    }
    else
    {
        s += " - 0x";
        s += cc::to_string(std::byte(-v));
    }
}

void add_modm_to_string(cc::string& s, babel::x64::instruction const& in)
{
    auto has_rex = in.rex != std::byte(0);

    CC_ASSERT(!x64_op_info.is_always_64bit[int(in.opcode)] && "TODO");

    auto const modrm = in.data[in.format.offset_modrm];
    auto const mode = modrm_mode_of(modrm);

    if (mode != modrm_mode::register_direct)
        s += '[';

    auto regi = modrm_rm_of(modrm);
    if (has_rex && is_rex_b(in.rex))
        regi += 8;

    auto skip_plus = false;

    // SIB
    if (mode != modrm_mode::register_direct && regi == 0b100)
    {
        CC_ASSERT(in.format.offset_sib > 0 && "no sib");
        auto const sib = in.data[in.format.offset_sib];
        auto const scale = sib_scale_of(sib);
        auto const index = sib_index_of(sib, in.rex);
        auto const base = sib_base_of(sib, in.rex);

        auto const no_base_reg = mode == modrm_mode::register_indirect && (base & 0b111) == 0b101;
        auto const no_index_reg = mode != modrm_mode::register_indirect && index == 0b0100;

        if (no_index_reg && no_base_reg)
        {
            // only disp
            skip_plus = true;
        }
        else if (no_index_reg)
        {
            s += x64::to_string(reg64(base));
        }
        else if (no_base_reg)
        {
            s += x64::to_string(reg64(index));
            s += " * ";
            s += cc::to_string(scale);
        }
        else
        {
            s += x64::to_string(reg64(base));
            s += " + ";
            s += x64::to_string(reg64(index));
            s += " * ";
            s += cc::to_string(scale);
        }
    }
    else if (mode == modrm_mode::register_indirect && regi == 0b101)
    {
        CC_UNREACHABLE("TODO: impl RIP+disp");
    }
    // r/m register
    else
    {
        if (mode != modrm_mode::register_direct || is_rex_w(in.rex))
            s += x64::to_string(reg64(regi));
        else
            s += x64::to_string(reg32(regi));
    }

    // displacement
    if (mode == modrm_mode::memory_disp8)
    {
        CC_ASSERT(in.format.offset_displacement > 0 && "instruction has no disp set");
        add_disp8_to_string(s, in.data[in.format.offset_displacement]);
    }
    else if (mode == modrm_mode::memory_disp32_64)
    {
        CC_ASSERT(in.format.offset_displacement > 0 && "instruction has no disp set");
        if (!skip_plus)
            s += " + ";
        s += "0x";
        for (auto i = 3; i >= 0; --i)
            s += cc::to_string(in.data[in.format.offset_displacement + i]);
    }

    if (mode != modrm_mode::register_direct)
        s += ']';
}

void add_imm8_to_string(cc::string& s, babel::x64::instruction const& in)
{
    CC_ASSERT(in.format.offset_immediate > 0 && "no immediate available");
    s += "0x";
    s += cc::to_string(in.data[in.format.offset_immediate]);
}

void add_imm32_to_string(cc::string& s, babel::x64::instruction const& in)
{
    CC_ASSERT(in.format.offset_immediate > 0 && "no immediate available");
    s += "0x";
    for (auto i = 3; i >= 0; --i)
        s += cc::to_string(in.data[in.format.offset_immediate + i]);
}

} // namespace
} // namespace babel::x64

babel::x64::instruction babel::x64::decode_one(std::byte const* data, std::byte const* end)
{
    if (data >= end)
        return {};

    instruction instr;
    instr.data = data;
    instr.opcode = *data++;

    // rex prefix
    if (is_rex(instr.opcode))
    {
        if (data >= end)
            return {};

        instr.rex = instr.opcode;
        instr.opcode = *data++;
        instr.format.offset_op++;
    }

    // look up primary op
    // TODO: single load?
    instr.mnemonic = x64_op_info.mnemonic[int(instr.opcode)];
    instr.format.args = x64_op_info.args[int(instr.opcode)];
    if (instr.mnemonic == mnemonic::invalid)
    {
        LOG_WARN("unknown instruction for byte 0x%s (in %s)", instr.opcode, cc::span<std::byte const>(instr.data, cc::min(instr.data + 16, end)));
        return {};
    }

    // ModR/M
    if (has_modrm(instr.format.args))
    {
        if (data >= end)
            return {};

        instr.format.offset_modrm = data - instr.data;
        auto modrm = *data++;
        auto mod = modrm_mode_of(modrm);

        // extended mnemonic
        if (instr.mnemonic == mnemonic::extended_resolve)
            instr.mnemonic = x64_op_info.mnemonic_opext[int(instr.opcode) * 8 + modrm_reg_of(modrm)];
        if (instr.mnemonic == mnemonic::invalid)
        {
            LOG_WARN("unknown instruction for byte 0x%s (in %s)", instr.opcode, cc::span<std::byte const>(instr.data, cc::min(instr.data + 16, end)));
            return {};
        }

        // see https://wiki.osdev.org/X86-64_Instruction_Encoding#32.2F64-bit_addressing

        // SIB
        if (mod != modrm_mode::register_direct && modrm_rm_of(modrm) == 0b100)
        {
            if (data >= end)
                return {};

            instr.format.offset_sib = data - instr.data;
            data++;
        }

        // disp8
        if (mod == modrm_mode::memory_disp8)
        {
            if (data >= end)
                return {};

            instr.format.offset_displacement = data - instr.data;
            instr.format.size_displacement = 0;
            data++;
        }
        // disp32
        else if (mod == modrm_mode::memory_disp32_64)
        {
            if (data + 3 >= end)
                return {};

            // TODO: is there 64 bit displacement?
            instr.format.offset_displacement = data - instr.data;
            instr.format.size_displacement = 2;
            data += 4;
        }

        // special
        CC_ASSERT(!(mod == modrm_mode::register_indirect && modrm_rm_of(modrm) == 0b101) && "TODO: implement RIP/EIP+disp32");
    }

    // immediate args
    switch (instr.format.args)
    {
    case arg_format::opreg_imm:
        CC_UNREACHABLE("TODO");
        break;

    case arg_format::imm32:
        if (data + 3 >= end)
            return {};
        instr.format.offset_immediate = data - instr.data;
        instr.format.size_immediate = 2;
        data += 4;
        break;

    case arg_format::modm_imm8:
        if (data >= end)
            return {};
        instr.format.offset_immediate = data - instr.data;
        instr.format.size_immediate = 0;
        data++;
        break;

    case arg_format::modm:
    case arg_format::modm_modr:
    case arg_format::modr_modm:
    case arg_format::opreg:
    case arg_format::none:
        // no immediate
        break;
    }

    instr.size = data - instr.data;
    return instr;
}

char const* babel::x64::to_string(mnemonic m)
{
    switch (m)
    {
    case mnemonic::invalid:
        return "<invalid-op>";
    case mnemonic::extended_resolve:
        return "<unresolved-extended-mnemonic>";

    case mnemonic::push:
        return "push";
    case mnemonic::pop:
        return "pop";

    case mnemonic::mov:
        return "mov";
    case mnemonic::lea:
        return "lea";

    case mnemonic::call:
        return "call";
    case mnemonic::ret:
        return "ret";

    case mnemonic::add:
        return "add";
    case mnemonic::or_:
        return "or";
    case mnemonic::adc:
        return "adc";
    case mnemonic::sbb:
        return "sbb";
    case mnemonic::and_:
        return "and";
    case mnemonic::sub:
        return "sub";
    case mnemonic::xor_:
        return "xor";
    case mnemonic::cmp:
        return "cmp";
    }
    return "<unknown-op>";
}

cc::string babel::x64::instruction::to_string() const
{
    if (!is_valid())
        return "<invalid instruction>";

    cc::string s;
    s += x64::to_string(mnemonic);

    switch (format.args)
    {
        // 0 args
    case arg_format::none:
        break;

        // 1 arg
    case arg_format::opreg:
        s += ' '; // TODO: pad?
        add_opreg_to_string(s, *this);
        break;
    case arg_format::imm32:
        s += ' '; // TODO: pad?
        add_imm32_to_string(s, *this);
        break;
    case arg_format::modm:
        s += ' '; // TODO: pad?
        add_modm_to_string(s, *this);
        break;

        // 2 args
    case arg_format::modm_modr:
        s += ' '; // TODO: pad?
        add_modm_to_string(s, *this);
        s += ", ";
        add_modr_to_string(s, *this);
        break;
    case arg_format::modr_modm:
        s += ' '; // TODO: pad?
        add_modr_to_string(s, *this);
        s += ", ";
        add_modm_to_string(s, *this);
        break;
    case arg_format::opreg_imm:
        CC_UNREACHABLE("TODO");
        break;
    case arg_format::modm_imm8:
        s += ' '; // TODO: pad?
        add_modm_to_string(s, *this);
        s += ", ";
        add_imm8_to_string(s, *this);
        break;
    }

    return s;
}
