/**********************************************************\
* File: QwSubsystemArrayParity.cc                         *
*                                                         *
* Author:                                                 *
* Time-stamp:                                             *
\**********************************************************/

#include "QwSubsystemArrayParity.h"

// System headers
#include <stdexcept>

// Qweak headers
#include "VQwSubsystemParity.h"

//*****************************************************************//

/// Default constructor
QwSubsystemArrayParity::QwSubsystemArrayParity(QwOptions& options)
: QwSubsystemArray(options, CanContain),
  fErrorFlag(0),
  fErrorFlagTreeIndex(-1)
{

}

//*****************************************************************//

/// Copy constructor
QwSubsystemArrayParity::QwSubsystemArrayParity(const QwSubsystemArrayParity& source)
: QwSubsystemArray(source)
{
  // Copy error flags
  fErrorFlag = source.fErrorFlag;
  fErrorFlagTreeIndex = source.fErrorFlagTreeIndex;
}

//*****************************************************************//

/// Destructor
QwSubsystemArrayParity::~QwSubsystemArrayParity()
{
  // nothing
}

//*****************************************************************//

VQwSubsystemParity* QwSubsystemArrayParity::GetSubsystemByName(const TString& name)
{
  return dynamic_cast<VQwSubsystemParity*>(QwSubsystemArray::GetSubsystemByName(name));
}

//*****************************************************************//

void  QwSubsystemArrayParity::FillDB_MPS(QwParityDB *db, TString type)
{
  for (iterator subsys = begin(); subsys != end(); ++subsys) {
    VQwSubsystemParity* subsys_parity = dynamic_cast<VQwSubsystemParity*>(subsys->get());
    subsys_parity->FillDB_MPS(db, type);
  }
}

void  QwSubsystemArrayParity::FillDB(QwParityDB *db, TString type)
{
  for (iterator subsys = begin(); subsys != end(); ++subsys) {
    VQwSubsystemParity* subsys_parity = dynamic_cast<VQwSubsystemParity*>(subsys->get());
    subsys_parity->FillDB(db, type);
  }
}

void  QwSubsystemArrayParity::FillErrDB(QwParityDB *db, TString type)
{
  //  for (const_iterator subsys = dummy_source->begin(); subsys != dummy_source->end(); ++subsys) {
  for (iterator subsys = begin(); subsys != end(); ++subsys) {
    VQwSubsystemParity* subsys_parity = dynamic_cast<VQwSubsystemParity*>(subsys->get());
    subsys_parity->FillErrDB(db, type);
  }
  return;
}

void QwSubsystemArrayParity::WritePromptSummary(QwPromptSummary *ps, TString type)
{
  for (const_iterator subsys = begin(); subsys != end(); ++subsys) {
    VQwSubsystemParity* subsys_parity = dynamic_cast<VQwSubsystemParity*>(subsys->get());
    subsys_parity->WritePromptSummary(ps, type);
  }
}

//*****************************************************************//

/**
 * Assignment operator
 * @param source Subsystem array to assign to this array
 * @return This subsystem array after assignment
 */
QwSubsystemArrayParity& QwSubsystemArrayParity::operator= (const QwSubsystemArrayParity &source)
{
  QwSubsystemArray::operator=(source);
  this->fErrorFlag = source.fErrorFlag;
  return *this;
}


/**
 * Addition-assignment operator
 * @param value Subsystem array to add to this array
 * @return This subsystem array after addition
 */
QwSubsystemArrayParity& QwSubsystemArrayParity::operator+= (const QwSubsystemArrayParity &value)
{
  if (!value.empty()){
    fCodaEventNumber = (fCodaEventNumber == 0) ? value.fCodaEventNumber :
        std::min(fCodaEventNumber, value.fCodaEventNumber);
    if (this->size() == value.size()){
      this->fErrorFlag|=value.fErrorFlag;
      for(size_t i=0;i<value.size();i++){
	if (value.at(i)==NULL || this->at(i)==NULL){
	  //  Either the value or the destination subsystem
	  //  are null
	} else {
	  VQwSubsystemParity *ptr1 =
	    dynamic_cast<VQwSubsystemParity*>(this->at(i).get());
	  if (typeid(*ptr1)==typeid(*(value.at(i).get()))){
	    *(ptr1) += value.at(i).get();
	    //std::cout<<"QwSubsystemArrayParity::operator+ here where types match \n";
	  } else {
	    QwError << "QwSubsystemArrayParity::operator+ here where types don't match" << QwLog::endl;
	    QwError << " typeid(ptr1)=" << typeid(ptr1).name()
                    << " but typeid(value.at(i)))=" << typeid(value.at(i)).name()
                    << QwLog::endl;
	    //  Subsystems don't match
	  }
	}
      }
    } else {
      //  Array sizes don't match
    }
  } else {
    //  The vsource is empty
  }
  return *this;
}

