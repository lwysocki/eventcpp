#include <array>

#include <eventcpp/event.hpp>

#include <catch2/catch_test_macros.hpp>

std::array<int, 6> state;

void Func1()
{
    state[0] = 1;
}

void Func2()
{
    state[1] = 1;
}

class StaticExample
{
public:
    static void Mem1()
    {
        state[2] = 1;
    }

    static void Mem2()
    {
        state[3] = 1;
    }
};

class ObjExample
{
    int _idx;

public:
    ObjExample(int idx): _idx(idx) {}

    void Mem()
    {
        state[_idx] = 1;
    }
};

TEST_CASE("event should notify all subscribers")
{
    event::event<void ()> e;
    state = { 0, 0, 0, 0, 0, 0 };
    ObjExample obj1(4);
    ObjExample obj2(5);

    e.attach(Func1);
    e.attach(Func2);
    e.attach(StaticExample::Mem1);
    e.attach(StaticExample::Mem2);
    e.attach(&ObjExample::Mem, obj1);
    e.attach(&ObjExample::Mem, obj2);

    e();
    std::array<int, 6> expected = { 1, 1, 1, 1, 1 , 1 };

    REQUIRE(state == expected);
}