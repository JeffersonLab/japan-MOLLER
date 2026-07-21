#ifndef __QWSCANNER__
#define __QWSCANNER__

// System headers
#include <vector>

// ROOT headers
#include "TTree.h"
#include "TString.h"
#include "TFile.h"
#include "TH2.h"
#include "TH2F.h"


// Qweak headers
#include "VQwSubsystemParity.h"
#include "QwIntegrationPMT.h"

enum EQwScannerType {kScannerUnknown, kScannerPosition, kScannerReference, kScannerPMT, kNumScannerTypes};

class QwScannerID {

 public:

    QwScannerID():fSubbankIndex(-1),fWordInSubbank(-1),
     fTypeID(kScannerUnknown),fIndex(-1),
     fSubelement(kInvalidSubelementIndex),fmoduletype(""),fdetectorname("") {};

    int fSubbankIndex;
    int fWordInSubbank; //first word reported for this channel in the subbank
                        //(eg VQWK channel report 6 words for each event, scalers only report one word per event)
                        // The first word of the subbank gets fWordInSubbank=0

    EQwScannerType fTypeID;     // type of variable
    int fIndex;      // index of this variable in the vector containing all the variables of same type
    UInt_t fSubelement;

    TString fmoduletype; // eg: VQWK, SCALER
    TString fdetectorname;
    TString fdetectortype;

    void Print() const;
};

/*****************************************************************
*  Class: QwScanner
******************************************************************/

class QwScanner: public VQwSubsystemParity, public MQwSubsystemCloneable<QwScanner> {

  private:
    /// Private default constructor (not implemented, will throw linker error on use)
    QwScanner();

  public:

    // Constructor with name
    QwScanner(const TString& name)
      :VQwSubsystem(name),VQwSubsystemParity(name),bNormalization(kFALSE) {

        fTargetCharge.InitializeChannel("q_targ","derived");
        // define fixed voltages
        fPosRef.first.SetHardwareSum(3);
        fPosRef.second.SetHardwareSum(3);


    };

    // Copy constructor
    QwScanner(const QwScanner& source)
     :VQwSubsystem(source),VQwSubsystemParity(source),
     fScanner(source.fScanner),
     fPosVolt(source.fPosVolt),
     fPosVal(source.fPosVal),
     fPosRef(source.fPosRef),
     fPosFullScale(source.fPosFullScale),
     fPosOrigin(source.fPosOrigin),
     fScannerID(source.fScannerID) {};

    /// Destructor
    virtual ~QwScanner();

//    virtual void PublishInternalValues();

    // Handle command line options
    static void DefineOptions(QwOptions &options);
    void ProcessOptions(QwOptions &options);
    Int_t LoadChannelMap(TString mapfile);
    Int_t LoadInputParameters(TString pedestalfile);
    Int_t LoadMockRateMap(TString rootfilename);

    void  ClearEventData();

    Int_t ProcessConfigurationBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words);
    Int_t ProcessEvBuffer(const ROCID_t roc_id, const BankID_t bank_id, UInt_t* buffer, UInt_t num_words);
    void  ProcessEvent();


    // imlementation of scanner behavior
    void  SetRandomEventParameters(Double_t mean, Double_t sigma);
    void  SetRandomEventAsymmetry(Double_t asymmetry);
    void  RandomizeEventData(int helicity, Double_t time);
    void  EncodeEventData(std::vector<UInt_t> &buffer);
    void  RandomizeMollerEvent(int helicity);

    void  ConstructHistograms(TDirectory *folder, TString &prefix);
    void  FillHistograms();

    using VQwSubsystem::ConstructBranchAndVector;
    void  ConstructBranchAndVector(TTree *tree, TString &prefix, std::vector<Double_t> &values);
    void  ConstructBranch(TTree *tree, TString& prefix);
    void  ConstructBranch(TTree *tree, TString& prefix, QwParameterFile& trim_file) {};
    void  FillTreeVector(std::vector<Double_t> &values) const;

    Bool_t Compare(VQwSubsystem *source);

    VQwSubsystem&  operator=  (VQwSubsystem *value);
    VQwSubsystem&  operator+= (VQwSubsystem *value);
    VQwSubsystem&  operator-= (VQwSubsystem *value);
    void SetVoltPerHz(Double_t value) {fVoltPerHz = value;};
    void Ratio(VQwSubsystem *value1, VQwSubsystem  *value2);
    void Scale(Double_t factor);
    void AccumulateRunningSum(VQwSubsystem* value, Int_t count=0, Int_t ErrorMask=0xFFFFFFF);
    void FindMockPrincipleScanAxis(TString varprincipleaxis);
    //remove one entry from the running sums for devices
    void DeaccumulateRunningSum(VQwSubsystem* value, Int_t ErrorMask=0xFFFFFFF);
    void CalculateRunningAverage();

    const QwIntegrationPMT* GetChannel(const TString name) const;
    const QwIntegrationPMT* GetIntegrationPMT(const TString name) const;
