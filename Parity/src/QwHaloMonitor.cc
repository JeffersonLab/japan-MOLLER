/**
 * \file QwHaloMonitor.cc
 * \brief Implementation of the halo monitor data element.
 */
/**********************************************************\
* File: QwHaloMonitor.cc                                  *
*                                                         *
* Author:B. Waidyawansa                                   *
* Time-stamp:24-june-2010                                 *
\**********************************************************/

#include "QwHaloMonitor.h"

// System headers
#include <stdexcept>

// ROOT headers for RNTuple support
#ifdef HAS_RNTUPLE_SUPPORT
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleWriter.hxx>
#endif // HAS_RNTUPLE_SUPPORT

// Qweak headers
#ifdef __USE_DATABASE__
#include "QwDBInterface.h"
#endif // __USE_DATABASE__

/**
 * \brief Initialize the halo monitor with subsystem and name.
 * \param subsystem Subsystem identifier.
 * \param name Detector name used for branches and histograms.
 */
void  QwHaloMonitor::InitializeChannel(TString subsystem, TString name){
  fHalo_Counter.InitializeChannel(name);
  SetElementName(name);
}

/**
 * \brief Initialize the halo monitor with a detector name.
 * \param name Detector name used for branches and histograms.
 */
void  QwHaloMonitor::InitializeChannel(TString name){
  fHalo_Counter.InitializeChannel(name);
  SetElementName(name);
}

/** \brief Clear event-scoped data in the underlying counter. */
void QwHaloMonitor::ClearEventData()
{
  fHalo_Counter.ClearEventData();
}

/** \brief Process event (delegated to the underlying counter). */
void QwHaloMonitor::ProcessEvent()
{
  // no processing required for the halos as they are just counters(?).
  fHalo_Counter.ProcessEvent();
}

/** \brief Decode the raw event buffer into the underlying counter. */
Int_t QwHaloMonitor::ProcessEvBuffer(UInt_t* buffer, UInt_t num_words_left,UInt_t index)
{
  return fHalo_Counter.ProcessEvBuffer(buffer,num_words_left);
}


/** \brief Apply hardware checks (no hardware errors for simple counters). */
Bool_t QwHaloMonitor::ApplyHWChecks()
{
  Bool_t eventokay=kTRUE;
  return eventokay;
}


/** \brief Apply single-event cuts on the underlying counter. */
Bool_t QwHaloMonitor::ApplySingleEventCuts()
{
  return fHalo_Counter.ApplySingleEventCuts();
}


/** \brief Print accumulated error counters for this monitor. */
void QwHaloMonitor::PrintErrorCounters() const
{
  fHalo_Counter.PrintErrorCounters();
}


/** \brief Copy-assign from another halo monitor. */
QwHaloMonitor& QwHaloMonitor::operator= (const QwHaloMonitor &value)
{
  if (GetElementName()!=""){
    this->fHalo_Counter=value.fHalo_Counter;
  }
  return *this;
}

/** \brief Add-assign from another halo monitor (sum counters). */
QwHaloMonitor& QwHaloMonitor::operator+= (const QwHaloMonitor &value)
{
  if (GetElementName()!=""){
    this->fHalo_Counter+=value.fHalo_Counter;
  }
  return *this;
}

/** \brief Subtract-assign from another halo monitor (difference counters). */
QwHaloMonitor& QwHaloMonitor::operator-= (const QwHaloMonitor &value)
{
  if (GetElementName()!=""){
    this->fHalo_Counter-=value.fHalo_Counter;
  }
  return *this;
}


/** \brief Sum two halo monitors into this instance. */
void QwHaloMonitor::Sum(QwHaloMonitor &value1, QwHaloMonitor &value2){
  *this =  value1;
  *this += value2;
}

/** \brief Compute the difference of two halo monitors into this instance. */
void QwHaloMonitor::Difference(QwHaloMonitor &value1, QwHaloMonitor &value2){
  *this =  value1;
  *this -= value2;
}

/** \brief Form the ratio of two halo monitors into this instance. */
void QwHaloMonitor::Ratio(QwHaloMonitor &numer, QwHaloMonitor &denom)
{
  if (GetElementName()!=""){
      this->fHalo_Counter.Ratio(numer.fHalo_Counter,denom.fHalo_Counter);
  }
  return;
}

/** \brief Scale the underlying counter by a constant factor. */
void QwHaloMonitor::Scale(Double_t factor)
{
  fHalo_Counter.Scale(factor);
}

/** \brief Accumulate running sums from another monitor into this one. */
void QwHaloMonitor::AccumulateRunningSum(const QwHaloMonitor& value, Int_t count, Int_t ErrorMask) {
  fHalo_Counter.AccumulateRunningSum(value.fHalo_Counter, count, ErrorMask);
}

/** \brief Remove a single entry from the running sums using a source value. */
void QwHaloMonitor::DeaccumulateRunningSum(QwHaloMonitor& value, Int_t ErrorMask) {
  fHalo_Counter.DeaccumulateRunningSum(value.fHalo_Counter, ErrorMask);
}

/** \brief Update running averages for the underlying counter. */
void QwHaloMonitor::CalculateRunningAverage(){
  fHalo_Counter.CalculateRunningAverage();
}


/** \brief Print a compact value summary for this monitor. */
void QwHaloMonitor::PrintValue() const
{
  fHalo_Counter.PrintValue();
}

