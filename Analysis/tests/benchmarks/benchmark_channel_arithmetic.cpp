/**
 * @file benchmark_channel_arithmetic.cpp
 * @brief Benchmarks for VQwHardwareChannel arithmetic operations
 * 
 * Performance benchmarks for critical path operations in the analysis framework.
 * These benchmarks help identify performance regressions and optimization opportunities.
 */

#ifdef ENABLE_BENCHMARKING

#include <benchmark/benchmark.h>
#include <vector>
#include <memory>
#include <random>

// Core Analysis headers
#include "QwVQWK_Channel.h"
#include "QwMollerADC_Channel.h"
#include "VQwHardwareChannel.h"

//==============================================================================
// Benchmark Fixtures and Utilities
//==============================================================================

class ChannelBenchmarkFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        // Setup test channels
        vqwk_ch1.SetElementName("bench_vqwk_1");
        vqwk_ch2.SetElementName("bench_vqwk_2");
        
        moller_ch1.SetElementName("bench_moller_1");
        moller_ch2.SetElementName("bench_moller_2");
        
        // Initialize with some values
        vqwk_ch1.SetValue(100.0);
        vqwk_ch2.SetValue(50.0);
        moller_ch1.SetValue(200.0);
        moller_ch2.SetValue(75.0);
        
        // Create arrays for bulk operations
        createChannelArrays(state.range(0));
    }
    
    void TearDown(const ::benchmark::State& state) override {
        // Cleanup
        vqwk_channels.clear();
        moller_channels.clear();
    }
    
protected:
    QwVQWK_Channel vqwk_ch1, vqwk_ch2, vqwk_result;
    QwMollerADC_Channel moller_ch1, moller_ch2, moller_result;
    
    std::vector<QwVQWK_Channel> vqwk_channels;
    std::vector<QwMollerADC_Channel> moller_channels;
    
    void createChannelArrays(int size) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dis(0.0, 1000.0);
        
        vqwk_channels.reserve(size);
        moller_channels.reserve(size);
        
        for (int i = 0; i < size; ++i) {
            QwVQWK_Channel vqwk_ch("vqwk_" + std::to_string(i));
            vqwk_ch.SetValue(dis(gen));
            vqwk_channels.push_back(std::move(vqwk_ch));
            
            QwMollerADC_Channel moller_ch("moller_" + std::to_string(i));
            moller_ch.SetValue(dis(gen));
            moller_channels.push_back(std::move(moller_ch));
        }
    }
};

//==============================================================================
// Basic Arithmetic Benchmarks
//==============================================================================

