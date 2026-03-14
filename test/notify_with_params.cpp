#include <algorithm>
#include <eventcpp/event.hpp>

#include <catch2/catch_test_macros.hpp>
#include <memory>

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
    SECTION("lambda subscriber")
    {
        auto lambda = [](int val) { return val - 1; };
        e.attach(lambda);

        int ret = e(1);
        REQUIRE(ret == 0);
    }
}

TEST_CASE("event should perfectly forward move-only arguments")
{
    event::event<int (std::unique_ptr<int>)> e;

    e.attach([](std::unique_ptr<int> ptr) {
        return *ptr;
    });

    auto ptr = std::make_unique<int>(3);
    int result = e(std::move(ptr));

    REQUIRE(result == 3);
    REQUIRE(ptr == nullptr);
}