/**
 * Subtraction-assignment operator
 * @param value Subsystem array to subtract from this array
 * @return This array after subtraction
 */
QwSubsystemArrayParity& QwSubsystemArrayParity::operator-= (const QwSubsystemArrayParity &value)
{
  if (!value.empty()){
    fCodaEventNumber = (fCodaEventNumber == 0) ? value.fCodaEventNumber :
        std::min(fCodaEventNumber, value.fCodaEventNumber);
    if (this->size() == value.size()){
      this->fErrorFlag|=value.fErrorFlag;
      for(size_t i=0;i<value.size();i++){
	if (value.at(i)==NULL || this->at(i)==NULL){
	  //  Either the value or the destination subsystem
	  //  are null
	} else {
	  VQwSubsystemParity *ptr1 =
	    dynamic_cast<VQwSubsystemParity*>(this->at(i).get());
	  if (typeid(*ptr1)==typeid(*(value.at(i).get()))){
	    *(ptr1) -= value.at(i).get();
	  } else {
	    //  Subsystems don't match
	  }
	}
      }
    } else {
      //  Array sizes don't match
    }
  } else {
    //  The vsource is empty
  }
  return *this;
}

/**
 * Sum of two subsystem arrays
 * @param value1 First subsystem array
 * @param value2 Second subsystem array
 */
void QwSubsystemArrayParity::Sum(
  const QwSubsystemArrayParity &value1,
  const QwSubsystemArrayParity &value2)
{
  if (!value1.empty()&& !value2.empty()){
    *(this)  = value1;
    *(this) += value2;
  } else {
    //  The source is empty
  }
}

/**
 * Difference of two subsystem arrays
 * @param value1 First subsystem array
 * @param value2 Second subsystem array
 */
void QwSubsystemArrayParity::Difference(
  const QwSubsystemArrayParity &value1,
  const QwSubsystemArrayParity &value2)
{

  if (!value1.empty()&& !value2.empty()){
    *(this)  =  value1;
    *(this) -= value2;
  } else {
    //  The source is empty
  }
}

/**
 * Scale this subsystem array
 * @param factor Scale factor
 */
void QwSubsystemArrayParity::Scale(Double_t factor)
{
  for (iterator subsys = begin(); subsys != end(); ++subsys) {
    VQwSubsystemParity* subsys_parity = dynamic_cast<VQwSubsystemParity*>(subsys->get());
    subsys_parity->Scale(factor);
  }
}

//*****************************************************************//

void QwSubsystemArrayParity::PrintValue() const
{
  for (const_iterator subsys = begin(); subsys != end(); ++subsys) {
    VQwSubsystemParity* subsys_parity = dynamic_cast<VQwSubsystemParity*>(subsys->get());
    subsys_parity->PrintValue();
  }
}



//*****************************************************************//

Bool_t QwSubsystemArrayParity::CheckForEndOfBurst() const
{
  Bool_t status = kFALSE;
  for (const_iterator subsys = begin(); subsys != end(); ++subsys) {
    VQwSubsystemParity* subsys_parity = dynamic_cast<VQwSubsystemParity*>(subsys->get());
    status |= subsys_parity->CheckForEndOfBurst();
  }
  return status;
};

//*****************************************************************//

void QwSubsystemArrayParity::CalculateRunningAverage()
{
  for (iterator subsys = begin(); subsys != end(); ++subsys) {
    VQwSubsystemParity* subsys_parity = dynamic_cast<VQwSubsystemParity*>(subsys->get());
    subsys_parity->CalculateRunningAverage();
  }
}