BENCHMARK_F(ChannelBenchmarkFixture, VQWKAddition)(benchmark::State& state) {
    for (auto _ : state) {
        vqwk_result = vqwk_ch1;
        vqwk_result += vqwk_ch2;
        benchmark::DoNotOptimize(vqwk_result);
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(ChannelBenchmarkFixture, VQWKSubtraction)(benchmark::State& state) {
    for (auto _ : state) {
        vqwk_result = vqwk_ch1;
        vqwk_result -= vqwk_ch2;
        benchmark::DoNotOptimize(vqwk_result);
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(ChannelBenchmarkFixture, VQWKSumMethod)(benchmark::State& state) {
    for (auto _ : state) {
        vqwk_result.Sum(vqwk_ch1, vqwk_ch2);
        benchmark::DoNotOptimize(vqwk_result);
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(ChannelBenchmarkFixture, MollerAddition)(benchmark::State& state) {
    for (auto _ : state) {
        moller_result = moller_ch1;
        moller_result += moller_ch2;
        benchmark::DoNotOptimize(moller_result);
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(ChannelBenchmarkFixture, MollerSumMethod)(benchmark::State& state) {
    for (auto _ : state) {
        moller_result.Sum(moller_ch1, moller_ch2);
        benchmark::DoNotOptimize(moller_result);
    }
    state.SetItemsProcessed(state.iterations());
}

//==============================================================================
// Clone Operation Benchmarks
//==============================================================================

BENCHMARK_F(ChannelBenchmarkFixture, VQWKClone)(benchmark::State& state) {
    for (auto _ : state) {
        VQwDataElement* cloned = vqwk_ch1.Clone();
        benchmark::DoNotOptimize(cloned);
        delete cloned;
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(ChannelBenchmarkFixture, MollerClone)(benchmark::State& state) {
    for (auto _ : state) {
        VQwDataElement* cloned = moller_ch1.Clone();
        benchmark::DoNotOptimize(cloned);
        delete cloned;
    }
    state.SetItemsProcessed(state.iterations());
}

//==============================================================================
// Bulk Operations Benchmarks
//==============================================================================

BENCHMARK_F(ChannelBenchmarkFixture, VQWKBulkAddition)(benchmark::State& state) {
    const int size = state.range(0);
    for (auto _ : state) {
        for (int i = 1; i < size; ++i) {
            vqwk_channels[0] += vqwk_channels[i];
        }
        benchmark::DoNotOptimize(vqwk_channels[0]);
    }
    state.SetItemsProcessed(state.iterations() * (size - 1));
}
BENCHMARK_F(ChannelBenchmarkFixture, VQWKBulkAddition)->Arg(10)->Arg(100)->Arg(1000);

BENCHMARK_F(ChannelBenchmarkFixture, MollerBulkAddition)(benchmark::State& state) {
    const int size = state.range(0);
    for (auto _ : state) {
        for (int i = 1; i < size; ++i) {
            moller_channels[0] += moller_channels[i];
        }
        benchmark::DoNotOptimize(moller_channels[0]);
    }
    state.SetItemsProcessed(state.iterations() * (size - 1));
}
BENCHMARK_F(ChannelBenchmarkFixture, MollerBulkAddition)->Arg(10)->Arg(100)->Arg(1000);

//==============================================================================
// Polymorphic Operation Benchmarks
//==============================================================================

static void BM_PolymorphicClone(benchmark::State& state) {
    QwVQWK_Channel vqwk_ch("poly_test");
    vqwk_ch.SetValue(123.456);
    VQwDataElement* base_ptr = &vqwk_ch;
    
    for (auto _ : state) {
        VQwDataElement* cloned = base_ptr->Clone();
        benchmark::DoNotOptimize(cloned);
        delete cloned;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PolymorphicClone);

static void BM_PolymorphicArithmetic(benchmark::State& state) {
    QwVQWK_Channel vqwk_ch1("poly_test_1");
    QwVQWK_Channel vqwk_ch2("poly_test_2");
    vqwk_ch1.SetValue(100.0);
    vqwk_ch2.SetValue(50.0);
    
    VQwDataElement* base1 = &vqwk_ch1;
    VQwDataElement* base2 = &vqwk_ch2;
    
    for (auto _ : state) {
        *base1 += *base2;
        benchmark::DoNotOptimize(base1);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_PolymorphicArithmetic);

//==============================================================================
// Memory Allocation Benchmarks
//==============================================================================

static void BM_VQWKConstruction(benchmark::State& state) {
    for (auto _ : state) {
        QwVQWK_Channel ch("benchmark_channel");
        ch.SetValue(42.0);
        benchmark::DoNotOptimize(ch);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VQWKConstruction);

static void BM_MollerConstruction(benchmark::State& state) {
    for (auto _ : state) {
        QwMollerADC_Channel ch("benchmark_channel");
        ch.SetValue(42.0);
        benchmark::DoNotOptimize(ch);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_MollerConstruction);

static void BM_VQWKHeapAllocation(benchmark::State& state) {
    for (auto _ : state) {
        auto ch = std::make_unique<QwVQWK_Channel>("heap_channel");
        ch->SetValue(42.0);
        benchmark::DoNotOptimize(ch);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_VQWKHeapAllocation);

//==============================================================================
// Event Processing Simulation
//==============================================================================

static void BM_EventProcessingSimulation(benchmark::State& state) {
    const int num_channels = state.range(0);
    std::vector<QwVQWK_Channel> channels;
    channels.reserve(num_channels);
    
    // Setup channels
    for (int i = 0; i < num_channels; ++i) {
        channels.emplace_back("event_ch_" + std::to_string(i));
        channels.back().SetValue(100.0 + i);
    }
    
    QwVQWK_Channel accumulator("accumulator");
    
    for (auto _ : state) {
        accumulator.ClearEventData();
        
        // Simulate event processing: accumulate all channels
        for (const auto& ch : channels) {
            accumulator += ch;
        }
        
        benchmark::DoNotOptimize(accumulator);
    }
    
    state.SetItemsProcessed(state.iterations() * num_channels);
    state.SetBytesProcessed(state.iterations() * num_channels * sizeof(QwVQWK_Channel));
}
BENCHMARK(BM_EventProcessingSimulation)
    ->Arg(10)
    ->Arg(50) 
    ->Arg(100)
    ->Arg(500)
    ->Unit(benchmark::kMicrosecond);

#endif // ENABLE_BENCHMARKING