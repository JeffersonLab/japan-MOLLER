//
// C++ Implementation: QwScanner
//
// Description: used to collect and process the information from the Scanner channel

#include "QwScanner.h"

// Qweak headers
#include "QwParameterFile.h"

// Register this subsystem with the factory
RegisterSubsystemFactory(QwScanner);


void QwScanner::DefineOptions(QwOptions &options)
{
  // Define command line options
}

void QwScanner::ProcessOptions(QwOptions &options)
{
  // Handle command line options
}

QwScanner::~QwScanner()
{
  fScanner.clear();
}

//***********************************************************************************************************//

Int_t QwScanner::LoadGeometryDefinition(TString mapfile)
{
  Bool_t ldebug=kFALSE;
  TString varname, varvalue;
  Double_t varxmin, varxmax, varymin, varymax;

  Int_t lineread=1;

  if(ldebug)std::cout<<"QwScanner::LoadGeometryParameters("<< mapfile<<")\n";

  QwParameterFile mapstr(mapfile.Data());  //Open the file
  fDetectorMaps.insert(mapstr.GetParamFileNameContents());
  while (mapstr.ReadNextLine()){
    lineread+=1;
    if(ldebug)std::cout<<" line read so far ="<<lineread<<"\n";
    mapstr.TrimComment('!');
    mapstr.TrimWhitespace();

    if (mapstr.LineIsEmpty())  continue;

    varname = mapstr.GetTypedNextToken<TString>();
    varname.ToLower();
    varname.Remove(TString::kBoth,' ');

    if (varname == "positionrange") {
      varxmin        = mapstr.GetTypedNextToken<Double_t>();
      varxmax        = mapstr.GetTypedNextToken<Double_t>();
      varymin        = mapstr.GetTypedNextToken<Double_t>();
      varymax        = mapstr.GetTypedNextToken<Double_t>();

      if (ldebug)
       std::cout << "Inputs for channel "      << varname        << ": ped="  << varped  << ": cal=" << varcal << "\n"
                 << ": Xmin=" << varxmin << ", Xmax=" << varxmax << "\n"
                 << ": Ymin=" << varymin << ", Ymax=" << varymax << "\n";

      gxmin                     = varxmin;
      gxmax                     = varxmax;
      gymin                     = varymin;
      gymax                     = varymax;
      fPosFullScale.first  = gxmax-gxmin;
      fPosFullScale.second = gymax-gymin;
      fPosOrigin.first     = gxmin;
      fPosOrigin.second    = gymin;

      if (ldebug) {
        std::cout << "fPosFullScale.first: " << fPosFullScale.first << std::endl;
        std::cout << "fPosFullScale.second: " << fPosFullScale.second << std::endl;
        std::cout << "fPosOrigin.first: " << fPosOrigin.first << std::endl;
        std::cout << "fPosOrigin.second: " << fPosOrigin.second << std::endl;
      }
    }
  }
  mapstr.Close(); // Close the file (ifstream)
  return 0;
}