/** \brief Print detailed information for this monitor. */
void QwHaloMonitor::PrintInfo() const
{
  std::cout << "QwVQWK_Channel Info " << std::endl;
  fHalo_Counter.PrintInfo();
}

/**
 * \brief Check for burp failures by delegating to the underlying counter.
 * \param ev_error Reference halo monitor to compare against.
 * \return kTRUE if a burp failure was detected; otherwise kFALSE.
 */
Bool_t QwHaloMonitor::CheckForBurpFail(const VQwDataElement *ev_error){
  Bool_t burpstatus = kFALSE;
  try {
    if(typeid(*ev_error)==typeid(*this)) {
      //std::cout<<" Here in QwHaloMonitor::CheckForBurpFail \n";
      if (this->GetElementName()!="") {
        const QwHaloMonitor* value_halo = dynamic_cast<const QwHaloMonitor* >(ev_error);
        burpstatus |= fHalo_Counter.CheckForBurpFail(&(value_halo->fHalo_Counter)); 
      }
    } else {
      TString loc="Standard exception from QwHaloMonitor::CheckForBurpFail :"+
        ev_error->GetElementName()+" "+this->GetElementName()+" are not of the "
        +"same type";
      throw std::invalid_argument(loc.Data());
    }
  } catch (std::exception& e) {
    std::cerr<< e.what()<<std::endl;
  }
  return burpstatus;
}

/**
 * \brief Define histograms for this monitor (delegated to underlying counter).
 * \param folder ROOT folder to contain histograms.
 * \param prefix Histogram name prefix.
 */
void  QwHaloMonitor::ConstructHistograms(TDirectory *folder, TString &prefix)
{
  if (GetElementName()==""){
    //  This channel is not used, so skip filling the histograms.
  }
  else{
      fHalo_Counter.ConstructHistograms(folder, prefix);
  }
}

/** \brief Fill histograms for this monitor if enabled. */
void  QwHaloMonitor::FillHistograms()
{
  if (GetElementName()==""){
    //  This channel is not used, so skip filling the histograms.
  }
  else{
    fHalo_Counter.FillHistograms();
  }
}

/**
 * \brief Construct ROOT branches and value vector entries.
 * \param tree Output tree.
 * \param prefix Branch name prefix.
 * \param values Output value vector to be appended.
 */
void  QwHaloMonitor::ConstructBranchAndVector(TTree *tree, TString &prefix, QwRootTreeBranchVector &values)
{
  if (GetElementName()==""){
    //  This channel is not used, so skip filling the histograms.
  }
  else{
    fHalo_Counter.ConstructBranchAndVector(tree, prefix,values);
    // this functions doesn't do anything yet
  }
}

void  QwHaloMonitor::ConstructBranch(TTree *tree, TString &prefix)
{
  if (GetElementName()==""){
    //  This channel is not used, so skip filling the histograms.
  }
  else{
    fHalo_Counter.ConstructBranch(tree, prefix);
    // this functions doesn't do anything yet
  }
}



void  QwHaloMonitor::ConstructBranch(TTree *tree, TString &prefix, QwParameterFile& modulelist)
{
  TString devicename;

  devicename=GetElementName();
  devicename.ToLower();
  if (GetElementName()==""){
    //  This channel is not used, so skip filling the histograms.
  } else {

    //QwMessage <<" QwHaloMonitor "<<devicename<<QwLog::endl;
    if (modulelist.HasValue(devicename)){
      fHalo_Counter.ConstructBranch(tree, prefix);
      QwMessage <<" Tree leaf added to "<<devicename<<QwLog::endl;
    }
    // this functions doesn't do anything yet
  }
}




void  QwHaloMonitor::FillTreeVector(QwRootTreeBranchVector &values) const
{
  if (GetElementName()==""){
    //  This channel is not used, so skip filling the histograms.
  }
  else{
    fHalo_Counter.FillTreeVector(values);
    // this functions doesn't do anything yet
  }
}

#ifdef HAS_RNTUPLE_SUPPORT
void  QwHaloMonitor::ConstructNTupleAndVector(std::unique_ptr<ROOT::RNTupleModel>& model, TString& prefix, QwRootTreeBranchVector &values, std::vector<std::shared_ptr<Double_t>>& fieldPtrs)
{
  if (GetElementName()==""){
    //  This channel is not used, so skip construction.
  }
  else{
    fHalo_Counter.ConstructNTupleAndVector(model, prefix, values, fieldPtrs);
  }
}

void  QwHaloMonitor::FillNTupleVector(QwRootTreeBranchVector &values) const
{
  if (GetElementName()==""){
    //  This channel is not used, so skip filling.
  }
  else{
    fHalo_Counter.FillNTupleVector(values);
  }
}
#endif // HAS_RNTUPLE_SUPPORT

#ifdef __USE_DATABASE__
std::vector<QwDBInterface> QwHaloMonitor::GetDBEntry()
{
  std::vector <QwDBInterface> row_list;
  row_list.clear();
  fHalo_Counter.AddEntriesToList(row_list);
  return row_list;
}


std::vector<QwErrDBInterface> QwHaloMonitor::GetErrDBEntry()
{
  std::vector <QwErrDBInterface> row_list;
  row_list.clear();
  fHalo_Counter.AddErrEntriesToList(row_list);
  return row_list;
}
#endif // __USE_DATABASE__
