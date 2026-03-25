#include <eventcpp/event.hpp>

#include <catch2/catch_test_macros.hpp>

namespace
{
    int call_count = 0;

    struct member_function_tester
    {
        void func() { call_count++; }
        void another_func() { call_count++; }
    };
}

TEST_CASE("member function subscribers")
{
    event::event<void ()> e;
    call_count = 0;

    SECTION("attach and invoke")
    {
        member_function_tester tester;
        e.attach(&member_function_tester::func, tester);
        e();
        REQUIRE(call_count == 1);
    }
    SECTION("double detach safety")
    {
        member_function_tester tester;
        auto connection = e.attach(&member_function_tester::func, tester);
        e.detach(connection);
        e.detach(connection);
        e();
        REQUIRE(call_count == 0);
    }
    SECTION("member function with multiple objects")
    {
        member_function_tester tester;
        member_function_tester another_tester;
        e.attach(&member_function_tester::func, tester);
        e.attach(&member_function_tester::func, another_tester);
        e();
        REQUIRE(call_count == 2);
    }
    SECTION("multiple different member functions")
    {
        member_function_tester tester;
        e.attach(&member_function_tester::func, tester);
        e.attach(&member_function_tester::another_func, tester);
        e();
        REQUIRE(call_count == 2);
    }
}
