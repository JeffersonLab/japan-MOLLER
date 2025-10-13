/**
 * @file benchmark_event_processing.cpp
 * @brief Comprehensive benchmarks for event processing components
 * 
 * Performance benchmarks for data ingestion, event processing, and ROOT file writing.
 * These benchmarks help identify performance bottlenecks and track performance regressions.
 */

#ifdef ENABLE_BENCHMARKING

#include <benchmark/benchmark.h>
#include <vector>
#include <memory>
#include <random>
#include <fstream>

// Core framework headers
#include "QwEventBuffer.h"
#include "QwSubsystemArray.h"
#include "QwVQWK_Channel.h"
#include "QwMollerADC_Channel.h"
#include "QwScaler_Channel.h"
#include "QwRootFile.h"
#include "QwOptions.h"

// ROOT headers for file I/O benchmarks
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"

//==============================================================================
// Benchmark Fixtures and Utilities
//==============================================================================

class EventProcessingBenchmark : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        // Setup event buffer
        event_buffer_ = std::make_unique<QwEventBuffer>();
        event_buffer_->ProcessOptions(options_);
        
        // Setup subsystem array
        subsystem_array_ = std::make_unique<QwSubsystemArray>(options_);
        
        // Create test events
        createTestEvents(state.range(0));
        
        // Initialize random number generator
        rng_.seed(12345);
    }
    
    void TearDown(const ::benchmark::State& state) override {
        test_events_.clear();
        event_buffer_.reset();
        subsystem_array_.reset();
    }
    
protected:
    std::unique_ptr<QwEventBuffer> event_buffer_;
    std::unique_ptr<QwSubsystemArray> subsystem_array_;
    std::vector<std::vector<UInt_t>> test_events_;
    QwOptions options_;
    std::mt19937 rng_;
    
    void createTestEvents(int num_events) {
        test_events_.reserve(num_events);
        
        for (int i = 0; i < num_events; ++i) {
            std::vector<UInt_t> event;
            
            // CODA event header
            event.push_back(20);  // Event length
            event.push_back(i + 1);  // Event number
            event.push_back(1);   // Event type
            event.push_back(0x12345678);  // Timestamp
            
            // Add detector data (16 words)
            for (int j = 0; j < 16; ++j) {
                event.push_back(1000 + (i * 16 + j) % 4096);
            }
            
            test_events_.push_back(std::move(event));
        }
    }
    
    std::vector<UInt_t> generateRandomEvent(int event_number) {
        std::uniform_int_distribution<UInt_t> dist(500, 4095);
        
        std::vector<UInt_t> event;
        event.push_back(20);  // Event length
        event.push_back(event_number);
        event.push_back(1);
        event.push_back(0x12345678);
        
        for (int i = 0; i < 16; ++i) {
            event.push_back(dist(rng_));
        }
        
        return event;
    }
};

//==============================================================================
// Event Buffer Processing Benchmarks (Data Ingestion)
//==============================================================================