Int_t QwScanner::LoadChannelMap(TString mapfile) {

    Bool_t ldebug=kFALSE;

    std::vector<TString> combinedchannelnames;
    std::vector<Double_t> weight;
    Int_t wordsofar=0;
    Int_t currentsubbankindex=-1;
    Int_t sample_size=0;
    Double_t abs_saturation_limit = 8.5; // default saturation limit(volt)
    Bool_t bAssignedLimit = kFALSE;

    // Open the file
    QwParameterFile mapstr(mapfile.Data());
    TString varname, varvalue;

    fDetectorMaps.insert(mapstr.GetParamFileNameContents());
    mapstr.EnableGreediness();
    mapstr.SetCommentChars("!");

    UInt_t value;
    size_t vqwk_buffer_offset = 0;

    while (mapstr.ReadNextLine()) {

        RegisterRocBankMarker(mapstr);
        if (mapstr.PopValue("abs_saturation_limit",value)) {
                abs_saturation_limit=value;
                bAssignedLimit = kTRUE;
        }

        if (mapstr.PopValue("sample_size",value)) {
                sample_size=value;
        }

        if (mapstr.PopValue("vqwk_buffer_offset",value)) {
            vqwk_buffer_offset=value;
        }

        mapstr.TrimComment('!');   // Remove everything after a '!' character.
        mapstr.TrimWhitespace();   // Get rid of leading and trailing spaces.

        if (mapstr.LineIsEmpty())  continue;

        Bool_t  lineok   = kTRUE;
        TString keyword  = "";
        TString keyword2 = "";
        TString modtype  = "";
        TString dettype  = "";
        TString namech   = "";
        Int_t modnum     = 0;
        Int_t channum    = 0;

        modtype = mapstr.GetTypedNextToken<TString>();      // module type

        modtype.ToUpper();

        if (modtype == "VPMT") {

            channum       = mapstr.GetTypedNextToken<Int_t>();  //channel number
            Int_t combinedchans = mapstr.GetTypedNextToken<Int_t>();    //number of combined channels
            dettype     = mapstr.GetTypedNextToken<TString>();  //type-purpose of the detector
            dettype.ToLower();
            namech      = mapstr.GetTypedNextToken<TString>();  //name of the detector
            namech.ToLower();
            combinedchannelnames.clear();

            for (int i=0; i<combinedchans; i++){

                TString nameofcombinedchan = mapstr.GetTypedNextToken<TString>();
                nameofcombinedchan.ToLower();
                combinedchannelnames.push_back(nameofcombinedchan);
            }

            weight.clear();

            for (int i=0; i<combinedchans; i++) {

                weight.push_back( mapstr.GetTypedNextToken<Double_t>());
            }

            keyword  = mapstr.GetTypedNextToken<TString>();
                keyword.ToLower();
                keyword2 = mapstr.GetTypedNextToken<TString>();
                keyword2.ToLower();

        } else {

            modnum    = mapstr.GetTypedNextToken<Int_t>();      //slot number
            channum   = mapstr.GetTypedNextToken<Int_t>();      //channel number
            dettype = mapstr.GetTypedNextToken<TString>();      //type-purpose of the detector
            dettype.ToLower();
            namech  = mapstr.GetTypedNextToken<TString>();  //name of the detector
            namech.ToLower();

                keyword   = mapstr.GetTypedNextToken<TString>();
                keyword.ToLower();
                keyword2  = mapstr.GetTypedNextToken<TString>();
                keyword2.ToLower();
        }


        if (currentsubbankindex!=GetSubbankIndex(fCurrentROC_ID,fCurrentBank_ID)) {

            currentsubbankindex=GetSubbankIndex(fCurrentROC_ID,fCurrentBank_ID);
            wordsofar=0;
        }

        QwScannerID localScannerID;
        localScannerID.fdetectorname=namech;
        localScannerID.fmoduletype=modtype;
        localScannerID.fSubbankIndex=currentsubbankindex;
        localScannerID.fdetectortype=dettype;
        localScannerID.fWordInSubbank=wordsofar;

        if (modtype=="MOLLERADC") {

                Int_t offset = QwMollerADC_Channel::GetBufferOffset(modnum, channum)+vqwk_buffer_offset;

                if (offset>=0){

                    localScannerID.fWordInSubbank = wordsofar + offset;
                }

        } else {

            QwError << "QwScanner::LoadChannelMap:  Unknown module type: "
                     << modtype <<", "<<namech<<" will not be decoded "
                     << QwLog::endl;
            lineok=kFALSE;
            continue;
        }

        localScannerID.fTypeID=GetScannerTypeID(dettype);

        if (localScannerID.fTypeID==kScannerUnknown) {
            QwError << "QwScanner::LoadChannelMap:  Unknown detector type: "
                 << dettype <<", the detector "<<namech<<" will not be decoded "
                 << QwLog::endl;
            lineok=kFALSE;
            continue;
        }

        localScannerID.fIndex=GetScannerIndex(localScannerID.fTypeID, localScannerID.fdetectorname);

        if (localScannerID.fIndex==-1){

            if (localScannerID.fTypeID==kScannerPMT){

                QwIntegrationPMT localIntegrationPMT(GetName(),localScannerID.fdetectorname);

                fScanner.push_back(localIntegrationPMT);
                fScanner[fScanner.size()-1].SetDefaultSampleSize(sample_size);
		fScanner[fScanner.size()-1].SetCalibrationFactor(3.05175e-5);

                if(bAssignedLimit) {
                  fScanner[fScanner.size()-1].SetSaturationLimit(abs_saturation_limit);
		}
                localScannerID.fIndex=fScanner.size()-1;

            } else if (localScannerID.fTypeID==kScannerPosition){

			Int_t axis = GetScannerAxisID(localScannerID.fdetectorname);
			if (axis==1) {
			  fPosVolt.first.InitializeChannel(GetName(),"",localScannerID.fdetectorname, "raw");
			  fPosVolt.first.SetDefaultSampleSize(sample_size);
	                  fPosVolt.first.SetCalibrationFactor(3.05175e-5);
                          if(bAssignedLimit)
                            fPosVolt.first.SetMollerADCSaturationLimt(abs_saturation_limit);
			} else if (axis==2) {
                          fPosVolt.second.InitializeChannel(GetName(),"",localScannerID.fdetectorname, "raw");
                          fPosVolt.second.SetDefaultSampleSize(sample_size);
                          fPosVolt.second.SetCalibrationFactor(3.05175e-5);
                          if(bAssignedLimit)
                            fPosVolt.second.SetMollerADCSaturationLimt(abs_saturation_limit);
                        }

                        localScannerID.fIndex=axis;
	    } else if (localScannerID.fTypeID==kScannerReference){

                        Int_t axis = GetScannerAxisID(localScannerID.fdetectorname);
                        if (axis==1) {
                          fPosRef.first.InitializeChannel(GetName(),"",localScannerID.fdetectorname, "raw");
                          fPosRef.first.SetDefaultSampleSize(sample_size);
                          fPosRef.first.SetCalibrationFactor(3.05175e-5);
                          if(bAssignedLimit)
                            fPosRef.first.SetMollerADCSaturationLimt(abs_saturation_limit);
                        } else if (axis==2) {
                          fPosRef.second.InitializeChannel(GetName(),"",localScannerID.fdetectorname, "raw");
                          fPosRef.second.SetDefaultSampleSize(sample_size);
                          fPosRef.second.SetCalibrationFactor(3.05175e-5);
                          if(bAssignedLimit)
                            fPosRef.second.SetMollerADCSaturationLimt(abs_saturation_limit);
                        }

                        localScannerID.fIndex=axis;
            }
        }

        if (ldebug) {

            localScannerID.Print();
            std::cout<<"line ok=";

            if (lineok)
             std::cout<<"TRUE"<<std::endl;

            else
             std::cout<<"FALSE"<<std::endl;
        }

        if (lineok)
         fScannerID.push_back(localScannerID);
    } // End of "while (mapstr.ReadNextLine())"

    fPosVal.first.InitializeChannel(GetName(),"","posvalx", "derived");
    fPosVal.first.SetDefaultSampleSize(sample_size);
    fPosVal.second.InitializeChannel(GetName(),"","posvaly", "derived");
    fPosVal.second.SetDefaultSampleSize(sample_size);


    // Now load the variables to publish
    mapstr.RewindToFileStart();
    QwParameterFile *section;
    std::vector<TString> publishinfo;
    while ((section = mapstr.ReadNextSection(varvalue))) {
        if (varvalue == "PUBLISH") {
            fPublishList.clear();

            while (section->ReadNextLine()) {
                section->TrimComment(); // Remove everything after a comment character
                section->TrimWhitespace(); // Get rid of leading and trailing spaces

                for (int ii = 0; ii < 4; ii++) {
                    varvalue = section->GetNextToken().c_str();

                    if (varvalue.Length()) {
                        publishinfo.push_back(varvalue);
                    }
                }
                if (publishinfo.size() == 4)
                 fPublishList.push_back(publishinfo);

                publishinfo.clear();
            }
        }
    }

    // Print list of variables to publish
    if (fPublishList.size()>0){

        QwMessage << "Variables to publish:" << QwLog::endl;

        for (size_t jj = 0; jj < fPublishList.size(); jj++)
         QwMessage << fPublishList.at(jj).at(0) << " " << fPublishList.at(jj).at(1) << " "
                  << fPublishList.at(jj).at(2) << " " << fPublishList.at(jj).at(3) << QwLog::endl;
    }

    if (ldebug) {

        std::cout<<"Done with Load channel map\n";

        for (size_t i=0;i<fScannerID.size();i++)
         if (fScannerID[i].fIndex>=0)
          fScannerID[i].Print();
    }

    ldebug=kFALSE;
    mapstr.Close(); // Close the file (ifstream)

    return 0;
}

//***********************************************************************************************************//
Int_t QwScanner::LoadInputParameters(TString pedestalfile)
{
    return 0;
}

