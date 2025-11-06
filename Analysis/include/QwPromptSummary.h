/*!
 * \file   QwPromptSummary.h
 * \brief  Prompt summary data management
 * \author jhlee@jlab.org
 * \date   2011-12-16
 */

#pragma once

#include <iostream>
#include <fstream>

#include "TObject.h"
#include "TClonesArray.h"
#include "TList.h"
#include "TString.h"

#include "TROOT.h"
#include "QwOptions.h"
#include "QwParameterFile.h"
/**
 *  \class QwPromptSummary
 *  \ingroup QwAnalysis
 *
 *  \brief
 *
 */

class PromptSummaryElement :  public TObject
{
 public:
  PromptSummaryElement();
  PromptSummaryElement(TString name);
  ~PromptSummaryElement() override;
  //  friend std::ostream& operator<<(std::ostream& os, const PromptSummaryElement &ps_element);

  void FillData(Double_t yield, Double_t yield_err, Double_t yield_width, TString yield_unit,
		Double_t asym_diff, Double_t asym_diff_err,  Double_t asym_diff_width, TString asym_diff_unit);

  void    SetElementName (const TString in)  {fElementName=in;};
  TString GetElementName ()                  {return fElementName;};


  // Yield      : fHardwareBlockSumM2
  // YieldError : fHardwareBlockSumError = sqrt(fHardwareBlockSumM2) / fGoodEventCount;
  //void SetNumGoodEvents        (const Double_t in) { fNumGoodEvents=in;};

  void SetYield                (const Double_t in) { fYield=in; };
  void SetYieldError           (const Double_t in) { fYieldError=in; };
  void SetYieldWidth           (const Double_t in) { fYieldWidth=in; };
  void SetYieldUnit            (const TString  in) { fYieldUnit=in; };

  // Asymmetry :
  void SetAsymmetry           (const Double_t in) { fAsymDiff=in; };
  void SetAsymmetryError      (const Double_t in) { fAsymDiffError=in; };
  void SetAsymmetryWidth      (const Double_t in) { fAsymDiffWidth=in; };
  void SetAsymmetryUnit       (const TString  in) { fAsymDiffUnit=in; };


  // Difference :
  void SetDifference           (const Double_t in) { fAsymDiff=in; };
  void SetDifferenceError      (const Double_t in) { fAsymDiffError=in; };
  void SetDifferenceWidth      (const Double_t in) { fAsymDiffWidth=in; };
  void SetDifferenceUnit       (const TString  in) { fAsymDiffUnit=in; };

  // Yield
  Double_t GetNumGoodEvents ()    {
    //  Returns the number of entries corresponding to the asymmetry error/width ratio
    if(fAsymDiffError!=0){
      Double_t temp = (fAsymDiffWidth/fAsymDiffError);
      return temp*temp;
    }else{
      return 0;
    }
  };

  Double_t GetYield         () { return fYield; };
  Double_t GetYieldError    () { return fYieldError; };
  Double_t GetYieldWidth    () { return fYieldWidth; };
  TString  GetYieldUnit     () { return  fYieldUnit; };

  // Asymmetry :
  Double_t GetAsymmetry     () { return fAsymDiff; };
  Double_t GetAsymmetryError() { return fAsymDiffError; };
  Double_t GetAsymmetryWidth() { return fAsymDiffWidth; };
  TString  GetAsymmetryUnit () { return fAsymDiffUnit; };


  // Difference :
  Double_t GetDifference     () { return fAsymDiff; };
  Double_t GetDifferenceError() { return fAsymDiffError; };
  Double_t GetDifferenceWidth() { return fAsymDiffWidth; };
  TString  GetDifferenceUnit () { return fAsymDiffUnit; };

  void Set(TString type, const Double_t a, const Double_t a_err, const Double_t a_width);

  //  void SetAsymmetryWidthError (const Double_t in) { fAsymmetryWidthError=in; };
  // void SetAsymmetryWidthUnit  (const TString  in) { fAsymmetryWidthUnit=in; };


  TString GetCSVSummary(TString type);
  TString GetTextSummary();
  //

  // This is not sigma, but error defined in QwVQWK_Channel::CalculateRunningAverage() in QwVQWK_Channel.cc as follows
  // fHardwareBlockSumError = sqrt(fHardwareBlockSumM2) / fGoodEventCount;
  //
 private:

  TString fElementName;

  Double_t fNumGoodEvents;
  Double_t fYield;
  Double_t fYieldError;
  Double_t fYieldWidth;
  TString  fYieldUnit;

  Double_t fAsymDiff;
  Double_t fAsymDiffError;
  Double_t fAsymDiffWidth;
  TString  fAsymDiffUnit;

  /* Double_t fAsymmetryWidth; */
  /* Double_t fAsymmetryWidthError; */
  /* TString  fAsymmetryWidthUnit; */

  ClassDefOverride(PromptSummaryElement,0);

};


class QwPromptSummary  :  public TObject
{

 public:
  QwPromptSummary();
  QwPromptSummary(Int_t run_number, Int_t runlet_number);
  QwPromptSummary(Int_t run_number, Int_t runlet_number, const std::string& parameter_file);
  virtual ~QwPromptSummary();

  std::map<TString, PromptSummaryElement*> fElementList;
  PromptSummaryElement* fReferenceElement{nullptr};

  void SetRunNumber(const Int_t in) {fRunNumber = in;};
  Int_t GetRunNumber() {return fRunNumber;};

  void SetRunletNumber(const Int_t in) {fRunletNumber = in;};
  Int_t GetRunletNumber() {return fRunletNumber;};

  void SetPatternSize(const Int_t in) { fPatternSize=in; };
  Int_t GetPatternSize() { return fPatternSize; };

  void AddElement(PromptSummaryElement *in);
  PromptSummaryElement* GetElementByName(TString name);

  void FillDataInElement(TString name,
			 Double_t yield, Double_t yield_err, Double_t yield_width, TString yield_unit,
			 Double_t asym_diff, Double_t asym_diff_err, Double_t asym_diff_width, TString asym_diff_unit);

  void FillYieldToElement(TString name, Double_t yield, Double_t yield_error, Double_t yield_width, TString yield_unit);
  void FillAsymDiffToElement(TString name, Double_t asym_diff, Double_t asym_diff_err, Double_t asym_diff_width, TString asym_diff_unit);
  //  void FillDifferenceToElement(Double_t asym_diff, Double_t asym_diff_err, TString asym_diff_unit);

  //  void Print(const Option_t* options = 0) const;

  void FillDoubleDifference(TString type, TString name1, TString name2);

  Int_t  GetSize()         const {return fElementList.size();};
  Int_t  Size()            const {return fElementList.size();};
  Int_t  HowManyElements() const {return fElementList.size();};


  void PrintCSV(Int_t nEvents, TString start_time, TString end_time);
  void PrintTextSummary();

private:


  Int_t fPatternSize;

  TString PrintTextSummaryHeader();
  TString PrintTextSummaryTailer();
  TString PrintCSVHeader(Int_t nEvents, TString start_time, TString end_time);

  void    SetupElementList();
  void    LoadElementsFromParameterFile(const std::string& parameter_file);
  void    LoadElementsFromParameterFile(QwParameterFile& parameterfile);

  Int_t   fRunNumber;
  Int_t   fRunletNumber;

  Bool_t  fLocalDebug;

  ClassDefOverride(QwPromptSummary,0);

};
