#include <eventcpp/event.hpp>

#include <catch2/catch_test_macros.hpp>

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

class ObjExample
{
public:
    int Half(int val)
    {
        return val / 2;
    }
};

TEST_CASE("event should notify subscribers and passing them parameters")
{
    event::event<int (int)> e;

    SECTION("function subscriber")
    {
        e.attach(Double);

        int ret = e(3);
        REQUIRE(ret == 6);
    }
    SECTION("static member function subscriber")
    {
        e.attach(StaticExample::Triple);

        int ret = e(4);
        REQUIRE(ret == 12);
    }
    SECTION("member function subscriber")
    {
        ObjExample obj;
        e.attach(&ObjExample::Half, obj);

        int ret = e(8);
        REQUIRE(ret == 4);
    }
}