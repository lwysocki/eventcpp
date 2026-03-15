#include <benchmark/benchmark.h>
#include <eventcpp/event.hpp>

void dummy_callback(int value)
{
    benchmark::DoNotOptimize(value);
}

static void BM_EventDispatch(benchmark::State& state)
{
    event::event<void (int)> event;
    int num_listeners = state.range(0);

    for (int i = 0; i < num_listeners; ++i)
    {
        event.attach(dummy_callback);
    }

    for (auto _ : state)
    {
        event(42);
        benchmark::ClobberMemory();
    }

    state.SetComplexityN(num_listeners);
}

BENCHMARK(BM_EventDispatch)->Range(1, 1024)->Complexity();

BENCHMARK_MAIN();
