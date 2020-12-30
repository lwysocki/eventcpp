#include <eventcpp/event.hpp>

#include <catch2/catch.hpp>

int Double(int val)
{
    return val * 2;
}

class StaticExample
{
public:
    static int Triple(int val)
    {
        return val * 3;
    }
};

TEST_CASE("event should notify subscribers")
{
    SECTION("function subscriber")
    {
        event::event<int (int)> e;
        e.attach(Double);

        int ret = e(3);

        REQUIRE(ret == 6);
    }
    SECTION("static member function subscriber")
    {
        event::event<int (int)> e;
        e.attach(StaticExample::Triple);

        int ret = e(4);

        REQUIRE(ret == 12);
    }
}