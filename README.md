# C++ Events

eventcpp is a library that implement C#-like events for C++.

## Requirements

This library require C++17 standard to be enabled.

## Installation

This is a header-only library. Copy header file to folder of your choice and
include it in your project.

## Usage

Following example shows that single event instance can handle functions, static
member functions, and non-static member functions.

```C++
#include <eventcpp/event.hpp>

int Mul2(int x)
{
    return x * 2;
}

int Add2(int x)
{
    return x + 2;
}

class Abs
{
public:
    virtual int Sub(int x) = 0;
};

class A : public Abs
{
public:
    int Sub(int x) override
    {
        return x - 2;
    }
};

class B : public Abs
{
public:
    int Sub(int x) override
    {
        return x - 3;
    }

    int Div(int x)
    {
        return x / 2;
    }
};

int main()
{
    A* a = new A();
    B* b = new B();
    Abs* a1 = new A();
    Abs* b1 = new B();
    A a2;
    B* b2 = new B();

    event::event<int (int)> e;

    e.attach(Mul2); // subscribe function
    e.attach(Add2);
    e.attach<Abs>(&Abs::Sub, a); // subscribe method Sub on object a
    e.attach<B>(&B::Sub, b);
    e.attach(&Abs::Sub, b1);
    e.attach(&A::Sub, a2);
    e.attach(&B::Sub, b2);
    e.attach(&B::Div, b2);

    int result = e(2); // invoke event

    e.detach(&Abs::Sub, b2); // remove subscriber by base class member, b2.Div still receive message

    return 0;
}
```

## Authors

- Lukasz Wysocki

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