BENCHMARK_F(EventProcessingBenchmark, EventBufferLoading)(benchmark::State& state) {
    int event_index = 0;
    const int num_events = state.range(0);
    
    for (auto _ : state) {
        const auto& event = test_events_[event_index % num_events];
        event_buffer_->LoadEvent(event.data(), event.size());
        
        benchmark::DoNotOptimize(event_buffer_->GetEventNumber());
        event_index++;
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * 20 * sizeof(UInt_t));
}
BENCHMARK_F(EventProcessingBenchmark, EventBufferLoading)
    ->Arg(100)->Arg(1000)->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_F(EventProcessingBenchmark, EventBufferValidation)(benchmark::State& state) {
    int event_index = 0;
    const int num_events = state.range(0);
    
    for (auto _ : state) {
        const auto& event = test_events_[event_index % num_events];
        event_buffer_->LoadEvent(event.data(), event.size());
        
        // Perform validation checks
        bool is_good = event_buffer_->IsGoodEvent();
        bool is_physics = event_buffer_->IsPhysicsEvent();
        bool in_range = event_buffer_->IsEventInRange();
        
        benchmark::DoNotOptimize(is_good);
        benchmark::DoNotOptimize(is_physics);
        benchmark::DoNotOptimize(in_range);
        
        event_index++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_F(EventProcessingBenchmark, EventBufferValidation)
    ->Arg(100)->Arg(1000)->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_F(EventProcessingBenchmark, BulkEventLoading)(benchmark::State& state) {
    const int events_per_iteration = state.range(0);
    
    for (auto _ : state) {
        for (int i = 0; i < events_per_iteration; ++i) {
            const auto& event = test_events_[i % test_events_.size()];
            event_buffer_->LoadEvent(event.data(), event.size());
        }
        benchmark::DoNotOptimize(event_buffer_->GetEventCount());
    }
    
    state.SetItemsProcessed(state.iterations() * events_per_iteration);
    state.SetBytesProcessed(state.iterations() * events_per_iteration * 20 * sizeof(UInt_t));
}
BENCHMARK_F(EventProcessingBenchmark, BulkEventLoading)
    ->Arg(10)->Arg(100)->Arg(1000)
    ->Unit(benchmark::kMicrosecond);

//==============================================================================
// Channel Processing Benchmarks
//==============================================================================

static void BM_VQWKChannelProcessing(benchmark::State& state) {
    const int num_channels = state.range(0);
    std::vector<QwVQWK_Channel> channels;
    
    // Create channels
    for (int i = 0; i < num_channels; ++i) {
        channels.emplace_back("vqwk_" + std::to_string(i));
        channels.back().SetCalibration(1.0 + i * 0.001);
        channels.back().SetPedestal(100.0 + i);
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(1000.0, 4000.0);
    
    for (auto _ : state) {
        for (auto& channel : channels) {
            channel.SetRawValue(dis(gen));
            channel.ProcessEvent();
            benchmark::DoNotOptimize(channel.GetValue());
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_channels);
}
BENCHMARK(BM_VQWKChannelProcessing)
    ->Arg(10)->Arg(100)->Arg(1000)->Arg(5000)
    ->Unit(benchmark::kMicrosecond);

static void BM_MollerADCChannelProcessing(benchmark::State& state) {
    const int num_channels = state.range(0);
    std::vector<QwMollerADC_Channel> channels;
    
    for (int i = 0; i < num_channels; ++i) {
        channels.emplace_back("moller_" + std::to_string(i));
        channels.back().SetCalibration(0.001 + i * 0.0001);
        channels.back().SetPedestal(200.0 + i * 2);
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(500.0, 3000.0);
    
    for (auto _ : state) {
        for (auto& channel : channels) {
            channel.SetRawValue(dis(gen));
            channel.ProcessEvent();
            benchmark::DoNotOptimize(channel.GetValue());
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_channels);
}
BENCHMARK(BM_MollerADCChannelProcessing)
    ->Arg(10)->Arg(100)->Arg(1000)->Arg(5000)
    ->Unit(benchmark::kMicrosecond);

static void BM_ScalerChannelProcessing(benchmark::State& state) {
    const int num_channels = state.range(0);
    std::vector<QwScaler_Channel> channels;
    
    for (int i = 0; i < num_channels; ++i) {
        channels.emplace_back("scaler_" + std::to_string(i));
        channels.back().SetIntegrationTime(1.0);
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<UInt_t> dis(10000, 1000000);
    
    for (auto _ : state) {
        for (auto& channel : channels) {
            channel.SetValue(dis(gen));
            double rate = channel.GetRate();
            benchmark::DoNotOptimize(rate);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_channels);
}
BENCHMARK(BM_ScalerChannelProcessing)
    ->Arg(10)->Arg(100)->Arg(1000)->Arg(5000)
    ->Unit(benchmark::kMicrosecond);

//==============================================================================
// Arithmetic Operations Benchmarks
//==============================================================================

static void BM_ChannelArithmeticOperations(benchmark::State& state) {
    const int num_operations = state.range(0);
    
    std::vector<QwVQWK_Channel> channels_a, channels_b, results;
    
    for (int i = 0; i < num_operations; ++i) {
        channels_a.emplace_back("a_" + std::to_string(i));
        channels_b.emplace_back("b_" + std::to_string(i));
        results.emplace_back("result_" + std::to_string(i));
        
        channels_a.back().SetValue(1000.0 + i);
        channels_b.back().SetValue(500.0 + i * 0.5);
    }
    
    for (auto _ : state) {
        for (int i = 0; i < num_operations; ++i) {
            results[i].Sum(channels_a[i], channels_b[i]);
            benchmark::DoNotOptimize(results[i].GetValue());
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_operations);
}
BENCHMARK(BM_ChannelArithmeticOperations)
    ->Arg(100)->Arg(1000)->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

static void BM_PolymorphicArithmeticOperations(benchmark::State& state) {
    const int num_operations = state.range(0);
    
    std::vector<std::unique_ptr<VQwDataElement>> elements_a, elements_b;
    
    for (int i = 0; i < num_operations; ++i) {
        elements_a.push_back(std::make_unique<QwVQWK_Channel>("a_" + std::to_string(i)));
        elements_b.push_back(std::make_unique<QwVQWK_Channel>("b_" + std::to_string(i)));
        
        elements_a.back()->SetValue(1000.0 + i);
        elements_b.back()->SetValue(500.0 + i * 0.5);
    }
    
    for (auto _ : state) {
        for (int i = 0; i < num_operations; ++i) {
            *elements_a[i] += *elements_b[i];
            benchmark::DoNotOptimize(elements_a[i]->GetValue());
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_operations);
}
BENCHMARK(BM_PolymorphicArithmeticOperations)
    ->Arg(100)->Arg(1000)->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

//==============================================================================
// ROOT File I/O Benchmarks
//==============================================================================

static void BM_ROOTFileWriting(benchmark::State& state) {
    const int num_entries = state.range(0);
    
    // Data to write
    struct EventData {
        Double_t asymmetry;
        Double_t charge;
        Int_t event_number;
        Int_t helicity;
    };
    
    for (auto _ : state) {
        // Create temporary file
        std::string filename = "/tmp/benchmark_" + std::to_string(state.iterations()) + ".root";
        
        auto start = std::chrono::high_resolution_clock::now();
        
        TFile file(filename.c_str(), "RECREATE");
        TTree tree("events", "Benchmark Event Tree");
        
        EventData data;
        tree.Branch("asymmetry", &data.asymmetry, "asymmetry/D");
        tree.Branch("charge", &data.charge, "charge/D");
        tree.Branch("event_number", &data.event_number, "event_number/I");
        tree.Branch("helicity", &data.helicity, "helicity/I");
        
        // Fill tree
        for (int i = 0; i < num_entries; ++i) {
            data.asymmetry = 100.0 + i * 0.01;
            data.charge = 1000.0 + i;
            data.event_number = i + 1;
            data.helicity = (i % 2 == 0) ? 1 : -1;
            
            tree.Fill();
        }
        
        tree.Write();
        file.Close();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Clean up
        std::remove(filename.c_str());
        
        benchmark::DoNotOptimize(duration.count());
    }
    
    state.SetItemsProcessed(state.iterations() * num_entries);
    state.SetBytesProcessed(state.iterations() * num_entries * sizeof(EventData));
}
BENCHMARK(BM_ROOTFileWriting)
    ->Arg(100)->Arg(1000)->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

static void BM_ROOTFileReading(benchmark::State& state) {
    const int num_entries = state.range(0);
    
    // Create test file
    std::string filename = "/tmp/benchmark_read_test.root";
    {
        TFile file(filename.c_str(), "RECREATE");
        TTree tree("events", "Benchmark Event Tree");
        
        Double_t asymmetry, charge;
        Int_t event_number, helicity;
        
        tree.Branch("asymmetry", &asymmetry, "asymmetry/D");
        tree.Branch("charge", &charge, "charge/D");
        tree.Branch("event_number", &event_number, "event_number/I");
        tree.Branch("helicity", &helicity, "helicity/I");
        
        for (int i = 0; i < num_entries; ++i) {
            asymmetry = 100.0 + i * 0.01;
            charge = 1000.0 + i;
            event_number = i + 1;
            helicity = (i % 2 == 0) ? 1 : -1;
            
            tree.Fill();
        }
        
        tree.Write();
        file.Close();
    }
    
    for (auto _ : state) {
        TFile file(filename.c_str(), "READ");
        TTree* tree = dynamic_cast<TTree*>(file.Get("events"));
        
        if (tree) {
            Double_t asymmetry, charge;
            Int_t event_number, helicity;
            
            tree->SetBranchAddress("asymmetry", &asymmetry);
            tree->SetBranchAddress("charge", &charge);
            tree->SetBranchAddress("event_number", &event_number);
            tree->SetBranchAddress("helicity", &helicity);
            
            Long64_t entries = tree->GetEntries();
            for (Long64_t i = 0; i < entries; ++i) {
                tree->GetEntry(i);
                benchmark::DoNotOptimize(asymmetry);
                benchmark::DoNotOptimize(charge);
            }
        }
        
        file.Close();
    }
    
    // Clean up
    std::remove(filename.c_str());
    
    state.SetItemsProcessed(state.iterations() * num_entries);
}
BENCHMARK(BM_ROOTFileReading)
    ->Arg(100)->Arg(1000)->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

//==============================================================================
// Memory Allocation and Management Benchmarks
//==============================================================================

static void BM_ChannelCreationDestruction(benchmark::State& state) {
    const int num_channels = state.range(0);
    
    for (auto _ : state) {
        std::vector<QwVQWK_Channel> channels;
        channels.reserve(num_channels);
        
        for (int i = 0; i < num_channels; ++i) {
            channels.emplace_back("channel_" + std::to_string(i));
            channels.back().SetValue(1000.0 + i);
        }
        
        benchmark::DoNotOptimize(channels.size());
    }
    
    state.SetItemsProcessed(state.iterations() * num_channels);
}
BENCHMARK(BM_ChannelCreationDestruction)
    ->Arg(100)->Arg(1000)->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

static void BM_DynamicChannelAllocation(benchmark::State& state) {
    const int num_channels = state.range(0);
    
    for (auto _ : state) {
        std::vector<std::unique_ptr<QwVQWK_Channel>> channels;
        channels.reserve(num_channels);
        
        for (int i = 0; i < num_channels; ++i) {
            channels.push_back(std::make_unique<QwVQWK_Channel>("channel_" + std::to_string(i)));
            channels.back()->SetValue(1000.0 + i);
        }
        
        benchmark::DoNotOptimize(channels.size());
    }
    
    state.SetItemsProcessed(state.iterations() * num_channels);
}
BENCHMARK(BM_DynamicChannelAllocation)
    ->Arg(100)->Arg(1000)->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

//==============================================================================
// End-to-End Workflow Benchmarks
//==============================================================================

static void BM_CompleteEventProcessingWorkflow(benchmark::State& state) {
    const int events_per_iteration = state.range(0);
    
    QwOptions options;
    auto event_buffer = std::make_unique<QwEventBuffer>();
    event_buffer->ProcessOptions(options);
    
    auto subsystem_array = std::make_unique<QwSubsystemArray>(options);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<UInt_t> dis(1000, 4000);
    
    for (auto _ : state) {
        for (int i = 0; i < events_per_iteration; ++i) {
            // Create event
            std::vector<UInt_t> event_data;
            event_data.push_back(20);  // Length
            event_data.push_back(i + 1);  // Event number
            event_data.push_back(1);   // Type
            event_data.push_back(0x12345678);  // Timestamp
            
            for (int j = 0; j < 16; ++j) {
                event_data.push_back(dis(gen));
            }
            
            // Load event
            event_buffer->LoadEvent(event_data.data(), event_data.size());
            
            // Process through subsystems
            subsystem_array->ClearEventData();
            subsystem_array->ProcessEvBuffer(1, 1, event_data.data() + 4, 16);
            subsystem_array->ProcessEvent();
        }
        
        benchmark::DoNotOptimize(subsystem_array->size());
    }
    
    state.SetItemsProcessed(state.iterations() * events_per_iteration);
    state.SetBytesProcessed(state.iterations() * events_per_iteration * 20 * sizeof(UInt_t));
}
BENCHMARK(BM_CompleteEventProcessingWorkflow)
    ->Arg(10)->Arg(100)->Arg(1000)
    ->Unit(benchmark::kMicrosecond);

//==============================================================================
// Specialized Performance Tests
//==============================================================================

static void BM_ErrorHandlingOverhead(benchmark::State& state) {
    std::vector<QwVQWK_Channel> channels;
    const int num_channels = state.range(0);
    
    for (int i = 0; i < num_channels; ++i) {
        channels.emplace_back("error_test_" + std::to_string(i));
        // Set error flags on some channels
        if (i % 10 == 0) {
            channels.back().SetEventcutErrorFlag(0x01);
        }
    }
    
    for (auto _ : state) {
        for (auto& channel : channels) {
            bool is_good = channel.IsGoodEvent();
            UInt_t error_flag = channel.GetEventcutErrorFlag();
            benchmark::DoNotOptimize(is_good);
            benchmark::DoNotOptimize(error_flag);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_channels);
}
BENCHMARK(BM_ErrorHandlingOverhead)
    ->Arg(100)->Arg(1000)->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

static void BM_StatisticalAccumulation(benchmark::State& state) {
    const int num_accumulations = state.range(0);
    
    QwVQWK_Channel accumulator("accumulator");
    std::vector<QwVQWK_Channel> sources;
    
    for (int i = 0; i < 100; ++i) {
        sources.emplace_back("source_" + std::to_string(i));
        sources.back().SetValue(100.0 + i * 0.1);
    }
    
    for (auto _ : state) {
        accumulator.ClearEventData();
        
        for (int i = 0; i < num_accumulations; ++i) {
            const auto& source = sources[i % sources.size()];
            accumulator.AccumulateRunningSum(source, 1);
        }
        
        benchmark::DoNotOptimize(accumulator.GetValue());
    }
    
    state.SetItemsProcessed(state.iterations() * num_accumulations);
}
BENCHMARK(BM_StatisticalAccumulation)
    ->Arg(100)->Arg(1000)->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

#endif // ENABLE_BENCHMARKING
