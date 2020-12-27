# C++ Events

Cpp_events is a header-only library that implement C#-like events for C++.

## Installation

Copy header file to folder of your choice and include it in your project.

## Usage

Following example shows that single Event instance can handle both functions and non-static methods.

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

    Event::Event<int (int)> e;

    e.Attach(Mul2); // subscribe function
    e.Attach(Add2);
    e.Attach<Abs>(&Abs::Sub, a); // subscribe method Sub on object a
    e.Attach<B>(&B::Sub, b);
    e.Attach(&Abs::Sub, b1);
    e.Attach(&A::Sub, a2);
    e.Attach(&B::Sub, b2);
    e.Attach(&B::Div, b2);

    int result = e(2); // invoke event

    e.Detach(&Abs::Sub, b2); // remove subscriber by base class member, b2.Div still receive message

    return 0;
}
```

## Authors

- Lukasz Wysocki

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