void QwSubsystemArrayParity::AccumulateRunningSum(const QwSubsystemArrayParity& value, Int_t count, Int_t ErrorMask)
{
  if (!value.empty()) {
    if (this->size() == value.size()) {
      if (value.GetEventcutErrorFlag()==0){//do running sum only if error flag is zero. This way will prevent any Beam Trip(in ev mode 3) related events going into the running sum.
        fCodaEventNumber = (fCodaEventNumber == 0) ? value.fCodaEventNumber :
            std::min(fCodaEventNumber, value.fCodaEventNumber);
        for (size_t i = 0; i < value.size(); i++) {
	  if (value.at(i)==NULL || this->at(i)==NULL) {
	    //  Either the value or the destination subsystem
	    //  are null
	  } else {
	    VQwSubsystemParity *ptr1 =
	      dynamic_cast<VQwSubsystemParity*>(this->at(i).get());
	    if (typeid(*ptr1) == typeid(*(value.at(i).get()))) {
	      ptr1->AccumulateRunningSum(value.at(i).get(), count, ErrorMask);
	    } else {
	      QwError << "QwSubsystemArrayParity::AccumulateRunningSum here where types don't match" << QwLog::endl;
	      QwError << " typeid(ptr1)=" << typeid(ptr1).name()
		      << " but typeid(value.at(i)))=" << typeid(value.at(i)).name()
		      << QwLog::endl;
	      //  Subsystems don't match
	    }
	  }
	}
      }

    } else {
      //  Array sizes don't match

    }
  } else {
    //  The value is empty
  }
}

void QwSubsystemArrayParity::AccumulateAllRunningSum(const QwSubsystemArrayParity& value, Int_t count, Int_t ErrorMask)
{
  if (!value.empty()) {
    if (this->size() == value.size()) {
      //if (value.GetEventcutErrorFlag()==0){//do running sum only if error flag is zero. This way will prevent any Beam Trip(in ev mode 3) related events going into the running sum.
	for (size_t i = 0; i < value.size(); i++) {
	  if (value.at(i)==NULL || this->at(i)==NULL) {
	    //  Either the value or the destination subsystem
	    //  are null
	  } else {
	    VQwSubsystemParity *ptr1 =
	      dynamic_cast<VQwSubsystemParity*>(this->at(i).get());
	    if (typeid(*ptr1) == typeid(*(value.at(i).get()))) {
	      ptr1->AccumulateRunningSum(value.at(i).get(), count, ErrorMask);
	    } else {
	      QwError << "QwSubsystemArrayParity::AccumulateRunningSum here where types don't match" << QwLog::endl;
	      QwError << " typeid(ptr1)=" << typeid(ptr1).name()
		      << " but typeid(value.at(i)))=" << typeid(value.at(i)).name()
		      << QwLog::endl;
	      //  Subsystems don't match
	    }
	  }
	}
	//}//else if ((value.fErrorFlag& 512)==512){
      //QwMessage << " AccumulateRunningSum "<<(value.fErrorFlag & 0x2FF)<<" - "<<value.GetCodaEventNumber()<< QwLog::endl;
      //}
    } else {
      //  Array sizes don't match

    }
  } else {
    //  The value is empty
  }
}


void QwSubsystemArrayParity::DeaccumulateRunningSum(const QwSubsystemArrayParity& value, Int_t ErrorMask)
{
  //Bool_t berror=kTRUE;//only needed for deaccumulation (stability check purposes)
  //if (value.fErrorFlag>0){//check the error is global
  //berror=((value.fErrorFlag & 0x2FF) == 0); //The operation value.fErrorFlag & 0x2FF clear everything else but the HW errors + event cut errors + blinder error    
  //}
  if (!value.empty()) {
    if (this->size() == value.size()) {
      //if (value.GetEventcutErrorFlag()==0){//do derunningsum only if error flag is zero. 
	for (size_t i = 0; i < value.size(); i++) {
	  if (value.at(i)==NULL || this->at(i)==NULL) {
	    //  Either the value or the destination subsystem
	    //  are null
	  } else {
	    VQwSubsystemParity *ptr1 =
	      dynamic_cast<VQwSubsystemParity*>(this->at(i).get());
	    if (typeid(*ptr1) == typeid(*(value.at(i).get()))) {
	      ptr1->DeaccumulateRunningSum(value.at(i).get(), ErrorMask);
	    } else {
	      QwError << "QwSubsystemArrayParity::AccumulateRunningSum here where types don't match" << QwLog::endl;
	      QwError << " typeid(ptr1)=" << typeid(ptr1).name()
		      << " but typeid(value.at(i)))=" << typeid(value.at(i)).name()
		      << QwLog::endl;
	      //  Subsystems don't match
	    }
	  }
	}
	//}//else if ((value.fErrorFlag & 268435968)==268435968){
      //QwMessage << " DeaccumulateRunningSum "<<(value.fErrorFlag & 0x2FF)<<" - "<<value.GetCodaEventNumber()<< QwLog::endl;
      //}
    } else {
      //  Array sizes don't match

    }
  } else {
    //  The value is empty
  }
}


