#include <eventcpp/event.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("return type basic tests")
{
    SECTION("return void")
    {
        auto lambda = []() { return; };
        event::event<void ()> e;
        e.attach(lambda);

        e();

        SUCCEED("");
    }
    SECTION("return non-void type")
    {
        auto lambda = []() { return 1; };
        event::event<int ()> e;
        e.attach(lambda);

        int value = e();

        REQUIRE(value == 1);
    }
}
