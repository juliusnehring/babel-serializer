#include <nexus/test.hh>

#include <clean-core/experimental/ringbuffer.hh>
#include <clean-core/set.hh>
#include <clean-core/vector.hh>

#include <babel-serializer/assembly/x64.hh>

#include <rich-log/log.hh>

static int test_add(int a, int b) { return a + b; }
static int test_min(int a, int b) { return a < b ? a : b; }
static int test_abs(int a, int b) { return std::abs(a - b); }
static int test_sum(cc::span<int const> vals)
{
    auto s = 0;
    for (auto i : vals)
        s += i;
    return s;
}
static int test_sum2(int* a, int *b)
{
    auto s = 0;
    while (a != b)
        s += *a++;
    return s;
}

TEST("x64 assembly basic decode")
{
    cc::set<std::byte const*> seen_starts;
    cc::ringbuffer<std::byte const*> start_queue;
    // start_queue.push_back((std::byte const*)test_abs);
    // start_queue.push_back((std::byte const*)test_add);
    // start_queue.push_back((std::byte const*)test_min);
    start_queue.push_back((std::byte const*)test_sum2);

    // auto fun = +[](int a, int b) { return a + b; };
    // start_queue.push_back((std::byte const*)fun);

    auto decode_block = [&](std::byte const* p_start)
    {
        if (!seen_starts.add(p_start))
            return; // already decoded

        LOG("");
        LOG("block: %s ...", cc::span(p_start, p_start + 30));
        auto p_end = p_start + 5000; // TODO: how to get a more reliable estimate? get readable pages?

        while (true)
        {
            auto i = babel::x64::decode_one(p_start, p_end);
            if (!i.is_valid())
                break;

            LOG("%s %<33s %s", i.data, i.as_span(), i);
            p_start += i.size;

            if (is_return(i)) // ret -> end
                break;

            if (is_unconditional_jump(i)) // jmp -> end
            {
                start_queue.push_back(unconditional_jump_target(i));
                // or just continue block directly there?
                break;
            }

            if (is_relative_call(i)) // call -> block continues
                start_queue.push_back(relative_call_target(i));

            if (is_conditional_jump(i)) // jzz -> block might continue
                start_queue.push_back(conditional_jump_target(i));
        }
    };

    while (!start_queue.empty())
    {
        auto p = start_queue.pop_back();
        decode_block(p);
    }
}
