/*!
 * \file   QwPMT_Channel.cc
 * \brief  Implementation for PMT channel data element
 * \author P. M. King
 * \date   2009-03-07
 */

#include "QwPMT_Channel.h"

// Qweak headers
#include "QwLog.h"
#include "QwHistogramHelper.h"
#include "QwRootFile.h"

const Bool_t QwPMT_Channel::kDEBUG = kFALSE;


/*!  Conversion factor to translate the average bit count in an ADC
 *   channel into average voltage.
 *   The base factor is roughly 76 uV per count, and zero counts corresponds
 *   to zero voltage.
 *   Store as the exact value for 20 V range, 18 bit ADC.
 */
const Double_t QwPMT_Channel::kPMT_VoltsPerBit = (20./(1<<18));


/** \brief Clear the event-scoped ADC word value. */
void QwPMT_Channel::ClearEventData(){
  fValue   = 0;
}

/**
 * \brief Generate a mock ADC word for testing.
 * \param helicity Helicity state indicator (unused).
 * \param SlotNum V775 slot number to encode in the word.
 * \param ChanNum V775 channel number to encode in the word.
 */
void QwPMT_Channel::RandomizeEventData(int helicity, int SlotNum, int ChanNum){

  Double_t mean = 1500.0;
  Double_t sigma = 300.0;
  UInt_t fV775Dataword = abs( (Int_t)gRandom->Gaus(mean,sigma) );

  UInt_t fV775SlotNumber = SlotNum;
  UInt_t fV775ChannelNumber = ChanNum;
  const UInt_t fV775DataValidBit = 0x00004000;

  UInt_t word = fV775Dataword | (fV775SlotNumber<<27);
  word = word | (fV775ChannelNumber<<16) | fV775DataValidBit;
  fValue = word;
}

/** \brief Encode this channel's word into the trigger buffer. */
void  QwPMT_Channel::EncodeEventData(std::vector<UInt_t> &TrigBuffer)
{
//  std::cout<<"QwPMT_Channel::EncodeEventData() not fully implemented yet."<<std::endl;

  Long_t localbuf;

  if (IsNameEmpty()) {
    //  This channel is not used, but is present in the data stream.
    //  Skip over this data.
  } else {
    localbuf = (Long_t) (this->fValue);
    TrigBuffer.push_back(localbuf);
  }

}

/** \brief Process the event (no-op for simple PMT channel). */
void  QwPMT_Channel::ProcessEvent()
{

}


/**
 * \brief Create histograms for this channel within an optional folder.
 * \param folder Optional ROOT folder to cd() into.
 * \param prefix Histogram name prefix.
 */
void  QwPMT_Channel::ConstructHistograms(TDirectory *folder, TString &prefix)
{
  //  If we have defined a subdirectory in the ROOT file, then change into it.
  if (folder != NULL) folder->cd();

  if (GetElementName() == "") {
    //  This channel is not used, so skip filling the histograms.
  } else {
    //  Now create the histograms.
    TString basename, fullname;
    basename = prefix + GetElementName();

    fHistograms.resize(1, NULL);
    size_t index = 0;
    fHistograms[index] = gQwHists.Construct1DHist(basename);
    index++;
  }
}

/** \brief Fill histograms for this channel if present. */
void  QwPMT_Channel::FillHistograms()
{
  size_t index = 0;
  if (GetElementName() == "") {
    //  This channel is not used, so skip creating the histograms.
  } else {
    if (fHistograms[index] != NULL)
      fHistograms[index]->Fill(fValue);
    index++;
  }
}

/**
 * \brief Construct a ROOT branch and append a value slot to the vector.
 * \param tree Output tree.
 * \param prefix Branch name prefix.
 * \param values Output value vector to be appended.
 */
void  QwPMT_Channel::ConstructBranchAndVector(TTree *tree, TString &prefix, QwRootTreeBranchVector &values)
{
  if (GetElementName() == "") {
    //  This channel is not used, so skip setting up the tree.
  } else {
    TString basename = prefix + GetElementName();
    fTreeArrayIndex  = values.size();

    values.push_back(basename.Data(), 'D');

    fTreeArrayNumEntries = values.size() - fTreeArrayIndex;
    tree->Branch(basename, &(values[fTreeArrayIndex]), values.LeafList(fTreeArrayIndex).c_str());
  }
}

/** \brief Write this channel's value into the tree vector slot. */
void  QwPMT_Channel::FillTreeVector(QwRootTreeBranchVector &values) const
{
  if (GetElementName()==""){
    //  This channel is not used, so skip filling the tree vector.
  } else if (fTreeArrayNumEntries<=0){
    std::cerr << "QwPMT_Channel::FillTreeVector:  fTreeArrayNumEntries=="
	      << fTreeArrayNumEntries << std::endl;
  } else if (values.size() < fTreeArrayIndex+fTreeArrayNumEntries){
    std::cerr << "QwPMT_Channel::FillTreeVector:  values.size()=="
	      << values.size()
	      << "; fTreeArrayIndex+fTreeArrayNumEntries=="
	      << fTreeArrayIndex+fTreeArrayNumEntries
	      << std::endl;
  } else {
    size_t index=fTreeArrayIndex;
    values.SetValue(index++, this->fValue);
  }
}



/** \brief Copy-assign from another PMT channel (event-scoped data). */
QwPMT_Channel& QwPMT_Channel::operator= (const QwPMT_Channel &value){
  if (this != &value) {
    if (GetElementName()!=""){
      VQwDataElement::operator=(value);
      this->fValue  = value.fValue;
    }
  }
  return *this;
}

QwPMT_Channel& QwPMT_Channel::operator+= (const QwPMT_Channel &value){
  if (GetElementName()!=""){
    this->fValue += value.fValue;
  }
  return *this;
}

QwPMT_Channel& QwPMT_Channel::operator-= (const QwPMT_Channel &value){
  if (GetElementName()!=""){
    this->fValue -= value.fValue;
  }
  return *this;
}

void QwPMT_Channel::Sum(const QwPMT_Channel &value1, const QwPMT_Channel &value2) {
  *this = value1;
  *this += value2;
}

void QwPMT_Channel::Difference(const QwPMT_Channel &value1, const QwPMT_Channel &value2) {
  *this = value1;
  *this -= value2;
}

/** \brief Print a compact value summary for this PMT channel. */
void QwPMT_Channel::PrintValue() const
{
  QwMessage << std::setprecision(4)
            << std::setw(18) << std::left << GetElementName() << ", "
            << std::setw(15) << std::left << GetValue()
            << QwLog::endl;
}

/** \brief Print a placeholder info line for this PMT channel. */
void QwPMT_Channel::PrintInfo() const
{
  std::cout << "QwPMT_Channel::Print() not implemented yet." << std::endl;
}
