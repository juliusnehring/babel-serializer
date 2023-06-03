// CAUTION: this file is auto-generated. DO NOT MODIFY!
#include "x64.hh"
#include "x64.gen.hh"

static char const* s_mnemonic_names[] = {
    "<invalid-mnemonic>",
    "<unresolved-mnemonic>",
    "aaa",
    "aad",
    "aam",
    "aas",
    "adc",
    "add",
    "addpd",
    "addps",
    "addsd",
    "addss",
    "addsubpd",
    "addsubps",
    "adx",
    "alter",
    "amx",
    "and",
    "andnpd",
    "andnps",
    "andpd",
    "andps",
    "arpl",
    "blendpd",
    "blendps",
    "blendvpd",
    "blendvps",
    "bound",
    "bsf",
    "bsr",
    "bswap",
    "bt",
    "btc",
    "btr",
    "bts",
    "call",
    "callf",
    "cbw",
    "cdq",
    "cdqe",
    "clc",
    "cld",
    "clflush",
    "cli",
    "cmc",
    "cmova",
    "cmovae",
    "cmovb",
    "cmovbe",
    "cmovc",
    "cmove",
    "cmovg",
    "cmovge",
    "cmovl",
    "cmovle",
    "cmovna",
    "cmovnae",
    "cmovnb",
    "cmovnbe",
    "cmovnc",
    "cmovne",
    "cmovng",
    "cmovnge",
    "cmovnl",
    "cmovnle",
    "cmovno",
    "cmovnp",
    "cmovns",
    "cmovnz",
    "cmovo",
    "cmovp",
    "cmovpe",
    "cmovpo",
    "cmovs",
    "cmovz",
    "cmp",
    "cmppd",
    "cmpps",
    "cmps",
    "cmpsb",
    "cmpsd",
    "cmpsq",
    "cmpss",
    "cmpsw",
    "cmpxchg",
    "cmpxchg16b",
    "cmpxchg8b",
    "comisd",
    "comiss",
    "cpuid",
    "cqo",
    "crc32",
    "cs",
    "cvtdq2pd",
    "cvtdq2ps",
    "cvtpd2dq",
    "cvtpd2pi",
    "cvtpd2ps",
    "cvtpi2pd",
    "cvtpi2ps",
    "cvtps2dq",
    "cvtps2pd",
    "cvtps2pi",
    "cvtsd2si",
    "cvtsd2ss",
    "cvtsi2sd",
    "cvtsi2ss",
    "cvtss2sd",
    "cvtss2si",
    "cvttpd2dq",
    "cvttpd2pi",
    "cvttps2dq",
    "cvttps2pi",
    "cvttsd2si",
    "cvttss2si",
    "cwd",
    "cwde",
    "daa",
    "das",
    "dec",
    "div",
    "divpd",
    "divps",
    "divsd",
    "divss",
    "dppd",
    "dpps",
    "ds",
    "emms",
    "enter",
    "es",
    "extractps",
    "f2xm1",
    "fabs",
    "fadd",
    "faddp",
    "fbld",
    "fbstp",
    "fchs",
    "fclex",
    "fcmovb",
    "fcmovbe",
    "fcmove",
    "fcmovnb",
    "fcmovnbe",
    "fcmovne",
    "fcmovnu",
    "fcmovu",
    "fcom",
    "fcom2",
    "fcomi",
    "fcomip",
    "fcomp",
    "fcomp3",
    "fcomp5",
    "fcompp",
    "fcos",
    "fdecstp",
    "fdisi",
    "fdiv",
    "fdivp",
    "fdivr",
    "fdivrp",
    "feni",
    "ffree",
    "ffreep",
    "fiadd",
    "ficom",
    "ficomp",
    "fidiv",
    "fidivr",
    "fild",
    "fimul",
    "fincstp",
    "finit",
    "fist",
    "fistp",
    "fisttp",
    "fisub",
    "fisubr",
    "fld",
    "fld1",
    "fldcw",
    "fldenv",
    "fldl2e",
    "fldl2t",
    "fldlg2",
    "fldln2",
    "fldpi",
    "fldz",
    "fmul",
    "fmulp",
    "fnclex",
    "fndisi",
    "fneni",
    "fninit",
    "fnop",
    "fnsave",
    "fnsetpm",
    "fnstcw",
    "fnstenv",
    "fnstsw",
    "fpatan",
    "fprem",
    "fprem1",
    "fptan",
    "frndint",
    "frstor",
    "fs",
    "fsave",
    "fscale",
    "fsetpm",
    "fsin",
    "fsincos",
    "fsqrt",
    "fst",
    "fstcw",
    "fstenv",
    "fstp",
    "fstp1",
    "fstp8",
    "fstp9",
    "fstsw",
    "fsub",
    "fsubp",
    "fsubr",
    "fsubrp",
    "ftst",
    "fucom",
    "fucomi",
    "fucomip",
    "fucomp",
    "fucompp",
    "fwait",
    "fxam",
    "fxch",
    "fxch4",
    "fxch7",
    "fxrstor",
    "fxsave",
    "fxtract",
    "fyl2x",
    "fyl2xp1",
    "getsec",
    "gs",
    "haddpd",
    "haddps",
    "hint_nop",
    "hsubpd",
    "hsubps",
    "icebp",
    "idiv",
    "imul",
    "in",
    "inc",
    "ins",
    "insb",
    "insd",
    "insertps",
    "insw",
    "int",
    "int1",
    "into",
    "iret",
    "iretd",
    "iretq",
    "ja",
    "jae",
    "jb",
    "jbe",
    "jcxz",
    "je",
    "jecxz",
    "jg",
    "jge",
    "jl",
    "jle",
    "jmp",
    "jmpe",
    "jmpf",
    "jne",
    "jno",
    "jns",
    "jo",
    "jpe",
    "jpo",
    "jrcxz",
    "js",
    "lahf",
    "lddqu",
    "ldmxcsr",
    "lds",
    "lea",
    "leave",
    "les",
    "lfence",
    "lfs",
    "lgs",
    "lock",
    "lods",
    "lodsb",
    "lodsd",
    "lodsq",
    "lodsw",
    "loop",
    "loope",
    "loopne",
    "loopnz",
    "loopz",
    "lss",
    "maskmovdqu",
    "maskmovq",
    "maxpd",
    "maxps",
    "maxsd",
    "maxss",
    "mfence",
    "minpd",
    "minps",
    "minsd",
    "minss",
    "mov",
    "movapd",
    "movaps",
    "movbe",
    "movd",
    "movddup",
    "movdq2q",
    "movdqa",
    "movdqu",
    "movhlps",
    "movhpd",
    "movhps",
    "movlhps",
    "movlpd",
    "movlps",
    "movmskpd",
    "movmskps",
    "movntdq",
    "movntdqa",
    "movnti",
    "movntpd",
    "movntps",
    "movntq",
    "movq",
    "movq2dq",
    "movs",
    "movsb",
    "movsd",
    "movshdup",
    "movsldup",
    "movsq",
    "movss",
    "movsw",
    "movsx",
    "movsxd",
    "movupd",
    "movups",
    "movzx",
    "mpsadbw",
    "mul",
    "mulpd",
    "mulps",
    "mulsd",
    "mulss",
    "neg",
    "nop",
    "not",
    "ntaken",
    "or",
    "orpd",
    "orps",
    "out",
    "outs",
    "outsb",
    "outsd",
    "outsw",
    "pabsb",
    "pabsd",
    "pabsw",
    "packssdw",
    "packsswb",
    "packusdw",
    "packuswb",
    "paddb",
    "paddd",
    "paddq",
    "paddsb",
    "paddsw",
    "paddusb",
    "paddusw",
    "paddw",
    "palignr",
    "pand",
    "pandn",
    "pause",
    "pavgb",
    "pavgw",
    "pblendvb",
    "pblendw",
    "pcmpeqb",
    "pcmpeqd",
    "pcmpeqq",
    "pcmpeqw",
    "pcmpestri",
    "pcmpestrm",
    "pcmpgtb",
    "pcmpgtd",
    "pcmpgtq",
    "pcmpgtw",
    "pcmpistri",
    "pcmpistrm",
    "pextrb",
    "pextrd",
    "pextrq",
    "pextrw",
    "phaddd",
    "phaddsw",
    "phaddw",
    "phminposuw",
    "phsubd",
    "phsubsw",
    "phsubw",
    "pinsrb",
    "pinsrd",
    "pinsrq",
    "pinsrw",
    "pmaddubsw",
    "pmaddwd",
    "pmaxsb",
    "pmaxsd",
    "pmaxsw",
    "pmaxub",
    "pmaxud",
    "pmaxuw",
    "pminsb",
    "pminsd",
    "pminsw",
    "pminub",
    "pminud",
    "pminuw",
    "pmovmskb",
    "pmovsxbd",
    "pmovsxbq",
    "pmovsxbw",
    "pmovsxdq",
    "pmovsxwd",
    "pmovsxwq",
    "pmovzxbd",
    "pmovzxbq",
    "pmovzxbw",
    "pmovzxdq",
    "pmovzxwd",
    "pmovzxwq",
    "pmuldq",
    "pmulhrsw",
    "pmulhuw",
    "pmulhw",
    "pmulld",
    "pmullw",
    "pmuludq",
    "pop",
    "popa",
    "popad",
    "popcnt",
    "popf",
    "popfd",
    "popfq",
    "por",
    "prefetchnta",
    "prefetcht0",
    "prefetcht1",
    "prefetcht2",
    "psadbw",
    "pshufb",
    "pshufd",
    "pshufhw",
    "pshuflw",
    "pshufw",
    "psignb",
    "psignd",
    "psignw",
    "pslld",
    "pslldq",
    "psllq",
    "psllw",
    "psrad",
    "psraw",
    "psrld",
    "psrldq",
    "psrlq",
    "psrlw",
    "psubb",
    "psubd",
    "psubq",
    "psubsb",
    "psubsw",
    "psubusb",
    "psubusw",
    "psubw",
    "ptest",
    "punpckhbw",
    "punpckhdq",
    "punpckhqdq",
    "punpckhwd",
    "punpcklbw",
    "punpckldq",
    "punpcklqdq",
    "punpcklwd",
    "push",
    "pusha",
    "pushad",
    "pushf",
    "pushfd",
    "pushfq",
    "pxor",
    "rcl",
    "rcpps",
    "rcpss",
    "rcr",
    "rdpmc",
    "rdtsc",
    "rdtscp",
    "rep",
    "repe",
    "repne",
    "repnz",
    "repz",
    "retf",
    "retn",
    "rol",
    "ror",
    "roundpd",
    "roundps",
    "roundsd",
    "roundss",
    "rsqrtps",
    "rsqrtss",
    "sahf",
    "sal",
    "salc",
    "sar",
    "sbb",
    "scas",
    "scasb",
    "scasd",
    "scasq",
    "scasw",
    "seta",
    "setae",
    "setalc",
    "setb",
    "setbe",
    "setc",
    "sete",
    "setg",
    "setge",
    "setl",
    "setle",
    "setna",
    "setnae",
    "setnb",
    "setnbe",
    "setnc",
    "setne",
    "setng",
    "setnge",
    "setnl",
    "setnle",
    "setno",
    "setnp",
    "setns",
    "setnz",
    "seto",
    "setp",
    "setpe",
    "setpo",
    "sets",
    "setz",
    "sfence",
    "sgdt",
    "shld",
    "shr",
    "shrd",
    "shufpd",
    "shufps",
    "sidt",
    "smsw",
    "sqrtpd",
    "sqrtps",
    "sqrtsd",
    "sqrtss",
    "ss",
    "stc",
    "std",
    "sti",
    "stmxcsr",
    "stos",
    "stosb",
    "stosd",
    "stosq",
    "stosw",
    "sub",
    "subpd",
    "subps",
    "subsd",
    "subss",
    "syscall",
    "sysenter",
    "taken",
    "test",
    "ucomisd",
    "ucomiss",
    "ud",
    "ud2",
    "unpckhpd",
    "unpckhps",
    "unpcklpd",
    "unpcklps",
    "wait",
    "xadd",
    "xchg",
    "xgetbv",
    "xlat",
    "xlatb",
    "xor",
    "xorpd",
    "xorps",
    "xrstor",
    "xsave",
};

char const* babel::x64::to_string(mnemonic m)
{
    if (m > mnemonic::xsave)
        return "<unknown-mnemonic>";
    return s_mnemonic_names[int(m)];
}

