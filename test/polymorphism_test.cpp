#include <eventcpp/event.hpp>

#include <catch2/catch_test_macros.hpp>

namespace
{
    int call_count = 0;

    class base
    {
    public:
        virtual void func() = 0;
    };

    class another_base
    {
    public:
        virtual void func() = 0;
    };

    class polymorphism_tester : public base, public another_base
    {
    public:
        void func() override { call_count++; }
    };
}

TEST_CASE("polymorphic subscribers")
{
    event::event<void ()> e;
    call_count = 0;

    SECTION("attach call detach call")
    {
        polymorphism_tester tester;
        auto connection = e.attach(&base::func, tester);
        e();
        e.detach(connection);
        e();

        REQUIRE(call_count == 1);
    }

    SECTION("subscribe from different base classes")
    {
        polymorphism_tester tester;
        auto connection = e.attach(&base::func, tester);
        auto another_connection = e.attach(&another_base::func, tester);
        e();
        e.detach(connection);
        e();
        e.detach(another_connection);
        e();

        REQUIRE(call_count == 3);
    }
}
