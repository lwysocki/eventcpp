#include <eventcpp/event.hpp>

#include <catch2/catch_test_macros.hpp>

int calls_count = 0;

void CountCalls()
{
    calls_count++;
}

TEST_CASE("empty event basic tests")
{
    SECTION("default construction")
    {
        event::event<void ()> e;

        e();

        SUCCEED("event created and called while empty without crashing");
    }
    SECTION("destruction")
    {
        {
            event::event<void ()> e;
        }
        SUCCEED("event destroyed without crashing");
    }
    SECTION("empty event returns default value")
    {
        event::event<int ()> e;
        REQUIRE(e() == 0);
    }
    SECTION("destruction with subscribers attached")
    {
        {
            event::event<void ()> e;
            e.attach(CountCalls);
        }
        SUCCEED("event with subscribers attached destroyed without crashing");
    }
    SECTION("move semantics")
    {
        event::event<void ()> e_org;
        e_org.attach(CountCalls);

        event::event<void ()> e_moved = std::move(e_org);
        e_moved();
        int current_count = calls_count;
        e_org();

        REQUIRE(calls_count == current_count);
    }
}
