#include <eventcpp/event.hpp>

#include <catch2/catch_test_macros.hpp>
#include <memory>

namespace
{
    struct signature_tester
    {
        int by_value(int i) { return i * 2; }
        int by_reference(int& i)
        {
            i *= 2;
            return i;
        }
        int by_pointer(int* i)
        {
            *i *= 2;
            return *i;
        }
        int by_const_reference(const int& i) { return i; }
    };
}

TEST_CASE("parameter passed by value")
{
    event::event<int (int)> e;
    int test_value = 10;
    int expected_value = test_value * 2;

    SECTION("free function/lambda")
    {
        auto lambda = [](int i) { return i * 2; };
        e.attach(lambda);

        int returned_value = e(test_value);

        REQUIRE(returned_value == expected_value);
    }
    SECTION("member function")
    {
        signature_tester tester;
        e.attach(&signature_tester::by_value, tester);

        int returned_value = e(test_value);

        REQUIRE(returned_value == expected_value);
    }
}

TEST_CASE("parameter passed by reference")
{
    event::event<int (int&)> e;
    int test_value = 10;
    int expected_value = test_value * 2;

    SECTION("free function/lambda")
    {
        auto lambda = [](int& i)
        {
            i *= 2;
            return i;
        };
        e.attach(lambda);

        int returned_value = e(test_value);

        REQUIRE(returned_value == expected_value);
    }
    SECTION("member function")
    {
        signature_tester tester;
        e.attach(&signature_tester::by_reference, tester);

        int returned_value = e(test_value);

        REQUIRE(returned_value == expected_value);
    }
}

TEST_CASE("parameter passed by pointer")
{
    event::event<int (int*)> e;
    std::shared_ptr<int> test_value = std::make_shared<int>(10);
    int expected_value = *test_value * 2;

    SECTION("free function/lambda")
    {
        auto lambda = [](int* i)
        {
            *i *= 2;
            return *i;
        };
        e.attach(lambda);

        int returned_value = e(test_value.get());

        REQUIRE(returned_value == expected_value);
    }
    SECTION("member function")
    {
        signature_tester tester;
        e.attach(&signature_tester::by_pointer, tester);

        int returned_value = e(test_value.get());

        REQUIRE(returned_value == expected_value);
    }
}

// TEST_CASE("const-correctness")
// {
//     event::event<int (int&)> e;
//     int test_value = 10;

    // SECTION("free function/lambda")
    // {
    //     auto lambda = [](const int& i)
    //     {
    //         return i;
    //     };
    //     e.attach(lambda);

    //     int test_value = 1;

    //     REQUIRE(e(test_value) == test_value);
    // }
//     SECTION("member function")
//     {
//         signature_tester tester;
//         e.attach(&signature_tester::by_const_reference, tester);

//         int returned_value = e(test_value);

//         REQUIRE(returned_value == test_value);
//     }
// }

TEST_CASE("return type")
{
    SECTION("return void")
    {
        auto lambda = []() { return; };
        event::event<void ()> e;
        e.attach(lambda);

        e();

        SUCCEED();
    }
    SECTION("return non-void")
    {
        auto lambda = []() { return 1; };
        event::event<int ()> e;
        e.attach(lambda);

        int value = e();

        REQUIRE(value == 1);
    }
}

TEST_CASE("perfect forwarding move-only arguments")
{
    event::event<int (std::unique_ptr<int>)> e;

    e.attach([](std::unique_ptr<int> ptr) {
        return *ptr;
    });

    auto ptr = std::make_unique<int>(3);
    int result = e(std::move(ptr));

    CHECK(result == 3);
    CHECK(ptr == nullptr);
}