void QwSubsystemArrayParity::Blind(const QwBlinder *blinder)
{
  // Loop over subsystem array
  for (size_t i = 0; i < this->size(); i++) {
    // Cast into parity subsystems
    VQwSubsystemParity* subsys = dynamic_cast<VQwSubsystemParity*>(this->at(i).get());

    // Check for null pointers
    if (this->at(i) == 0) {
      QwError << "QwSubsystemArrayParity::Blind: "
              << "parity subsystem null pointer!" << QwLog::endl;
      return;
    }

    // Apply blinding
    subsys->Blind(blinder);
  }
}

void QwSubsystemArrayParity::Blind(const QwBlinder *blinder, const QwSubsystemArrayParity& yield)
{
  // Check for array size
  if (this->size() != yield.size()) {
    QwError << "QwSubsystemArrayParity::Blind: "
            << "diff and yield array dimension mismatch!" << QwLog::endl;
    return;
  }

  // Loop over subsystem array
  for (size_t i = 0; i < this->size(); i++) {
    // Cast into parity subsystems
    VQwSubsystemParity* subsys_diff  = dynamic_cast<VQwSubsystemParity*>(this->at(i).get());
    VQwSubsystemParity* subsys_yield = dynamic_cast<VQwSubsystemParity*>(yield.at(i).get());

    // Check for null pointers
    if (subsys_diff == 0 || subsys_yield == 0) {
      QwError << "QwSubsystemArrayParity::Blind: "
              << "diff or yield parity subsystem null pointer!" << QwLog::endl;
      return;
    }

    // Apply blinding
    subsys_diff->Blind(blinder, subsys_yield);
  }
}

void QwSubsystemArrayParity::Ratio(
  const QwSubsystemArrayParity &numer,
  const QwSubsystemArrayParity &denom)
{
  Bool_t localdebug=kFALSE;

  if(localdebug) std::cout<<"QwSubsystemArrayParity::Ratio \n";
  *this=numer;
  if ( !denom.empty()){
    this->fErrorFlag=(numer.fErrorFlag|denom.fErrorFlag);
    if (this->size() == denom.size() ){
      for(size_t i=0;i<denom.size();i++){
        if (denom.at(i)==NULL || this->at(i)==NULL){
          //  Either the value or the destination subsystem  are null
	  if(localdebug) std::cout<<"Either the value or the destination subsystem  are null\n";
        } else {
	  VQwSubsystemParity *ptr1 =
	    dynamic_cast<VQwSubsystemParity*>(this->at(i).get());
	  if (typeid(*ptr1)==typeid(*(denom.at(i).get())))
            {
              ptr1->Ratio(numer.at(i).get(),denom.at(i).get());
            } else {
              //  Subsystems don't match
              QwError << "subsystem #" << i
                      << " type do not match : ratio computation aborted" << QwLog::endl;
            }
        }
      }
    } else {
      QwError << "array size do not match : ratio computation aborted" << QwLog::endl;
      //  Array sizes don't match
    }
  } else {
    QwError << "source empty : ratio computation aborted" << QwLog::endl;
    //  The source is empty
  }
  if(localdebug) std::cout<<"I am out of it \n";

}

Bool_t QwSubsystemArrayParity::ApplySingleEventCuts(){
  Int_t CountFalse;
  Bool_t status;
  UInt_t ErrorFlag;
  fErrorFlag=0;  // Testing if event number is within bad Event Range cut
  if( CheckBadEventRange() )
    fErrorFlag |=kBadEventRangeError;
  
  VQwSubsystemParity *subsys_parity;
  CountFalse=0;
  if (!empty()){
    for (iterator subsys = begin(); subsys != end(); ++subsys){
      subsys_parity=dynamic_cast<VQwSubsystemParity*>((subsys)->get());
      status=subsys_parity->ApplySingleEventCuts();
      ErrorFlag = subsys_parity->GetEventcutErrorFlag();
      if ((ErrorFlag & kEventCutMode3)==kEventCutMode3)//we only care about the event cut flag in event cut mode 3
	fErrorFlag |= ErrorFlag; 
      if (!status)
      {
	if ((ErrorFlag&kGlobalCut)==kGlobalCut){
	  CountFalse++;
	  fErrorFlag |= ErrorFlag; //we need the error code for failed events in event mode 2 for beam trips and etc.
	}
      }
      

    }
  }
  if (CountFalse > 0)
    status = kFALSE;
  else
    status = kTRUE;

  //  Propagate all error codes to derived objects in the subsystems.
  UpdateErrorFlag();

  return status;
}