void QwScanner::LoadMockDataParameters(TString pedestalfile) {

    Bool_t ldebug=kFALSE;
    TString varname, varvalue;
    TString vartype;

    Double_t value;
    TString varprincipleaxis;
    TString localname;

    TString varrootfile, varhistpath, varhistname;

    Int_t lineread=0;

    QwParameterFile mapstr(pedestalfile.Data());  //Open the file
    fDetectorMaps.insert(mapstr.GetParamFileNameContents());
    mapstr.EnableGreediness();
    mapstr.SetCommentChars("!");

    while (mapstr.ReadNextLine()) {

        lineread+=1;
        if (ldebug)std::cout<<" line read so far ="<<lineread<<"\n";
         mapstr.TrimComment('!');   // Remove everything after a '!' character.

        mapstr.TrimWhitespace();   // Get rid of leading and trailing spaces.
	vartype = mapstr.GetTypedNextToken<TString>();
        if (mapstr.LineIsEmpty())  continue;
        if (vartype == "positionreference") {
            varname  = mapstr.GetTypedNextToken<TString>();  //name of the channel
            varname.ToLower();
            varname.Remove(TString::kBoth,' ');
            if (GetScannerAxisID(varname)==1) {
              fPosRef.first.LoadMockDataParameters(mapstr);
            } else if (GetScannerAxisID(varname)==2) {
              fPosRef.second.LoadMockDataParameters(mapstr);
            }
        } else if (vartype == "range") {
            sxmin          = mapstr.GetTypedNextToken<Double_t>();
            sxmax          = mapstr.GetTypedNextToken<Double_t>();
            symin          = mapstr.GetTypedNextToken<Double_t>();
            symax          = mapstr.GetTypedNextToken<Double_t>();
            if (ldebug)
             std::cout << "Inputs for range \n"
                       << ": xmin="         << sxmin << ", xmax=" << sxmax << "\n"
                       << ": ymin="         << symin << ", ymax=" << symax << "\n";
        } else if (vartype == "histogram") {
            varrootfile = mapstr.GetTypedNextToken<TString>();
            varhistpath = mapstr.GetTypedNextToken<TString>();
            varhistname = mapstr.GetTypedNextToken<TString>();
            if (ldebug)
             std::cout << "Inputs for histogram \n"
                       << ": Root File="         << varrootfile << "\n"
                       << ": Path to Histogram=" << varhistpath << "\n"
                       << ": Histogram Name="    << varhistname << "\n";
        } else std::cout << "ERROR: Unknown Scanner Type" << std::endl;
    }

    if (mapstr.PopValue("voltperhz",value)) {
        SetVoltPerHz(value);
	if (ldebug) {
	    std::cout << "VoltPerHz= " << fVoltPerHz << std::endl;
            std::cout << "Value= " << value << std::endl;
	}
    }
    if (mapstr.PopValue("scanspeed",value)) {
	fScanSpeed = value;
        if (ldebug) {
            std::cout << "fScanSpeed= " << fScanSpeed << std::endl;
            std::cout << "Value= " << value << std::endl;
        }
    }
    if (mapstr.PopValue("principleaxis",varprincipleaxis)) {
        FindMockPrincipleScanAxis(varprincipleaxis);
        if (ldebug) {
            std::cout << "fMockPrincipleScanAxis= " << fMockPrincipleScanAxis << std::endl;
            std::cout << "Value= " << varprincipleaxis << std::endl;
        }
    }
    if (mapstr.PopValue("secaxisstep",value)) {
        fMockSecondaryAxisStep = value;
        if (ldebug) {
            std::cout << "fMockSecondaryAxisStep= " << fMockSecondaryAxisStep << std::endl;
            std::cout << "Value= " << value << std::endl;
        }
    }
    if (mapstr.PopValue("printprescale",value)) {
        fPrintFreq = value;
        if (ldebug) {
            std::cout << "fPrintFreq= " << fPrintFreq << std::endl;
            std::cout << "Value= " << value << std::endl;
        }
    }

    fFileForHist = TFile::Open(varrootfile);
    if (fFileForHist->IsZombie()) {
        QwError << "could not open root file" << QwLog::endl;
    }
    fRateMap = (TH2F*)fFileForHist->Get(varhistpath+"/"+varhistname);
    if (!fRateMap) {
        QwError << "histogram not found" << QwLog::endl;
        fFileForHist->ls();
    }

    if (ldebug)
     std::cout<<" line read in the pedestal + cal file ="<<lineread<<" \n";

    hxmin = fRateMap->GetXaxis()->GetXmin();
    hxmax = fRateMap->GetXaxis()->GetXmax();
    hymin = fRateMap->GetYaxis()->GetXmin();
    hymax = fRateMap->GetYaxis()->GetXmax();

    if (ldebug) {
      std::cout << "Histogram ranges: min (" << hxmin << "," << hymin << "), max (" << hxmax << "," << hymax << ")" << std::endl;
      std::cout << "Geometry ranges: min (" << gxmin << "," << gymin << "), max (" << gxmax << "," << gymax << ")" << std::endl;
      std::cout << "Scan ranges: min (" << sxmin << "," << symin << "), max (" << sxmax << "," << symax << ")" << std::endl;
    }
    if (gxmin > hxmax || gxmax < hxmin || gymin > hymax || gymax < hymin) {
      QwError << "Geometry does not touch histogram" << QwLog::endl;
      std::cout << "Desired geometry range: xmin = " << gxmin << ", ymin = " << gymin << ", xmax = " << gxmax << ", ymax = " << gymax << std::endl;
      std::cout << "Histogram range: xmin = " << hxmin << ", ymin = " << hymin << ", xmax = " << hxmax << ", ymax = " << hymax << std::endl;
      exit(1);
    } else if (gxmin < hxmin || gxmax > hxmax || gymin < hymin || gymax > hymax) {
      QwWarning << "Geometry extends outside histogram" << QwLog::endl;
      std::cout << "Desired geometry range: xmin = " << gxmin << ", ymin = " << gymin << ", xmax = " << gxmax << ", ymax = " << gymax << std::endl;
      std::cout << "Histogram range: xmin = " << hxmin << ", ymin = " << hymin << ", xmax = " << hxmax << ", ymax = " << hymax << std::endl;
    }
    if (hxmin > gxmin) fxmin = hxmin;
    else fxmin = gxmin;
    if (hxmax < gxmax) fxmax = hxmax;
    else fxmax = gxmax;
    if (hymin > gymin) fymin = hymin;
    else fymin = gymin;
    if (hymax < gymax) fymax = hymax;
    else fymax = gymax;

    if (sxmin > fxmax || sxmax < fxmin || symin > fymax || symax < fymin) {
      QwError << "Desired scan area does not touch acceptable area" << QwLog::endl;
      std::cout << "Desired scan range: xmin = " << sxmin << ", ymin = " << symin << ", xmax = " << sxmax << ", ymax = " << symax << std::endl;
      std::cout << "Acceptable range: xmin = " << fxmin << ", ymin = " << fymin << ", xmax = " << fxmax << ", ymax = " << fymax << std::endl;
      exit(1);
    } else if (sxmin < fxmin || sxmax > fxmax || symin < fymin || symax > fymax) {
      QwWarning << "Desired scan area extends outside acceptable area" << QwLog::endl;
      std::cout << "Desired scan range: xmin = " << sxmin << ", ymin = " << symin << ", xmax = " << sxmax << ", ymax = " << symax << std::endl;
      std::cout << "Acceptable range: xmin = " << fxmin << ", ymin = " << fymin << ", xmax = " << fxmax << ", ymax = " << fymax << std::endl;
    }
    if (sxmin > fxmin) fxmin = sxmin;
    if (sxmax < fxmax) fxmax = sxmax;
    if (symin > fymin) fymin = symin;
    if (symax < fymax) fymax = symax;

    if (ldebug)
      std::cout << "Final ranges: min (" << fxmin << "," << fymin << "), max (" << fxmax << "," << fymax << ")" << std::endl;

    ldebug=kFALSE;
    mapstr.Close(); // Close the file (ifstream)

}

