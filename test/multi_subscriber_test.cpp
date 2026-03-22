#include <array>

#include <eventcpp/event.hpp>

#include <catch2/catch_test_macros.hpp>
#include <memory>

namespace
{
    std::array<int, 5> state;
    std::array<int, 5> expected_state = { 1, 1, 1, 1, 1 };

    void func() { state[0] = 1; }

    struct static_memeber_tester
    {
        static void func() { state[2] = 1; }
    };

    class member_function_tester
    {
        int _idx;

    public:
        member_function_tester(int idx): _idx(idx) {}

        void func() { state[_idx] = 1; }
    };

    class return_value_tester
    {
        int _value;

    public:
        return_value_tester(int value): _value(value) {}

        int func() { return _value; }
    };
}

TEST_CASE("event notify all subscribers")
{
    event::event<void ()> e;
    state = { 0, 0, 0, 0, 0 };

    member_function_tester obj_ref(3);
    std::unique_ptr<member_function_tester> obj_by_ptr = std::make_unique<member_function_tester>(4);

    e.attach(func);
    e.attach([]() { state[1] = 1; });
    e.attach(static_memeber_tester::func);
    e.attach(&member_function_tester::func, obj_ref);
    e.attach(&member_function_tester::func, obj_by_ptr.get());

    e();

    REQUIRE(state == expected_state);
}

TEST_CASE("multiple subscribers return value")
{
    event::event<int ()> e;

    return_value_tester tester1(1);
    e.attach(&return_value_tester::func, tester1);
    return_value_tester tester2(2);
    e.attach(&return_value_tester::func, tester2);
    int returned_value = e();

    REQUIRE((returned_value == 1 || returned_value == 2));
}