void QwSubsystemArrayParity::IncrementErrorCounters()
{
  VQwSubsystemParity *subsys_parity;
  if (!empty()){
    for (iterator subsys = begin(); subsys != end(); ++subsys){
      subsys_parity=dynamic_cast<VQwSubsystemParity*>((subsys)->get());
      subsys_parity->IncrementErrorCounters();
    }
  }
}

Bool_t QwSubsystemArrayParity::CheckForBurpFail(QwSubsystemArrayParity &event)
{
  Bool_t burpstatus = kFALSE;
  if (!event.empty() && this->size() == event.size()){
    for(size_t i=0;i<event.size();i++){
      if (event.at(i)!=NULL && this->at(i)!=NULL){
	      VQwSubsystemParity *ptr1 = dynamic_cast<VQwSubsystemParity*>(this->at(i).get());
	      if (typeid(*ptr1)==typeid(*(event.at(i).get()))){
	        //*(ptr1) = event.at(i).get();//when =operator is used
	        //pass the correct subsystem to update the errorflags at subsystem to devices to channel levels
          //wError << "************* test " << typeid(*ptr1).name() << "*****************" << QwLog::endl;
	        burpstatus |= ptr1->CheckForBurpFail(event.at(i).get());
	      } else {
	        //  Subsystems don't match
	        QwError << " QwSubsystemArrayParity::CheckForBurpFail types do not mach" << QwLog::endl;
	        QwError << " typeid(ptr1)=" << typeid(*ptr1).name()
		      << " but typeid(*(event.at(i).get()))=" << typeid(*(event.at(i).get())).name()
		      << QwLog::endl;
	      }
      }
    }
  } else {
    //  The source is empty
  }
  return burpstatus;
}


void QwSubsystemArrayParity::PrintErrorCounters() const{// report number of events failed due to HW and event cut faliure
  const VQwSubsystemParity *subsys_parity;
  if (!empty()){
    for (const_iterator subsys = begin(); subsys != end(); ++subsys){
      subsys_parity=dynamic_cast<const VQwSubsystemParity*>((subsys)->get());
      subsys_parity->PrintErrorCounters();
    }
  }
}

void QwSubsystemArrayParity::UpdateErrorFlag(const QwSubsystemArrayParity& ev_error){
  Bool_t localdebug=kFALSE;//kTRUE;
  if(localdebug)  std::cout<<"QwSubsystemArrayParity::UpdateErrorFlag \n";
  if (!ev_error.empty()){
    if (this->size() == ev_error.size()){
      this->fErrorFlag |= ev_error.fErrorFlag;
      for(size_t i=0;i<ev_error.size();i++){
	if (ev_error.at(i)==NULL || this->at(i)==NULL){
	  //  Either the source or the destination subsystem
	  //  are null
	} else {
	  VQwSubsystemParity *ptr1 =
	    dynamic_cast<VQwSubsystemParity*>(this->at(i).get());
	  if (typeid(*ptr1)==typeid(*(ev_error.at(i).get()))){
	    if(localdebug) std::cout<<" here in QwSubsystemArrayParity::UpdateErrorFlag types mach \n";
	    //*(ptr1) = ev_error.at(i).get();//when =operator is used
	    //pass the correct subsystem to update the errorflags at subsystem to devices to channel levels
	    ptr1->UpdateErrorFlag(ev_error.at(i).get());
	  } else {
	    //  Subsystems don't match
	      QwError << " QwSubsystemArrayParity::UpdateErrorFlag types do not mach" << QwLog::endl;
	      QwError << " typeid(ptr1)=" << typeid(*ptr1).name()
                      << " but typeid(*(ev_error.at(i).get()))=" << typeid(*(ev_error.at(i).get())).name()
                      << QwLog::endl;
	  }
	}
      }
    } else {
      //  Array sizes don't match
    }
  } else {
    //  The source is empty
  }  
};


