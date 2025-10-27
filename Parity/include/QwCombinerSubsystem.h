/*!
 * \file   QwCombinerSubsystem.h
 * \brief  Combiner subsystem for parity analysis data handling
 * \author meeker
 * \date   2011-08-11
 */

#pragma once

// headers
#include "VQwSubsystemParity.h"
#include "QwSubsystemArrayParity.h"
#include "QwCombiner.h"

class QwParameterFile;

/**
 * \class QwCombinerSubsystem
 *
 * \brief
 *
 */

class QwCombinerSubsystem: public VQwSubsystemParity,
  public MQwSubsystemCloneable<QwCombinerSubsystem>,
  public QwCombiner
{

  public:
      // Constructors
      /// \brief Constructor with just name.
      QwCombinerSubsystem(const TString name)
      : VQwSubsystem(name), VQwSubsystemParity(name), QwCombiner(name) { }

      // Copy Constructor
      QwCombinerSubsystem(const QwCombinerSubsystem &source)
      : VQwSubsystem(source), VQwSubsystemParity(source), QwCombiner(source) { }

      // Destructor
      ~QwCombinerSubsystem() override;

      std::shared_ptr<VQwSubsystem> GetSharedPointerToStaticObject();

      /// \brief Update the running sums
      void AccumulateRunningSum(VQwSubsystem* input, Int_t count=0, Int_t ErrorMask=0xFFFFFFF) override;
      void DeaccumulateRunningSum(VQwSubsystem* value, Int_t ErrorMask=0xFFFFFFF) override;
      /// \brief Calculate the average for all good events
      void CalculateRunningAverage() override;
      /// \brief Print values for all channels
      void PrintValue() const override;

      void ProcessData() override{
        QwCombiner::ProcessData();
      }

      /// \brief Overloaded Operators
      VQwSubsystem& operator=  (VQwSubsystem *value) override;
      VQwSubsystem& operator+= (VQwSubsystem *value) override;
      VQwSubsystem& operator-= (VQwSubsystem *value) override;
      VQwSubsystem& operator*= (VQwSubsystem *value);
      VQwSubsystem& operator/= (VQwSubsystem *value);
      void Ratio(VQwSubsystem* value1, VQwSubsystem* value2) override;
      void Scale(Double_t value) override;

      void ConstructBranchAndVector(TTree *tree, TString& prefix, QwRootTreeBranchVector &values) override{
        QwCombiner::ConstructBranchAndVector(tree,prefix,values);
      }
      void FillTreeVector(QwRootTreeBranchVector &values) const override{
        QwCombiner::FillTreeVector(values);
      }
#ifdef HAS_RNTUPLE_SUPPORT
      void ConstructNTupleAndVector(std::unique_ptr<ROOT::RNTupleModel>& model, TString& prefix, std::vector<Double_t>& values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs) override{
        QwCombiner::ConstructNTupleAndVector(model, prefix, values, fieldPtrs);
      }
      void FillNTupleVector(std::vector<Double_t>& values) const override{
        QwCombiner::FillNTupleVector(values);
      }
#endif // HAS_RNTUPLE_SUPPORT

      void ConstructHistograms(TDirectory *folder, TString &prefix) override;
      void FillHistograms() override;
      void DeleteHistograms();
      void ConstructBranch(TTree *tree, TString & prefix) override;
      void ConstructBranch(TTree *tree, TString & prefix, QwParameterFile& trim_file) override;




      //update the error flag in the subsystem level from the top level routines related to stability checks
      // This will uniquely update the errorflag at each channel based on the error flag in the corresponding channel in the ev_error subsystem
      void UpdateErrorFlag(const VQwSubsystem *ev_error) override;


      /// \brief Derived functions
      // not sure if there should be empty definition, no definition or defined
      Int_t LoadChannelMap(TString) override;
      Int_t LoadInputParameters(TString) override;
      Int_t LoadEventCuts(TString) override;
      void ClearEventData() override{
        for (size_t i = 0; i < fOutputVar.size(); ++i) {
          if (fOutputVar.at(i) != NULL) {
            fOutputVar.at(i)->ClearEventData();
          }
        }
        /*
        Iterator_HdwChan element;
        for (element = fOutputVar.begin(); element != fOutputVar.end(); ++element) {
          if (*element != NULL) {
            (*element)->ClearEventData();
          }
        }
        */
      };
      Int_t ProcessConfigurationBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) override;
	Int_t ProcessEvBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) override;
      void ProcessEvent() override{};

      Bool_t ApplySingleEventCuts() override;

      Bool_t  CheckForBurpFail(const VQwSubsystem *ev_error) override{
	      return kFALSE;
      };

      void IncrementErrorCounters() override;
      void PrintErrorCounters() const override;
      UInt_t GetEventcutErrorFlag() override;


  private:

     /**
      * Default Constructor
      *
      * Error: tries to call default constructors of base class,
      * 	QwCombiner() is private
      */
   //   QwCombinerSubsystem() {};


}; // class QwCombinerSubsystem

// Register this subsystem with the factory
REGISTER_SUBSYSTEM_FACTORY(QwCombinerSubsystem);
