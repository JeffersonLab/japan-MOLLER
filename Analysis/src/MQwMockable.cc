/*!
 * \file   MQwMockable.cc
 * \brief  Mix-in class implementation for mock data generation
 */

#include "MQwMockable.h"
#include "QwParameterFile.h"

// Randomness generator: Mersenne twister with period 2^19937 - 1
//
// This is defined as static to avoid getting stuck with 100% correlated
// ADC channels when each channel goes through the same list of pseudo-
// random numbers...
std::mt19937 MQwMockable::fRandomnessGenerator;
std::normal_distribution<double> MQwMockable::fNormalDistribution;
std::function<double()> MQwMockable::fNormalRandomVariable = []() -> double { return fNormalDistribution(fRandomnessGenerator); };


void MQwMockable::LoadMockDataParameters(QwParameterFile &paramfile){
  Bool_t   ldebug=kFALSE;
  Double_t asym=0.0, mean=0.0, sigma=0.0;
  Double_t amplitude=0.0, phase=0.0, frequency=0.0;

  //Check to see if this line contains "drift"
  if (paramfile.GetLine().find("drift")!=std::string::npos){
    //  "drift" appears somewhere in the line.  Assume it is the next token and move on...
    paramfile.GetNextToken();  //Throw away this token.  Now the next three should be the drift parameters.
    //read 3 parameters
    amplitude = paramfile.GetTypedNextToken<Double_t>();
    phase     = paramfile.GetTypedNextToken<Double_t>();
    frequency = paramfile.GetTypedNextToken<Double_t>();  //  The frequency we read in should be in Hz.
    this->AddRandomEventDriftParameters(amplitude, phase, frequency*Qw::Hz);
    // std::cout << "In MQwMockable::LoadMockDataParameters: amp = " << amplitude << "\t phase = " << phase << "\t freq = " << frequency << std::endl;
  } 
  else {
    asym  = paramfile.GetTypedNextToken<Double_t>();
    mean  = paramfile.GetTypedNextToken<Double_t>(); 
    sigma = paramfile.GetTypedNextToken<Double_t>();      
    if (ldebug==1) {
      std::cout << "#################### \n";
      std::cout << "asym, mean, sigma \n" << std::endl;
      std::cout << asym                   << " / "
  	        << mean                   << " / "
	        << sigma                  << " / "
                << std::endl;  
    }
    this->SetRandomEventParameters(mean, sigma);
    this->SetRandomEventAsymmetry(asym);
  }
}


void MQwMockable::SetRandomEventDriftParameters(Double_t amplitude, Double_t phase, Double_t frequency)
{
  // Clear existing values
  fMockDriftAmplitude.clear();
  fMockDriftFrequency.clear();
  fMockDriftPhase.clear();
  // Add new values
  fMockDriftAmplitude.push_back(amplitude);
  fMockDriftFrequency.push_back(frequency);
  fMockDriftPhase.push_back(phase);
  return;
}

void MQwMockable::AddRandomEventDriftParameters(Double_t amplitude, Double_t phase, Double_t frequency)
{
  // Add new values
  fMockDriftAmplitude.push_back(amplitude);
  fMockDriftFrequency.push_back(frequency);
  fMockDriftPhase.push_back(phase);
  return;
}

void MQwMockable::SetRandomEventParameters(Double_t mean, Double_t sigma)
{
  fMockGaussianMean = mean;
  fMockGaussianSigma = sigma;
  return;
}

void MQwMockable::SetRandomEventAsymmetry(Double_t asymmetry)
{
  fMockAsymmetry = asymmetry;
  return;
}

Double_t MQwMockable::GetRandomValue(){
  Double_t random_variable;
  if (fUseExternalRandomVariable)
    // external normal random variable
    random_variable = fExternalRandomVariable;
  else
    // internal normal random variable
    random_variable = fNormalRandomVariable();
  return random_variable;
}