void QwSubsystemArrayParity::UpdateErrorFlag() { 
  //this routine will refresh the global error flag after stability cut check
  //by default at the ApplySingleEventCuts routine fErrorFlag is updated properly and a const GetEventcutErrorFlag() routine 
  //returns the fErrorFlag value
  fErrorFlag=0;
  if( CheckBadEventRange() )
    fErrorFlag |=kBadEventRangeError;

  VQwSubsystemParity *subsys_parity;
  if (!empty()){
    for (iterator subsys = begin(); subsys != end(); ++subsys){
      subsys_parity=dynamic_cast<VQwSubsystemParity*>((subsys)->get());
      //Update the error flag of the parity subsystem
      fErrorFlag|=subsys_parity->UpdateErrorFlag();
    }
  }
}

Bool_t QwSubsystemArrayParity::CheckBadEventRange(){
  std::vector< std::pair<UInt_t, UInt_t> >::iterator itber = fBadEventRange.begin(); // ber = bad event range
  while(itber!=fBadEventRange.end()){
    if( fCodaEventNumber >= (*itber).first 
        && fCodaEventNumber <= (*itber).second){
      return kTRUE;
    }
      itber++;
  }
  return kFALSE;
}

void  QwSubsystemArrayParity::ConstructBranchAndVector(TTree *tree, TString& prefix, std::vector<Double_t>& values){
  QwSubsystemArray::ConstructBranchAndVector(tree, prefix, values);
  if (prefix.Contains("yield_") || prefix==""){
    values.push_back(0.0);
    fErrorFlagTreeIndex = values.size()-1;
    tree->Branch("ErrorFlag",&(values[fErrorFlagTreeIndex]),"ErrorFlag/D");
  } else {
    fErrorFlagTreeIndex = -1;
  }
}

void QwSubsystemArrayParity::FillTreeVector(std::vector<Double_t>& values) const
{
  QwSubsystemArray::FillTreeVector(values);
  if (fErrorFlagTreeIndex>=0 && fErrorFlagTreeIndex<static_cast<int>(values.size())){
    values.at(fErrorFlagTreeIndex) = fErrorFlag;
  }
}


//*****************************************************************//
void  QwSubsystemArrayParity::FillHistograms()
{
  if (GetEventcutErrorFlag()==0)
    QwSubsystemArray::FillHistograms();
}

//*****************************************************************//
void QwSubsystemArrayParity::LoadMockDataParameters(std::string mapfile)
{
  // for (iterator subsys = begin(); subsys != end(); ++subsys) {
  //   VQwSubsystemParity* subsys_parity = dynamic_cast<VQwSubsystemParity*>(GetSubsystemByName(subsys_name)->LoadMockDataParameters(mock_param_name));
  //   subsys_parity->LoadMockDataParameters(mapfile);
  // }
  QwParameterFile detectors(mapfile);
    // This is how this should work
  QwParameterFile* preamble;
  preamble = detectors.ReadSectionPreamble();
  // Process preamble
  QwVerbose << "Preamble:" << QwLog::endl;
  QwVerbose << *preamble << QwLog::endl;
  double window_period;
  if (preamble->FileHasVariablePair("=","window_period",window_period)){
    fWindowPeriod = window_period * Qw::sec;
  }else{
    fWindowPeriod = Qw::ms;
  }

  QwMessage << "fWindowPeriod = " << fWindowPeriod << QwLog::endl;

    
  if (preamble) delete preamble;

  QwParameterFile* section;
  std::string section_name;
  while ((section = detectors.ReadNextSection(section_name))) {

    // Debugging output of configuration section
    QwVerbose << "[" << section_name << "]" << QwLog::endl;
    QwVerbose << *section << QwLog::endl;

    // Determine type and name of subsystem
    std::string subsys_type = section_name;
    std::string subsys_name;
    if (! section->FileHasVariablePair("=","name",subsys_name)) {
      QwError << "No name defined in section for subsystem " << subsys_type << "." << QwLog::endl;
      delete section; section = 0;
      continue;
    }
    std::string mock_param_name;
    if (! section->FileHasVariablePair("=","mock_param",mock_param_name)) {
     QwError << "No mock data parameter defined for " << subsys_name << "." << QwLog::endl;
     delete section; section = 0;
     continue;
    }
    VQwSubsystemParity* subsys_parity = dynamic_cast<VQwSubsystemParity*>(GetSubsystemByName(subsys_name));
    if (! subsys_parity){
      QwError << "Subsystem " << subsys_name << " listed in the mock-data-parameter map does not match any subsystems in the detetor map file." <<QwLog::endl;
    } else {
      subsys_parity->LoadMockDataParameters(mock_param_name);
    }
    delete section; section = 0;
  }
}