void  QwScanner::RandomizeEventData(int helicity = 0, Double_t time = 1.0)
{
  Bool_t ldebug=kFALSE;
  Bool_t kdebug=kTRUE;
  Double_t timestep = 0.0;
  Double_t step = 0.0;

  if (first_time == kTRUE) {
    x_pos = fxmin;
    y_pos = fymin;
    old_time = time;
    first_time = kFALSE;
  } else {
    timestep = (time - old_time)*1e-3;
    step = timestep*fScanSpeed;
    old_time = time;
  }

  if (fMockPrincipleScanAxis == 0) {
    if (vert == true && (y_pos+step) > fymax) {
      y_goal = fymin;
      y_pos = fymin;
      num_y_pos = 0;
      x_pos = fxmin;
      vert = false;
      if (kdebug)
        std::cout << "Reset @ i: " << iteration << std::endl;
    }
    if (num_y_pos % 2 == 0) {
      if (vert == false && (x_pos+step) < fxmax) x_pos += step;
      else if (vert == false) {
        vert = true;
        y_goal = y_pos + fMockSecondaryAxisStep;
        y_pos += step-(fxmax-x_pos);
        x_pos = fxmax;
      } else if (vert == true && y_pos < (y_goal-step)) {
        y_pos += step;
      } else if (vert == true) {
        vert = false;
        num_y_pos += 1;
        x_pos -= step-(y_goal-y_pos);
        y_pos = y_goal;
      }
    }
    else if (num_y_pos % 2 == 1) {
      if (vert == false && (x_pos-step) > fxmin) x_pos -= step;
      else if (vert == false) {
        vert = true;
        y_goal = y_pos + fMockSecondaryAxisStep;
        y_pos += step-(x_pos-fxmin);
        x_pos = fxmin;
      } else if (vert == true && y_pos < (y_goal-step)) {
        y_pos += step;
      } else if (vert == true) {
        vert = false;
        num_y_pos += 1;
        x_pos += step-(y_goal-y_pos);
        y_pos = y_goal;
      }
    }
  } else if (fMockPrincipleScanAxis == 1) {
    if (vert == true && (x_pos+step) > fxmax) {
      x_goal = fxmin;
      x_pos = fxmin;
      num_x_pos = 0;
      y_pos = fymin;
      vert = false;
      if (kdebug)
        std::cout << "Reset @ i:" << iteration << std::endl;
    }
    if (num_x_pos % 2 == 0) {
      if (vert == false && (y_pos+step) < fymax) y_pos += step;
      else if (vert == false) {
        vert = true;
        x_goal = x_pos + fMockSecondaryAxisStep;
        x_pos += step-(fymax-y_pos);
        y_pos = fymax;
      } else if (vert == true && x_pos < (x_goal-step)) {
        x_pos += step;
      } else if (vert == true) {
        vert = false;
        num_x_pos += 1;
        y_pos -= step-(x_goal-x_pos);
        x_pos = x_goal;
      }
    }
    else if (num_x_pos % 2 == 1) {
      if (vert == false && (y_pos-step) > fymin) y_pos -= step;
      else if (vert == false) {
        vert = true;
        x_goal = x_pos + fMockSecondaryAxisStep;
        x_pos += step-(y_pos-fymin);
        y_pos = fymin;
      } else if (vert == true && x_pos < (x_goal-step)) {
        x_pos += step;
      } else if (vert == true) {
        vert = false;
        num_x_pos += 1;
        y_pos += step-(x_goal-x_pos);
        x_pos = x_goal;
      }
    }
  }
  xbin = fRateMap->GetXaxis()->FindFixBin(x_pos);
  ybin = fRateMap->GetYaxis()->FindFixBin(y_pos);

  Double_t bin1_x_center = fRateMap->GetXaxis()->GetBinCenter(xbin);
  Double_t bin1_y_center = fRateMap->GetYaxis()->GetBinCenter(ybin);
  Double_t x_width = fRateMap->GetXaxis()->GetBinWidth(xbin);
  Double_t y_width = fRateMap->GetYaxis()->GetBinWidth(ybin);
  mod_x_pos = (x_pos - bin1_x_center)/x_width;
  mod_y_pos = (y_pos - bin1_y_center)/y_width;
  bin1 = fRateMap->GetBinContent(xbin, ybin);

  if (mod_x_pos > 0) {
    bin2 = fRateMap->GetBinContent(xbin+1, ybin);
    if (mod_y_pos > 0) {
      bin3 = fRateMap->GetBinContent(xbin, ybin+1);
      bin4 = fRateMap->GetBinContent(xbin+1, ybin+1);
    }
    else {
      bin3 = fRateMap->GetBinContent(xbin, ybin-1);
      bin4 = fRateMap->GetBinContent(xbin+1, ybin-1);
    }
  }
  else {
    bin2 = fRateMap->GetBinContent(xbin-1, ybin);
    if (mod_y_pos > 0) {
      bin3 = fRateMap->GetBinContent(xbin, ybin+1);
      bin4 = fRateMap->GetBinContent(xbin-1, ybin+1);
    }
    else {
      bin3 = fRateMap->GetBinContent(xbin, ybin-1);
      bin4 = fRateMap->GetBinContent(xbin-1, ybin-1);
    }
  }
  mod_x_pos = std::abs(mod_x_pos);
  mod_y_pos = std::abs(mod_y_pos);
//  x_lin_val = ((1-mod_x_pos) * bin1 + mod_x_pos * bin2)/2+((1-mod_x_pos) * bin3 + mod_x_pos * bin4)/2;
//  y_lin_val = ((1-mod_y_pos) * bin1 + mod_y_pos * bin3)/2+((1-mod_y_pos) * bin2 + mod_y_pos * bin4)/2;
  lin_val = (1-mod_x_pos) * (1-mod_y_pos) * bin1 + (mod_x_pos) * (1-mod_y_pos) * bin2 + (1-mod_x_pos) * (mod_y_pos) * bin3 + (mod_x_pos) * (mod_y_pos) * bin4;

//  if (iteration % fPrintFreq == 0 && (bin1 > 0 || bin2 > 0 || bin3 > 0 || bin4 > 0)) {
  if (fPrintFreq == 0) {}
  else if (iteration % fPrintFreq == 0) {
    std::cout << "i: " << iteration << std::endl;
    std::cout << "  bin1 value: " << bin1 << ",     bin2 value: " << bin2 << std::endl;
    std::cout << "  bin3 value: " << bin3 << ",     bin4 value: " << bin4 << std::endl;
//    std::cout << "  bin 1&3 x dist: " << mod_x_pos << "     bin 2&4 x dist: " << 1-mod_x_pos << std::endl;
//    std::cout << "  bin 1&2 y dist: " << mod_y_pos << "     bin 3&4 y dist: " << 1-mod_y_pos << std::endl;
    std::cout << "  bin 1 weight: " <<  (1-mod_x_pos) * (1-mod_y_pos) << "  bin 2 weight: " << (mod_x_pos) * (1-mod_y_pos) << std::endl;
    std::cout << "  bin 3 weight: " <<  (1-mod_x_pos) * (mod_y_pos) << "    bin 4 weight: " << (mod_x_pos) * (mod_y_pos) << std::endl;
    std::cout << "  interpolated value: " << lin_val << std::endl;
//    std::cout << "  x linear value: " << x_lin_val << ",    y linear value: " << y_lin_val << std::endl;
    std::cout << "  xbin (column) #: " << xbin << ",    ybin (row) #: " << ybin << std::endl;
//    std::cout << "  x bin center: " << bin1_x_center << ",  y bin center: " << bin1_y_center << std::endl;
//    std::cout << "  x bin width: " << x_width << ", y bin width: " << y_width << std::endl;
    std::cout << "  x_pos: " << x_pos << ", y_pos: " << y_pos << std::endl;
    std::cout << "  y_goal: " << y_goal << ", y_max: " << fymax << ", xmax: " << fxmax << std::endl;
  }

  iteration += 1;

  fPMTrate = lin_val/65.*1.0e9; // Interpolated value (measured in GHz at a beam current of 65 uA) in units of Hz/uA
  if (ldebug) {
    if (fPMTrate==0) {
      z += 1;
      std::cout << "Zero detected: " << z << "\n\n\n\n" << std::endl;
    }
    if (x_pos > fxmax || x_pos < fxmin || y_pos > fymax || y_pos < fymin) {
      o += 1;
      std::cout << "Out of Bounds: " << o << std::endl;
      std::cout << "lin_val: " << lin_val << std::endl;
      std::cout << "fPMTrate: " << fPMTrate << std::endl;
    }
  }

  bIsExchangedDataValid = kTRUE;
  if (1==1 || bNormalization) {
    if (RequestExternalValue("q_targ", &fTargetCharge)) {
      if (bDEBUG) {
        QwWarning << "QwScanner::ExchangeProcessedData Found "<<fTargetCharge.GetElementName()<< QwLog::endl;
        (dynamic_cast<QwMollerADC_Channel*>(&fTargetCharge))->PrintInfo();
      }
    } else {
      bIsExchangedDataValid = kFALSE;
      QwError << GetName() << " could not get external value for "
       << fTargetCharge.GetElementName() << QwLog::endl;
    }
  }

  Double_t MeanCounts = fPMTrate * timestep * fTargetCharge.GetValue();
  Double_t RMS_Counts = sqrt(MeanCounts);
  Double_t PMT_Signal = MeanCounts*fVoltPerHz/timestep;
  Double_t RMS_Signal = RMS_Counts*fVoltPerHz/timestep;

  for (size_t i = 0; i < fScannerID.size(); i++) {
    if (fScannerID.at(i).fTypeID == kScannerPMT) {
      fScanner.at(i).SetRandomEventParameters(PMT_Signal, RMS_Signal);
      fScanner.at(i).RandomizeEventData(helicity, time);
    } else if (fScannerID.at(i).fTypeID == kScannerPosition) {
    } else if (fScannerID.at(i).fTypeID == kScannerReference) {
      if (fScannerID.at(i).fIndex==1) {
        fPosRef.first.RandomizeEventData(helicity, time);
      } else if (fScannerID.at(i).fIndex==2) {
        fPosRef.second.RandomizeEventData(helicity, time);
      }
    }
  }

  fPosVal.first.ClearEventData();
  fPosVal.first.AddChannelOffset(x_pos);
  fPosVal.second.ClearEventData();
  fPosVal.second.AddChannelOffset(y_pos);

  if (ldebug) {
    std::cout << "lin_val: " << lin_val << std::endl;
    std::cout << "fVoltPerHz: " << fVoltPerHz << std::endl;
    std::cout << "fPMTrate: " << fPMTrate << std::endl;
    std::cout << "MeanCounts: " << MeanCounts << " +- " << RMS_Counts << std::endl;
    std::cout << "PMT_Signal: " << PMT_Signal << " +- " << RMS_Signal << std::endl;
  }

  // calculate position voltages
  (fPosVolt.first) = (fPosRef.first);
  (fPosVolt.first).Scale((x_pos - fPosOrigin.first)/(fPosFullScale.first));
  (fPosVolt.second) = (fPosRef.second);
  (fPosVolt.second).Scale((y_pos - fPosOrigin.second)/(fPosFullScale.second));

  if (ldebug) {
    std::cout << "fPosVolt.first: " << fPosVolt.first.GetValue() << std::endl;
    std::cout << "fPosVolt.second: " << fPosVolt.second.GetValue() << std::endl;
    std::cout << "fPosRef.first: " << fPosRef.first.GetValue() << std::endl;
    std::cout << "fPosRef.second: " << fPosRef.second.GetValue() << std::endl;
  }

}