//    const QwCombinedPMT* GetCombinedPMT(const TString name) const;

//    Int_t LoadEventCuts(TString filename);
//    Bool_t SingleEventCuts();
    Bool_t ApplySingleEventCuts();

    Bool_t CheckForBurpFail(const VQwSubsystem *subsys){
        //QwError << "************* test inside scanner *****************" << QwLog::endl;
        return kFALSE;
    };

    void IncrementErrorCounters();

    void PrintErrorCounters() const;
    UInt_t GetEventcutErrorFlag();
    //update the error flag in the subsystem level from the top level routines related to stability checks. This will uniquely update the errorflag at each channel based on the error flag in the corresponding channel in the ev_error subsystem
    void UpdateErrorFlag(const VQwSubsystem *ev_error);

    void LoadMockDataParameters(TString pedestalfile);
    Int_t LoadGeometryDefinition(TString mapfile);
    void PrintValue() const;
    void PrintInfo() const;
    void PrintDetectorID() const;

    Double_t* GetRawChannelArray();

    Double_t GetDataForChannelInModule(Int_t modnum, Int_t channum) {
      Int_t index = fModuleChannel_Map[std::pair<Int_t,Int_t>(modnum,channum)];
      return fScanner.at(index).GetValue();
    }

    Int_t GetChannelIndex(TString channelName, UInt_t module_number);

  private:

    // Number of good events
    Int_t fGoodEventCount;

    // Mapping from module and channel number to Scanner channel
    typedef std::map< std::pair<Int_t,Int_t>, Int_t > Module_Channel_to_Scanner_Map_t;
    Module_Channel_to_Scanner_Map_t fModuleChannel_Map;

    Double_t fPMTrate;
    Bool_t ldebug=kFALSE;
    Bool_t first_time = kTRUE;
    TString varname;
    Double_t varped;
    Double_t varcal;


    // mapfile variables
    TString localname;
    Int_t lineread=0;
    Double_t old_time;
    int num_y_pos = 0;
    int num_x_pos = 0;
    Bool_t vert = false;
    Double_t bin1;
    Double_t bin2;
    Double_t bin3;
    Double_t bin4;
    Double_t x_lin_val;
    Double_t y_lin_val;
    Double_t lin_val;
    int xbin;
    int ybin;
    Double_t x_pos = fxmin;
    Double_t y_pos = fymin;
    Double_t x_goal = fxmin;
    Double_t y_goal = fymin;
    Double_t mod_x_pos;
    Double_t mod_y_pos;
    int iteration = 1;

    std::vector <QwIntegrationPMT> fScanner; // Raw channels
    std::pair <QwMollerADC_Channel, QwMollerADC_Channel> fPosVolt; // Position voltage channel
    std::pair <QwMollerADC_Channel, QwMollerADC_Channel> fPosVal; // Position channel
    std::pair <QwMollerADC_Channel, QwMollerADC_Channel> fPosRef; // Reference Voltages for the Position Encoders
    std::pair <Double_t, Double_t> fPosFullScale; // width of scannable range (geometry range)
    std::pair <Double_t, Double_t> fPosOrigin; // minimum of scannable range


  protected:
    Bool_t fDEBUG;

    EQwScannerType GetScannerTypeID(TString name);
    Int_t GetScannerAxisID(TString name);
    Int_t GetScannerIndex(EQwScannerType TypeID, TString name);

    std::vector <QwScannerID> fScannerID;

    QwBeamCharge   fTargetCharge;
    QwBeamPosition fTargetX;
    QwBeamPosition fTargetY;
    QwBeamAngle    fTargetXprime;
    QwBeamAngle    fTargetYprime;
    QwBeamEnergy   fTargetEnergy;

    Bool_t bIsExchangedDataValid;

    Bool_t bNormalization;
    Double_t fNormThreshold;

  private:

    static const Bool_t bDEBUG=kFALSE;
    Int_t fMainDetErrorCount;

    // Mock Parameters for rates
    TH2F *fRateMap;
    TFile *fFileForHist;
    Int_t fMockPrincipleScanAxis; // 1 vertical 0 horizontal
    Double_t fScanSpeed; // how fast the scanner moves (mm/s)
    Double_t fMockSecondaryAxisStep; // distance traveled on the secondary direction after completing the scan along the primary (mm)

    // geometry (full range of scanner) limits
    Double_t gxmin;
    Double_t gxmax;
    Double_t gymin;
    Double_t gymax;
    // histogram limits
    Double_t hxmin;
    Double_t hxmax;
    Double_t hymin;
    Double_t hymax;
    // input scan limits
    Double_t sxmin;
    Double_t sxmax;
    Double_t symin;
    Double_t symax;
    // actual scan limits (limits that fall within all the previous three)
    Double_t fxmin;
    Double_t fxmax;
    Double_t fymin;
    Double_t fymax;

    Int_t fPrintFreq; // how many events occur between motion debugging readouts
    Double_t fOldTime;
    Double_t fVoltPerHz;
    Double_t o;
    Double_t z;
};

#endif
