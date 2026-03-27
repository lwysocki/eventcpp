#include <eventcpp/event.hpp>

#include <catch2/catch_test_macros.hpp>

namespace
{
    int call_count;
    auto count_calls = []() { call_count++; };
}

TEST_CASE("connection lifetime")
{
    event::event<void ()> e;
    call_count = 0;

    SECTION("connection does not disconnect on destruction")
    {
        {
            auto c = e.attach(count_calls);
        }
        e();
        REQUIRE(call_count == 1);
    }

    SECTION("disconnect through connection")
    {
        auto c = e.attach(count_calls);
        c.disconnect();
        e();
        REQUIRE(call_count == 0);
    }

    SECTION("double disconnection safety")
    {
        auto c = e.attach([]() {});
        c.disconnect();
        c.disconnect();
        SUCCEED("connection disconnection is idempotent");
    }
}

TEST_CASE("event lifetime check")
{
    event::connection<void ()> c;
    {
        event::event<void ()> e;
        c = e.attach([]() {});
    }

    c.disconnect();
    SUCCEED("disconnection from destroyed event is safe");
}

TEST_CASE("connection move semantics")
{
    event::event<void ()> e;
    call_count = 0;
    auto c = e.attach(count_calls);

    SECTION("move constructor")
    {
        event::connection<void ()> c2(std::move(c));
        c2.disconnect();
        e();
        REQUIRE(call_count == 0);
    }

    SECTION("move assignment")
    {
        event::connection<void ()> c2;
        c2 = std::move(c);
        c2.disconnect();
        e();
        REQUIRE(call_count == 0);
    }
}
