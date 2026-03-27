#include <eventcpp/event.hpp>

#include <catch2/catch_test_macros.hpp>

namespace
{
    int call_count = 0;

    void count_calls() { call_count++; }
}

TEST_CASE("event sanity check")
{
    SECTION("instantiate event with default constructor")
    {
        event::event<void ()> e;
        e();

        SUCCEED("event created and called while empty without crashing");
    }
    SECTION("empty event destruction")
    {
        {
            event::event<void ()> e;
        }
        SUCCEED("event destroyed without crashing");
    }
    SECTION("destruction with subscribers attached")
    {
        {
            event::event<void ()> e;
            e.attach(count_calls);
        }
        SUCCEED("event with subscribers attached destroyed without crashing");
    }
    SECTION("empty event returns default value")
    {
        event::event<int ()> e;
        REQUIRE(e() == 0);
    }
    SECTION("detach from empty event")
    {
        event::event<void ()> e;
        event::connection<void ()> conn;
        e.detach(conn);

        SUCCEED("detached from empty event without crashing");
    }
}

TEST_CASE("event move semantics")
{
    event::event<void ()> e;
    e.attach(count_calls);

    SECTION("move constructor")
    {
        event::event<void ()> e2(std::move(e));
        e2();
        int current_count = call_count;
        e();

        REQUIRE(call_count == current_count);
    }

    SECTION("move assignment")
    {
        event::event<void ()> e2 = std::move(e);
        e2();
        int current_count = call_count;
        e();

        REQUIRE(call_count == current_count);
    }
}
