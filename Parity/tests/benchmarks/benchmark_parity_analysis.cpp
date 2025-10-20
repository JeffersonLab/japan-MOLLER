/**
 * @file benchmark_parity_analysis.cpp
 * @brief Benchmarks for Parity-specific analysis components
 * 
 * Performance benchmarks for helicity processing, asymmetry calculations,
 * blinding operations, and detector-specific processing.
 */

#ifdef ENABLE_BENCHMARKING

#include <benchmark/benchmark.h>
#include <vector>
#include <memory>
#include <random>

// Parity analysis headers
#include "QwHelicity.h"
#include "QwBlinder.h"
#include "QwBPMStripline.h"
#include "QwBCM.h"
#include "QwVQWK_Channel.h"
#include "QwSubsystemArrayParity.h"
#include "QwHelicityPattern.h"
#include "QwOptions.h"

//==============================================================================
// Helicity Processing Benchmarks
//==============================================================================

static void BM_HelicityRandomBitGeneration(benchmark::State& state) {
    QwHelicity helicity("BenchmarkHelicity");
    UInt_t seed = 12345;
    
    for (auto _ : state) {
        UInt_t bit = helicity.GetRandbit(seed);
        benchmark::DoNotOptimize(bit);
        benchmark::DoNotOptimize(seed);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HelicityRandomBitGeneration);

static void BM_HelicityPrediction(benchmark::State& state) {
    QwHelicity helicity("BenchmarkHelicity");
    helicity.SetHelicityDelay(8);
    helicity.SetMaxPatternPhase(4);
    
    for (auto _ : state) {
        helicity.PredictHelicity();
        helicity.RunPredictor();
        
        Int_t predicted = helicity.GetHelicityActual();
        benchmark::DoNotOptimize(predicted);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HelicityPrediction);

static void BM_HelicityEventProcessing(benchmark::State& state) {
    QwHelicity helicity("BenchmarkHelicity");
    const int num_events = state.range(0);
    
    std::vector<std::vector<UInt_t>> test_buffers;
    for (int i = 0; i < num_events; ++i) {
        std::vector<UInt_t> buffer = {
            static_cast<UInt_t>(0x1 << (i % 8)),  // Helicity bits
            static_cast<UInt_t>(i + 1000),        // Pattern number
            static_cast<UInt_t>(i % 4 + 1)        // Phase number
        };
        test_buffers.push_back(buffer);
    }
    
    for (auto _ : state) {
        for (int i = 0; i < num_events; ++i) {
            helicity.ClearEventData();
            helicity.ProcessEvBuffer(1, 1, test_buffers[i].data(), test_buffers[i].size());
            helicity.ProcessEvent();
        }
        
        benchmark::DoNotOptimize(helicity.GetEventNumber());
    }
    
    state.SetItemsProcessed(state.iterations() * num_events);
}
BENCHMARK(BM_HelicityEventProcessing)
    ->Arg(10)->Arg(100)->Arg(1000)
    ->Unit(benchmark::kMicrosecond);

//==============================================================================
// Blinding Operation Benchmarks
//==============================================================================

static void BM_AsymmetryBlinding(benchmark::State& state) {
    QwBlinder blinder;
    blinder.SetBlindingKey("benchmark_key_2023");
    blinder.SetBlindingOffset(100.0);
    blinder.SetBlindingFactor(0.5);
    blinder.SetBlindingState(true);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(-200.0, 200.0);
    
    for (auto _ : state) {
        double asymmetry = dis(gen);
        double blinded = blinder.BlindAsymmetry(asymmetry);
        benchmark::DoNotOptimize(blinded);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AsymmetryBlinding);

static void BM_AsymmetryUnblinding(benchmark::State& state) {
    QwBlinder blinder;
    blinder.SetBlindingKey("benchmark_key_2023");
    blinder.SetBlindingOffset(100.0);
    blinder.SetBlindingFactor(0.5);
    blinder.SetBlindingState(true);
    
    // Pre-generate blinded asymmetries
    std::vector<double> blinded_asymmetries;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(-200.0, 200.0);
    
    for (int i = 0; i < 1000; ++i) {
        double asymmetry = dis(gen);
        blinded_asymmetries.push_back(blinder.BlindAsymmetry(asymmetry));
    }
    
    int index = 0;
    for (auto _ : state) {
        double unblinded = blinder.UnblindAsymmetry(blinded_asymmetries[index % blinded_asymmetries.size()]);
        benchmark::DoNotOptimize(unblinded);
        index++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AsymmetryUnblinding);

static void BM_BlindingHashGeneration(benchmark::State& state) {
    QwBlinder blinder;
    const int string_length = state.range(0);
    
    std::string test_string(string_length, 'x');
    
    for (auto _ : state) {
        auto hash = blinder.GenerateHash(test_string);
        benchmark::DoNotOptimize(hash.size());
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * string_length);
}
BENCHMARK(BM_BlindingHashGeneration)
    ->Arg(10)->Arg(100)->Arg(1000)->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

static void BM_BulkAsymmetryBlinding(benchmark::State& state) {
    QwBlinder blinder;
    blinder.SetBlindingKey("bulk_benchmark_key");
    blinder.SetBlindingOffset(50.0);
    blinder.SetBlindingFactor(0.75);
    blinder.SetBlindingState(true);
    
    const int num_asymmetries = state.range(0);
    std::vector<double> asymmetries;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(-150.0, 150.0);
    
    for (int i = 0; i < num_asymmetries; ++i) {
        asymmetries.push_back(dis(gen));
    }
    
    for (auto _ : state) {
        for (double asym : asymmetries) {
            double blinded = blinder.BlindAsymmetry(asym);
            benchmark::DoNotOptimize(blinded);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_asymmetries);
}
BENCHMARK(BM_BulkAsymmetryBlinding)
    ->Arg(100)->Arg(1000)->Arg(10000)
    ->Unit(benchmark::kMicrosecond);

//==============================================================================
// BPM Processing Benchmarks
//==============================================================================

static void BM_BPMStriplineProcessing(benchmark::State& state) {
    using BPMType = QwBPMStripline<QwVQWK_Channel>;
    const int num_bpms = state.range(0);
    
    std::vector<BPMType> bpms;
    for (int i = 0; i < num_bpms; ++i) {
        bpms.emplace_back("bpm_" + std::to_string(i));
        bpms.back().SetPositionCalibration(1.0, 1.0);
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(900.0, 1100.0);
    
    for (auto _ : state) {
        for (auto& bpm : bpms) {
            // Set stripline signals
            bpm.GetSubelementByName("XP")->SetValue(dis(gen));
            bpm.GetSubelementByName("XM")->SetValue(dis(gen));
            bpm.GetSubelementByName("YP")->SetValue(dis(gen));
            bpm.GetSubelementByName("YM")->SetValue(dis(gen));
            
            bpm.ProcessEvent();
            
            benchmark::DoNotOptimize(bpm.GetRelativePositionX()->GetValue());
            benchmark::DoNotOptimize(bpm.GetRelativePositionY()->GetValue());
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_bpms);
}
BENCHMARK(BM_BPMStriplineProcessing)
    ->Arg(1)->Arg(10)->Arg(50)->Arg(100)
    ->Unit(benchmark::kMicrosecond);

static void BM_BPMPositionCalculation(benchmark::State& state) {
    using BPMType = QwBPMStripline<QwVQWK_Channel>;
    BPMType bpm("benchmark_bpm");
    bpm.SetPositionCalibration(2.5, 1.8);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(800.0, 1200.0);
    
    for (auto _ : state) {
        // Generate realistic stripline signals with small asymmetries
        double base_signal = 1000.0;
        double x_asymmetry = (dis(gen) - 1000.0) * 0.001;  // Small position offset
        double y_asymmetry = (dis(gen) - 1000.0) * 0.001;
        
        bpm.GetSubelementByName("XP")->SetValue(base_signal + x_asymmetry * base_signal);
        bpm.GetSubelementByName("XM")->SetValue(base_signal - x_asymmetry * base_signal);
        bpm.GetSubelementByName("YP")->SetValue(base_signal + y_asymmetry * base_signal);
        bpm.GetSubelementByName("YM")->SetValue(base_signal - y_asymmetry * base_signal);
        
        bpm.ProcessEvent();
        
        double x_pos = bpm.GetRelativePositionX()->GetValue();
        double y_pos = bpm.GetRelativePositionY()->GetValue();
        double charge = bpm.GetEffectiveCharge();
        
        benchmark::DoNotOptimize(x_pos);
        benchmark::DoNotOptimize(y_pos);
        benchmark::DoNotOptimize(charge);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_BPMPositionCalculation);

static void BM_BPMArithmeticOperations(benchmark::State& state) {
    using BPMType = QwBPMStripline<QwVQWK_Channel>;
    const int num_operations = state.range(0);
    
    std::vector<BPMType> bpm_a, bpm_b, results;
    for (int i = 0; i < num_operations; ++i) {
        bpm_a.emplace_back("bpm_a_" + std::to_string(i));
        bpm_b.emplace_back("bpm_b_" + std::to_string(i));
        results.emplace_back("result_" + std::to_string(i));
        
        // Set some values
        bpm_a.back().GetSubelementByName("XP")->SetValue(1000.0 + i);
        bpm_a.back().GetSubelementByName("XM")->SetValue(950.0 + i);
        bpm_b.back().GetSubelementByName("XP")->SetValue(1050.0 + i * 0.5);
        bpm_b.back().GetSubelementByName("XM")->SetValue(1000.0 + i * 0.5);
    }
    
    for (auto _ : state) {
        for (int i = 0; i < num_operations; ++i) {
            results[i].Sum(bpm_a[i], bpm_b[i]);
            benchmark::DoNotOptimize(results[i].GetEffectiveCharge());
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_operations);
}
BENCHMARK(BM_BPMArithmeticOperations)
    ->Arg(10)->Arg(100)->Arg(1000)
    ->Unit(benchmark::kMicrosecond);

//==============================================================================
// BCM Processing Benchmarks
//==============================================================================

static void BM_BCMCurrentProcessing(benchmark::State& state) {
    using BCMType = QwBCM<QwVQWK_Channel>;
    const int num_bcms = state.range(0);
    
    std::vector<BCMType> bcms;
    for (int i = 0; i < num_bcms; ++i) {
        bcms.emplace_back("bcm_" + std::to_string(i));
        bcms.back().SetCalibration(0.001 + i * 0.0001);  // Convert to microamps
        bcms.back().SetPedestal(100.0 + i);
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(1000.0, 5000.0);
    
    for (auto _ : state) {
        for (auto& bcm : bcms) {
            bcm.SetRawValue(dis(gen));
            bcm.ProcessEvent();
            
            double current = bcm.GetValue();
            benchmark::DoNotOptimize(current);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * num_bcms);
}
BENCHMARK(BM_BCMCurrentProcessing)
    ->Arg(1)->Arg(10)->Arg(50)->Arg(100)
    ->Unit(benchmark::kMicrosecond);

static void BM_BCMStatisticalAccumulation(benchmark::State& state) {
    using BCMType = QwBCM<QwVQWK_Channel>;
    const int num_accumulations = state.range(0);
    
    BCMType accumulator("accumulator");
    std::vector<BCMType> sources;
    
    for (int i = 0; i < 100; ++i) {
        sources.emplace_back("source_" + std::to_string(i));
        sources.back().SetValue(100.0 + i * 0.5);  // microamps
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
BENCHMARK(BM_BCMStatisticalAccumulation)
    ->Arg(100)->Arg(1000)->Arg(5000)
    ->Unit(benchmark::kMicrosecond);

//==============================================================================
// Subsystem Array Benchmarks
//==============================================================================

class BenchmarkSubsystem : public VQwSubsystemParity {
public:
    BenchmarkSubsystem(const std::string& name) : VQwSubsystemParity(name), value_(0.0) {}
    
    VQwSubsystem* Clone() override { return new BenchmarkSubsystem(*this); }
    void ClearEventData() override { value_ = 0.0; }
    void ProcessEvent() override { /* Simple processing */ }
    
    Int_t ProcessEvBuffer(const ROCID_t, const BankID_t, UInt_t* buffer, UInt_t num_words) override {
        if (buffer && num_words > 0) {
            value_ = static_cast<double>(buffer[0]);
        }
        return num_words;
    }
    
    VQwSubsystem& operator+=(VQwSubsystem* source) override {
        BenchmarkSubsystem* other = dynamic_cast<BenchmarkSubsystem*>(source);
        if (other) value_ += other->value_;
        return *this;
    }
    
    VQwSubsystem& operator-=(VQwSubsystem* source) override {
        BenchmarkSubsystem* other = dynamic_cast<BenchmarkSubsystem*>(source);
        if (other) value_ -= other->value_;
        return *this;
    }
    
    void Sum(VQwSubsystem* v1, VQwSubsystem* v2) override {
        BenchmarkSubsystem* s1 = dynamic_cast<BenchmarkSubsystem*>(v1);
        BenchmarkSubsystem* s2 = dynamic_cast<BenchmarkSubsystem*>(v2);
        if (s1 && s2) value_ = s1->value_ + s2->value_;
    }
    
    void Difference(VQwSubsystem* v1, VQwSubsystem* v2) override {
        BenchmarkSubsystem* s1 = dynamic_cast<BenchmarkSubsystem*>(v1);
        BenchmarkSubsystem* s2 = dynamic_cast<BenchmarkSubsystem*>(v2);
        if (s1 && s2) value_ = s1->value_ - s2->value_;
    }
    
    void Scale(Double_t factor) override { value_ *= factor; }
    
    double GetValue() const { return value_; }
    void SetValue(double value) { value_ = value; }
    
private:
    double value_;
};

static void BM_SubsystemArrayOperations(benchmark::State& state) {
    const int num_subsystems = state.range(0);
    
    QwOptions options;
    QwSubsystemArrayParity array1(options);
    QwSubsystemArrayParity array2(options);
    
    for (int i = 0; i < num_subsystems; ++i) {
        array1.push_back(std::make_shared<BenchmarkSubsystem>("subsys1_" + std::to_string(i)));
        array2.push_back(std::make_shared<BenchmarkSubsystem>("subsys2_" + std::to_string(i)));
        
        auto sub1 = dynamic_cast<BenchmarkSubsystem*>(array1.at(i).get());
        auto sub2 = dynamic_cast<BenchmarkSubsystem*>(array2.at(i).get());
        if (sub1 && sub2) {
            sub1->SetValue(1000.0 + i);
            sub2->SetValue(500.0 + i * 0.5);
        }
    }
    
    for (auto _ : state) {
        array1 += array2;
        benchmark::DoNotOptimize(array1.size());
    }
    
    state.SetItemsProcessed(state.iterations() * num_subsystems);
}
BENCHMARK(BM_SubsystemArrayOperations)
    ->Arg(1)->Arg(10)->Arg(50)->Arg(100)
    ->Unit(benchmark::kMicrosecond);

static void BM_SubsystemArrayEventProcessing(benchmark::State& state) {
    const int num_subsystems = state.range(0);
    const int events_per_iteration = 100;
    
    QwOptions options;
    QwSubsystemArrayParity array(options);
    
    for (int i = 0; i < num_subsystems; ++i) {
        array.push_back(std::make_shared<BenchmarkSubsystem>("subsys_" + std::to_string(i)));
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<UInt_t> dis(1000, 4000);
    
    for (auto _ : state) {
        for (int event = 0; event < events_per_iteration; ++event) {
            // Create event data
            std::vector<UInt_t> event_data;
            for (int i = 0; i < num_subsystems; ++i) {
                event_data.push_back(dis(gen));
            }
            
            // Process through array
            array.ClearEventData();
            array.ProcessEvBuffer(1, 1, event_data.data(), event_data.size());
            array.ProcessEvent();
        }
        
        benchmark::DoNotOptimize(array.size());
    }
    
    state.SetItemsProcessed(state.iterations() * events_per_iteration * num_subsystems);
}
BENCHMARK(BM_SubsystemArrayEventProcessing)
    ->Arg(1)->Arg(10)->Arg(50)
    ->Unit(benchmark::kMicrosecond);

//==============================================================================
// Memory and Object Management Benchmarks
//==============================================================================

static void BM_ParitySubsystemCreation(benchmark::State& state) {
    const int num_subsystems = state.range(0);
    
    for (auto _ : state) {
        std::vector<std::unique_ptr<BenchmarkSubsystem>> subsystems;
        subsystems.reserve(num_subsystems);
        
        for (int i = 0; i < num_subsystems; ++i) {
            subsystems.push_back(std::make_unique<BenchmarkSubsystem>("bench_" + std::to_string(i)));
            subsystems.back()->SetValue(1000.0 + i);
        }
        
        benchmark::DoNotOptimize(subsystems.size());
    }
    
    state.SetItemsProcessed(state.iterations() * num_subsystems);
}
BENCHMARK(BM_ParitySubsystemCreation)
    ->Arg(10)->Arg(100)->Arg(1000)
    ->Unit(benchmark::kMicrosecond);

static void BM_SubsystemCloning(benchmark::State& state) {
    BenchmarkSubsystem original("original");
    original.SetValue(12345.67);
    
    for (auto _ : state) {
        VQwSubsystem* cloned = original.Clone();
        benchmark::DoNotOptimize(cloned);
        delete cloned;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SubsystemCloning);

//==============================================================================
// End-to-End Parity Analysis Benchmarks
//==============================================================================

static void BM_CompleteParityAnalysisWorkflow(benchmark::State& state) {
    const int events_per_iteration = state.range(0);
    
    // Setup components
    QwOptions options;
    QwHelicity helicity("BenchmarkHelicity");
    QwBlinder blinder;
    QwSubsystemArrayParity detector_array(options);
    
    helicity.SetHelicityDelay(8);
    blinder.SetBlindingKey("workflow_benchmark");
    blinder.SetBlindingState(true);
    
    detector_array.push_back(std::make_shared<BenchmarkSubsystem>("MainDetector"));
    detector_array.push_back(std::make_shared<BenchmarkSubsystem>("Monitor1"));
    detector_array.push_back(std::make_shared<BenchmarkSubsystem>("Monitor2"));
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<UInt_t> dis(1000, 4000);
    std::uniform_int_distribution<int> helicity_dis(0, 1);
    
    for (auto _ : state) {
        double total_asymmetry = 0.0;
        
        for (int event = 0; event < events_per_iteration; ++event) {
            // Generate helicity state
            int helicity_state = (helicity_dis(gen) == 0) ? -1 : 1;
            
            // Process helicity
            std::vector<UInt_t> helicity_data = {
                static_cast<UInt_t>(helicity_state == 1 ? 0x1 : 0x0),
                static_cast<UInt_t>(event + 1000),
                static_cast<UInt_t>((event % 4) + 1)
            };
            
            helicity.ClearEventData();
            helicity.ProcessEvBuffer(1, 1, helicity_data.data(), helicity_data.size());
            helicity.ProcessEvent();
            
            // Generate detector signals with small asymmetry
            std::vector<UInt_t> detector_data;
            for (size_t i = 0; i < detector_array.size(); ++i) {
                UInt_t base_signal = dis(gen);
                UInt_t asymmetric_signal = base_signal + helicity_state * 5;  // Small asymmetry
                detector_data.push_back(asymmetric_signal);
            }
            
            // Process detector data
            detector_array.ClearEventData();
            detector_array.ProcessEvBuffer(1, 1, detector_data.data(), detector_data.size());
            detector_array.ProcessEvent();
            
            // Calculate asymmetry (simplified)
            auto main_detector = dynamic_cast<BenchmarkSubsystem*>(detector_array.at(0).get());
            if (main_detector) {
                double raw_asymmetry = main_detector->GetValue() / 1000.0 - 1.0;  // Normalize
                double blinded_asymmetry = blinder.BlindAsymmetry(raw_asymmetry * 1e6);  // Convert to ppb
                total_asymmetry += blinded_asymmetry;
            }
        }
        
        benchmark::DoNotOptimize(total_asymmetry);
    }
    
    state.SetItemsProcessed(state.iterations() * events_per_iteration);
}
BENCHMARK(BM_CompleteParityAnalysisWorkflow)
    ->Arg(10)->Arg(100)->Arg(500)
    ->Unit(benchmark::kMicrosecond);

#endif // ENABLE_BENCHMARKING
