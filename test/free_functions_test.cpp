#include <eventcpp/event.hpp>

#include <catch2/catch_test_macros.hpp>

namespace
{
    int call_count = 0;
}

TEST_CASE("lambda subscribers")
{
    event::event<void ()> e;
    call_count = 0;
    auto func = []() { call_count++; };
    auto another_func = []() { call_count++; };

    SECTION("attach and invoke")
    {
        e.attach(func);
        e();
        REQUIRE(call_count == 1);
    }
    SECTION("double detach safety")
    {
        e.attach(func);
        e.detach(func);
        e.detach(func);
        e();
        REQUIRE(call_count == 0);
    }
    SECTION("multiple different functions")
    {
        e.attach(func);
        e.attach(another_func);
        e();
        REQUIRE(call_count == 2);
    }
}