void QwScanner::UpdateErrorFlag(VQwSubsystem const*) {}
void QwScanner::EncodeEventData(std::vector<UInt_t> &buffer) {

    std::vector<UInt_t> elements;
    elements.clear();

    // Get all buffers in the order they are defined in the map file
    for (size_t i = 0; i < fScannerID.size(); i++) {
        if (fScannerID.at(i).fTypeID == kScannerPMT) {
          fScanner[fScannerID.at(i).fIndex].EncodeEventData(elements);
	} else if (fScannerID.at(i).fTypeID == kScannerPosition) {
          if (fScannerID.at(i).fIndex==1) {
	    fPosVolt.first.SetRawEventData();
	    fPosVolt.first.EncodeEventData(elements);
	  } else if (fScannerID.at(i).fIndex==2) {
            fPosVolt.second.SetRawEventData();
            fPosVolt.second.EncodeEventData(elements);
          }
        } else if (fScannerID.at(i).fTypeID == kScannerReference) {
	  if (fScannerID.at(i).fIndex==1) {
            fPosRef.first.SetRawEventData();
            fPosRef.first.EncodeEventData(elements);
          } else if (fScannerID.at(i).fIndex==2) {
            fPosRef.second.SetRawEventData();
            fPosRef.second.EncodeEventData(elements);
          }
        }
    }

    // If there is element data, generate the subbank header
    std::vector<UInt_t> subbankheader;
    std::vector<UInt_t> rocheader;

    if (elements.size() > 0) {
        // Form CODA subbank header
        subbankheader.clear();
        subbankheader.push_back(elements.size() + 1);   // subbank size
        subbankheader.push_back((fCurrentBank_ID << 16) | (0x01 << 8) | (1 & 0xff));
        // subbank tag | subbank type | event number

        // Form CODA bank/roc header
        rocheader.clear();
        rocheader.push_back(subbankheader.size() + elements.size() + 1);        // bank/roc size
        rocheader.push_back((fCurrentROC_ID << 16) | (0x10 << 8) | (1 & 0xff));
        // bank tag == ROC | bank type | event number

        // Add bank header, subbank header and element data to output buffer
        buffer.insert(buffer.end(), rocheader.begin(), rocheader.end());
        buffer.insert(buffer.end(), subbankheader.begin(), subbankheader.end());
        buffer.insert(buffer.end(), elements.begin(), elements.end());
    }
}


Int_t QwScanner::ProcessConfigurationBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) {
    return 0;
}


Int_t QwScanner::ProcessEvBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words) {

    Bool_t lkDEBUG=kFALSE;

    Int_t index = GetSubbankIndex(roc_id,bank_id);

    if (index>=0 && num_words>0) {

        //  We want to process this ROC.  Begin looping through the data.
        if (lkDEBUG)
            std::cout << "QwScanner::ProcessEvBuffer:  "
             << "Begin processing ROC" << roc_id
             << " and subbank "<<bank_id
             << " number of words="<<num_words<<std::endl;


        for (size_t i=0;i<fScannerID.size();i++) {

            if (fScannerID[i].fSubbankIndex==index) {

                if (fScannerID[i].fTypeID == kScannerPMT) {

                    if (lkDEBUG) {
                        std::cout<<"found IntegrationPMT data for "<<fScannerID[i].fdetectorname<<std::endl;
                        std::cout<<"word left to read in this buffer:"<<num_words-fScannerID[i].fWordInSubbank<<std::endl;
                    }
                    fScanner[fScannerID.at(i).fIndex].ProcessEvBuffer(&(buffer[fScannerID.at(i).fWordInSubbank]), num_words-fScannerID.at(i).fWordInSubbank);
                } else if (fScannerID[i].fTypeID == kScannerPosition) {

                    if (lkDEBUG) {
                        std::cout<<"found IntegrationPMT data for "<<fScannerID[i].fdetectorname<<std::endl;
                        std::cout<<"word left to read in this buffer:"<<num_words-fScannerID[i].fWordInSubbank<<std::endl;
                    }

		    if (fScannerID[i].fIndex==1) {
		        fPosVolt.first.ProcessEvBuffer(&(buffer[fScannerID[i].fWordInSubbank]), num_words-fScannerID[i].fWordInSubbank);
		    } else if (fScannerID[i].fIndex==2) {
                        fPosVolt.second.ProcessEvBuffer(&(buffer[fScannerID[i].fWordInSubbank]), num_words-fScannerID[i].fWordInSubbank);
                    }
                } else if (fScannerID[i].fTypeID == kScannerReference) {

                    if (lkDEBUG) {
                        std::cout<<"found IntegrationPMT data for "<<fScannerID[i].fdetectorname<<std::endl;
                        std::cout<<"word left to read in this buffer:"<<num_words-fScannerID[i].fWordInSubbank<<std::endl;
                    }

                    if (fScannerID[i].fIndex==1) {
                        fPosRef.first.ProcessEvBuffer(&(buffer[fScannerID[i].fWordInSubbank]), num_words-fScannerID[i].fWordInSubbank);
                    } else if (fScannerID[i].fIndex==2) {
                        fPosRef.second.ProcessEvBuffer(&(buffer[fScannerID[i].fWordInSubbank]), num_words-fScannerID[i].fWordInSubbank);
                    }
                }
            }
        }
    }
    return 0;
}
EQwScannerType QwScanner::GetScannerTypeID(TString name) {

  //  The name is passed by value because we will change it to lower case.
  name.ToLower();
  EQwScannerType result;
  result = kScannerUnknown;
  if (name=="integrationpmt"){
    result = kScannerPMT;
  } else if (name=="position"){
    result = kScannerPosition;
  }
  else if (name=="posref"){
    result = kScannerReference;
  }
  return result;

}

Int_t QwScanner::GetScannerAxisID(TString name) {

  //  The name is passed by value because we will change it to lower case.
  name.ToLower();
  Int_t result = 0;
  if (name.EndsWith("x", TString::kExact)) {
    result = 1;
  } else if (name.EndsWith("y", TString::kExact)) {
    result = 2;
  }
  return result;
}

Int_t QwScanner::GetScannerIndex(EQwScannerType type_id, TString name) {
    Bool_t ldebug=kFALSE;

    if (ldebug) {

        std::cout<<"QwScanner::GetScannerIndex\n";
        std::cout<<"type_id=="<<type_id<<" name="<<name<<"\n";
        std::cout<<fScannerID.size()<<" already registered detector\n";
    }

    Int_t result=-1;
    for (size_t i=0;i<fScannerID.size();i++) {

        if (fScannerID[i].fTypeID==type_id)
         if (fScannerID[i].fdetectorname==name) {

            result=fScannerID[i].fIndex;

            if (ldebug)
             std::cout<<"testing against ("<<fScannerID[i].fTypeID
             <<","<<fScannerID[i].fdetectorname<<")=>"<<result<<"\n";

         }
    }

    return result;

}
void QwScanner::IncrementErrorCounters() {}
void QwScanner::Ratio(VQwSubsystem*, VQwSubsystem*) {}
Bool_t QwScanner::ApplySingleEventCuts()
{
	return true;
}

// Operator commands
VQwSubsystem& QwScanner::operator=(VQwSubsystem *value)
{
	if (Compare(value)) {
	        QwScanner* input = dynamic_cast<QwScanner*> (value);

	        for (size_t i=0;i<input->fScannerID.size();i++) {
		  if (fScannerID[i].fTypeID == kScannerPMT) {

	            this->fScanner[i]=input->fScanner[i];

		  } else if (fScannerID[i].fTypeID == kScannerPosition) {

                    if (fScannerID[i].fIndex==1) {
                      this->fPosVolt.first=input->fPosVolt.first;
                    } else if (fScannerID[i].fIndex==2) {
                      this->fPosVolt.second=input->fPosVolt.second;
		    }

                  } else if (fScannerID[i].fTypeID == kScannerReference) {

		    if (fScannerID[i].fIndex==1) {
                      this->fPosRef.first=input->fPosRef.first;
                    } else if (fScannerID[i].fIndex==2) {
                      this->fPosRef.second=input->fPosRef.second;
		    }

                  }
		}
		this->fPosVal.first=input->fPosVal.first;
	        this->fPosVal.second=input->fPosVal.second;
		this->fPosFullScale.first=input->fPosFullScale.first;
                this->fPosFullScale.second=input->fPosFullScale.second;
                this->fPosOrigin.first=input->fPosOrigin.first;
                this->fPosOrigin.second=input->fPosOrigin.second;
	}
        return *this;
}
VQwSubsystem& QwScanner::operator+=(VQwSubsystem *value)
{
        if (Compare(value)) {
                QwScanner* input = dynamic_cast<QwScanner*> (value);

                for (size_t i=0;i<input->fScannerID.size();i++) {
                  if (fScannerID[i].fTypeID == kScannerPMT) {

                    this->fScanner[i]+=input->fScanner[i];

                  } else if (fScannerID[i].fTypeID == kScannerPosition) {

                    if (fScannerID[i].fIndex==1) {
                      this->fPosVolt.first+=input->fPosVolt.first;
                    } else if (fScannerID[i].fIndex==2) {
                      this->fPosVolt.second+=input->fPosVolt.second;
                    }

                  } else if (fScannerID[i].fTypeID == kScannerReference) {

                    if (fScannerID[i].fIndex==1) {
                      this->fPosRef.first+=input->fPosRef.first;
                    } else if (fScannerID[i].fIndex==2) {
                      this->fPosRef.second+=input->fPosRef.second;
                    }

                  }
                }
	        this->fPosVal.first+=input->fPosVal.first;
        	this->fPosVal.second+=input->fPosVal.second;
	}
        return *this;
}
VQwSubsystem& QwScanner::operator-=(VQwSubsystem *value)
{
        if (Compare(value)) {
                QwScanner* input = dynamic_cast<QwScanner*> (value);

                for (size_t i=0;i<input->fScannerID.size();i++) {
                  if (fScannerID[i].fTypeID == kScannerPMT) {

                    this->fScanner[i]-=input->fScanner[i];

                  } else if (fScannerID[i].fTypeID == kScannerPosition) {

                    if (fScannerID[i].fIndex==1) {
                      this->fPosVolt.first-=input->fPosVolt.first;
                    } else if (fScannerID[i].fIndex==2) {
                      this->fPosVolt.second-=input->fPosVolt.second;
                    }

                  } else if (fScannerID[i].fTypeID == kScannerReference) {

                    if (fScannerID[i].fIndex==1) {
                      this->fPosRef.first-=input->fPosRef.first;
                    } else if (fScannerID[i].fIndex==2) {
                      this->fPosRef.second-=input->fPosRef.second;
                    }

                  }
                }
	        this->fPosVal.first-=input->fPosVal.first;
        	this->fPosVal.second-=input->fPosVal.second;
	}
        return *this;
}
void QwScanner::PrintErrorCounters() const {}
void QwScanner::DeaccumulateRunningSum(VQwSubsystem*, int) {}
UInt_t QwScanner::GetEventcutErrorFlag()
{
	return 0;
}
void QwScanner::Scale(double) {}
void QwScanner::CalculateRunningAverage() {}
void QwScanner::AccumulateRunningSum(VQwSubsystem*, int, int) {}

void QwScanner::FindMockPrincipleScanAxis(TString varprincipleaxis)
{
  if (varprincipleaxis == "y") fMockPrincipleScanAxis = 1;
  else if (varprincipleaxis == "x") fMockPrincipleScanAxis = 0;
  else {
    QwError << "fMockPrincipleScanAxis must be either x or y" << QwLog::endl;
    exit(1);
  }
}

/**
 * Clear the event data in this subsystem
 */
void QwScanner::ClearEventData()
{
  // Clear all Scanner channels
  for (size_t i = 0; i < fScannerID.size(); i++) {
    if (fScannerID[i].fTypeID == kScannerPMT) {
      fScanner.at(i).ClearEventData();
    } else if (fScannerID[i].fTypeID == kScannerPosition) {
      if (fScannerID[i].fIndex==1) {
	fPosVolt.first.ClearEventData();
      } else if (fScannerID[i].fIndex==2) {
        fPosVolt.second.ClearEventData();
      }
    } else if (fScannerID[i].fTypeID == kScannerReference) {
      if (fScannerID[i].fIndex==1) {
        fPosRef.first.ClearEventData();
      } else if (fScannerID[i].fIndex==2) {
        fPosRef.second.ClearEventData();
      }
    }
  }
  fPosVal.first.ClearEventData();
  fPosVal.second.ClearEventData();

  // Reset good event count
  fGoodEventCount = 0;
}

void QwScanner::ProcessEvent()
{
  for (size_t i = 0; i < fScannerID.size(); i++) {
    if (fScannerID[i].fTypeID == kScannerPMT) {
      fScanner.at(i).ProcessEvent();
    } else if (fScannerID[i].fTypeID == kScannerPosition) {
      if (fScannerID[i].fIndex==1) {
        fPosVolt.first.ProcessEvent();
      } else if (fScannerID[i].fIndex==2) {
        fPosVolt.second.ProcessEvent();
      }
    } else if (fScannerID[i].fTypeID == kScannerReference) {
      if (fScannerID[i].fIndex==1) {
        fPosRef.first.ProcessEvent();
      } else if (fScannerID[i].fIndex==2) {
        fPosRef.second.ProcessEvent();
      }
    }
  }
  fPosVal.first.Ratio(fPosVolt.first, fPosRef.first);
  fPosVal.first.Scale(fPosFullScale.first);
  fPosVal.first.AddChannelOffset(fPosOrigin.first);
  fPosVal.second.Ratio(fPosVolt.second, fPosRef.second);
  fPosVal.second.Scale(fPosFullScale.second);
  fPosVal.second.AddChannelOffset(fPosOrigin.second);
}


void QwScanner::ConstructHistograms(TDirectory* folder, TString& prefix)
{
  for (size_t i = 0; i < fScannerID.size(); i++) {
    if (fScannerID[i].fTypeID == kScannerPMT) {
      fScanner.at(i).ConstructHistograms(folder,prefix);
    } else if (fScannerID[i].fTypeID == kScannerPosition) {
      if (fScannerID[i].fIndex==1) {
        fPosVolt.first.ConstructHistograms(folder,prefix);
      } else if (fScannerID[i].fIndex==2) {
        fPosVolt.second.ConstructHistograms(folder,prefix);
      }
    } else if (fScannerID[i].fTypeID == kScannerReference) {
      if (fScannerID[i].fIndex==1) {
        fPosRef.first.ConstructHistograms(folder,prefix);
      } else if (fScannerID[i].fIndex==2) {
        fPosRef.second.ConstructHistograms(folder,prefix);
      }
    }
  }
  fPosVal.first.ConstructHistograms(folder, prefix);
  fPosVal.second.ConstructHistograms(folder, prefix);
}

void QwScanner::FillHistograms()
{
  for (size_t i = 0; i < fScannerID.size(); i++) {
    if (fScannerID[i].fTypeID == kScannerPMT) {
      fScanner.at(i).FillHistograms();
    } else if (fScannerID[i].fTypeID == kScannerPosition) {
      if (fScannerID[i].fIndex==1) {
        fPosVolt.first.FillHistograms();
      } else if (fScannerID[i].fIndex==2) {
        fPosVolt.second.FillHistograms();
      }
    } else if (fScannerID[i].fTypeID == kScannerReference) {
      if (fScannerID[i].fIndex==1) {
        fPosRef.first.FillHistograms();
      } else if (fScannerID[i].fIndex==2) {
        fPosRef.second.FillHistograms();
      }
    }
  }
  fPosVal.first.FillHistograms();
  fPosVal.second.FillHistograms();
}

void QwScanner::ConstructBranchAndVector(TTree *tree, TString & prefix, std::vector <Double_t> &values)
{
  for (size_t i = 0; i < fScannerID.size(); i++) {
    if (fScannerID[i].fTypeID == kScannerPMT) {
      fScanner.at(i).ConstructBranchAndVector(tree, prefix, values);
    } else if (fScannerID[i].fTypeID == kScannerPosition) {
      if (fScannerID[i].fIndex==1) {
        fPosVolt.first.ConstructBranchAndVector(tree, prefix, values);
      } else if (fScannerID[i].fIndex==2) {
        fPosVolt.second.ConstructBranchAndVector(tree, prefix, values);
      }
    } else if (fScannerID[i].fTypeID == kScannerReference) {
      if (fScannerID[i].fIndex==1) {
        fPosRef.first.ConstructBranchAndVector(tree, prefix, values);
      } else if (fScannerID[i].fIndex==2) {
        fPosRef.second.ConstructBranchAndVector(tree, prefix, values);
      }
    }
  }
  fPosVal.first.ConstructBranchAndVector(tree, prefix, values);
  fPosVal.second.ConstructBranchAndVector(tree, prefix, values);
}

void QwScanner::ConstructBranch(TTree *tree, TString & prefix)
{
  for (size_t i = 0; i < fScannerID.size(); i++) {
    if (fScannerID[i].fTypeID == kScannerPMT) {
      fScanner.at(i).ConstructBranch(tree, prefix);
    } else if (fScannerID[i].fTypeID == kScannerPosition) {
      if (fScannerID[i].fIndex==1) {
        fPosVolt.first.ConstructBranch(tree, prefix);
      } else if (fScannerID[i].fIndex==2) {
        fPosVolt.second.ConstructBranch(tree, prefix);
      }
    } else if (fScannerID[i].fTypeID == kScannerReference) {
      if (fScannerID[i].fIndex==1) {
        fPosRef.first.ConstructBranch(tree, prefix);
      } else if (fScannerID[i].fIndex==2) {
        fPosRef.second.ConstructBranch(tree, prefix);
      }
    }
  }

  fPosVal.first.ConstructBranch(tree, prefix);
  fPosVal.second.ConstructBranch(tree, prefix);

}

void QwScanner::FillTreeVector(std::vector<Double_t> &values) const
{
  for (size_t i = 0; i < fScannerID.size(); i++) {
    if (fScannerID[i].fTypeID == kScannerPMT) {
      fScanner.at(i).FillTreeVector(values);
    } else if (fScannerID[i].fTypeID == kScannerPosition) {
      if (fScannerID[i].fIndex==1) {
        fPosVolt.first.FillTreeVector(values);
      } else if (fScannerID[i].fIndex==2) {
        fPosVolt.second.FillTreeVector(values);
      }
    } else if (fScannerID[i].fTypeID == kScannerReference) {
      if (fScannerID[i].fIndex==1) {
        fPosRef.first.FillTreeVector(values);
      } else if (fScannerID[i].fIndex==2) {
        fPosRef.second.FillTreeVector(values);
      }
    }
  }
  fPosVal.first.FillTreeVector(values);
  fPosVal.second.FillTreeVector(values);
}

/**
 * Compare two Scanner objects
 * @param value Object to compare with
 * @return kTRUE if the object is equal
 */
Bool_t QwScanner::Compare(VQwSubsystem *value)
{
  // Immediately fail on null objects
  if (value == 0) return kFALSE;

  // Continue testing on actual object
  Bool_t result = kTRUE;
  if (typeid(*value) != typeid(*this)) {
    result = kFALSE;

  } else {
    QwScanner* input = dynamic_cast<QwScanner*> (value);
    if (input->fScanner.size() != fScanner.size()) {
      result = kFALSE;
    }
  }
  return result;
}

/**
 * Print some debugging output for the subcomponents
 */
void QwScanner::PrintInfo() const
{
  VQwSubsystemParity::PrintInfo();

  QwOut << " there are " << fScannerID.size() << " ScannerPMT \n" << QwLog::endl;
  for (size_t i = 0; i < fScannerID.size(); i++) {
    QwOut << " scanner " << i << ": ";
    fScanner.at(i).PrintInfo();
    fPosVolt.first.PrintInfo();
    fPosVolt.second.PrintInfo();
    fPosRef.first.PrintInfo();
    fPosRef.second.PrintInfo();
  }
  fPosVal.first.PrintInfo();
  fPosVal.second.PrintInfo();
}

/**
 * Print the value for the subcomponents
 */
void QwScanner::PrintValue() const
{
  QwMessage << "=== QwScanner: " << GetName() << " ===" << QwLog::endl;
  for(size_t i = 0; i < fScannerID.size(); i++) {
    fScanner.at(i).PrintValue();
    fPosVolt.first.PrintValue();
    fPosVolt.second.PrintValue();
    fPosRef.first.PrintValue();
    fPosRef.second.PrintValue();
  }
  fPosVal.first.PrintValue();
  fPosVal.second.PrintValue();
}

void  QwScannerID::Print() const
{
    std::cout<<std::endl<<"Variable name= "<<fdetectorname<<std::endl;
    std::cout<<"SubbankkIndex= "<<fSubbankIndex<<std::endl;
    std::cout<<"word index in subbank= "<<fWordInSubbank<<std::endl;
    std::cout<<"module type= "<<fmoduletype<<std::endl;
    std::cout<<"variable type= "<<fdetectortype<<"    index= "<<fTypeID<<std::endl;
    std::cout<<"Index of this variable in the vector of similar variables= "<<fIndex<<std::endl;
    std::cout<<"Subelement index= "<<fSubelement<<std::endl;
    std::cout<<"==========================================\n";
    return;
}

