///////////////////////////////////////////////////////////////////
//  Macro to help with online analysis
//    B. Moffit  Oct. 2003
///////////////////////////////////////////////////////////////////

#include "panguinOnline.hh"
#include <string>
#include <fstream>
#include <iostream>
#include <list>
#include <functional>
#include <cstdint>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctime>
#include <TMath.h>
#include <TBranch.h>
#include <TGClient.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TROOT.h>
#include <TGImageMap.h>
#include <TGFileDialog.h>
#include <TKey.h>
#include <TMemFile.h>
#include <TObjString.h>
#include <TSystem.h>
#include <TLatex.h>
#include <TText.h>
#include "TPaveText.h"
#include <TApplication.h>
#include "TEnv.h"
#include "TRegexp.h"
#include "TGraph.h"
#include "TProfile.h"
#include "THLimitsFinder.h"
#include <algorithm>
#include <cctype>
#include <cmath>

#define OLDTIMERUPDATE

using namespace std;

///////////////////////////////////////////////////////////////////
//  Class: OnlineGUI
//
//    Creates a GUI to display the commands used in OnlineConfig
//
//

OnlineGUI::OnlineGUI(OnlineConfig& config, Bool_t printonly=0, int ver=0):
  runNumber(0),
  timer(0),
  timerNow(0),
#ifdef QW_ENABLE_MAPFILE
  fMapFile(nullptr),
  fIsMapFile(kFALSE),
#endif
  fFileAlive(kFALSE),
  fVerbosity(ver)
{
  // Constructor.  Get the config pointer, and make the GUI.

  fConfig = &config;
  fTreeLineAtt = TAttLine(gStyle->GetHistLineColor(), gStyle->GetHistLineStyle(), gStyle->GetHistLineWidth());
  fTreeFillAtt = TAttFill(gStyle->GetHistFillColor(), gStyle->GetHistFillStyle());
  fTreeMarkerAtt = TAttMarker(gStyle->GetMarkerColor(), gStyle->GetMarkerStyle(), gStyle->GetMarkerSize());
  int bin2Dx(0), bin2Dy(0);
  fConfig->Get2DnumberBins(bin2Dx,bin2Dy);
  if(bin2Dx>0 && bin2Dy>0){
    gEnv->SetValue("Hist.Binning.2D.x",bin2Dx);
    gEnv->SetValue("Hist.Binning.2D.y",bin2Dy);
    if(fVerbosity>1){
      cout<<"Set 2D default bins to x, y: "<<bin2Dx<<", "<<bin2Dy<<endl;
    }
  }

  if(printonly) {
    fPrintOnly=kTRUE;
    PrintPages();
  } else {
    fPrintOnly=kFALSE;
    CreateGUI(gClient->GetRoot(),800,600);
  }
}

void OnlineGUI::CreateGUI(const TGWindow *p, UInt_t w, UInt_t h)
{

  // Open the RootFile.  Die if it doesn't exist.
  //  unless we're watching a file.
#ifdef QW_ENABLE_MAPFILE
  fIsMapFile = LooksLikeMapFile(fConfig->GetRootFile());
  if(fIsMapFile) {
    fRootFile = NULL;
    fMapFile  = TMapFile::Create(fConfig->GetRootFile(), "READ");
    if(fMapFile == nullptr) {
      cout << "ERROR:  mapfile: " << fConfig->GetRootFile()
           << " could not be attached"
           << endl;
      if(fConfig->IsMonitor()) {
        cout << "Will wait... hopefully.." << endl;
      } else {
        gApplication->Terminate();
      }
    } else {
      fFileAlive = kTRUE;
      runNumber  = fConfig->GetRunNumber();
      // TMapFile hosts directly-stored objects: histograms and TTrees can be
      // read directly, while RNTuples require a serialized TMemFile snapshot.
      GetFileObjects();
      GetRootTree();
      GetTreeVars();
#ifdef HAS_RNTUPLE_SUPPORT
      if(LoadRNTupleSnapshot()) {
        GetRootNTuple();
        GetNTupleVars();
      }
#endif // HAS_RNTUPLE_SUPPORT
      for(UInt_t i=0; i<fRootTree.size(); i++) {
        if(fRootTree[i]==0) {
          fRootTree.erase(fRootTree.begin() + i);
        }
      }
    }
  } else {
    fRootFile = new TFile(fConfig->GetRootFile(),"READ");
    if(!fRootFile->IsOpen()) {
      cout << "ERROR:  rootfile: " << fConfig->GetRootFile()
           << " does not exist"
           << endl;
      if(fConfig->IsMonitor()) {
        cout << "Will wait... hopefully.." << endl;
      } else {
        gApplication->Terminate();
      }
    } else {
      fFileAlive = kTRUE;
      runNumber = fConfig->GetRunNumber();
      // Open the Root Trees.  Give a warning if it's not there..
      GetFileObjects();
      GetRootTree();
      GetTreeVars();
#ifdef HAS_RNTUPLE_SUPPORT
      // Initialize RNTuples
      GetRootNTuple();
      GetNTupleVars();
#endif // HAS_RNTUPLE_SUPPORT
      // Initialize DataFrame for large dataset support
      InitializeDataFrame();
      for(UInt_t i=0; i<fRootTree.size(); i++) {
        if(fRootTree[i]==0) {
          fRootTree.erase(fRootTree.begin() + i);
        }
      }
    }
  }
#else
  fRootFile = new TFile(fConfig->GetRootFile(),"READ");
  if(!fRootFile->IsOpen()) {
    cout << "ERROR:  rootfile: " << fConfig->GetRootFile()
	 << " does not exist"
	 << endl;
    if(fConfig->IsMonitor()) {
      cout << "Will wait... hopefully.." << endl;
    } else {
      gApplication->Terminate();
    }
  } else {
    fFileAlive = kTRUE;
    runNumber = fConfig->GetRunNumber();
    // Open the Root Trees.  Give a warning if it's not there..
    GetFileObjects();
    GetRootTree();
    GetTreeVars();
#ifdef HAS_RNTUPLE_SUPPORT
    // Initialize RNTuples
    GetRootNTuple();
    GetNTupleVars();
#endif // HAS_RNTUPLE_SUPPORT
    // Initialize DataFrame for large dataset support
    InitializeDataFrame();
    for(UInt_t i=0; i<fRootTree.size(); i++) {
      if(fRootTree[i]==0) {
	fRootTree.erase(fRootTree.begin() + i);
      }
    }

  }
#endif // QW_ENABLE_MAPFILE
  TString goldenfilename=fConfig->GetGoldenFile();
#ifdef QW_ENABLE_MAPFILE
  // Golden-file comparison requires "switching back" to the main file via
  // fRootFile->cd(), which is not meaningful with a TMapFile.  Refuse the
  // comparison and warn so the user knows the goldenrootfile is ignored.
  if(fIsMapFile && !goldenfilename.IsNull()) {
    cout << "NOTE:  goldenrootfile comparison is not supported in TMapFile "
            "mode; ignoring '" << goldenfilename << "'."
         << endl;
    goldenfilename = "";
  }
#endif
  if(!goldenfilename.IsNull()) {
    fGoldenFile = new TFile(goldenfilename,"READ");
    if(!fGoldenFile->IsOpen()) {
      cout << "ERROR: goldenrootfile: " << goldenfilename
	   << " does not exist.  Oh well, no comparison plots."
	   << endl;
      doGolden = kFALSE;
      fGoldenFile=NULL;
    } else {
      doGolden = kTRUE;
    }
  } else {
    doGolden=kFALSE;
    fGoldenFile=NULL;
  }


  // Create the main frame
  fMain = new TGMainFrame(p,w,h);
  fMain->Connect("CloseWindow()", "OnlineGUI", this, "MyCloseWindow()");
  ULong_t lightgreen, lightblue, red, mainguicolor;
  gClient->GetColorByName("lightgreen",lightgreen);
  gClient->GetColorByName("lightblue",lightblue);
  gClient->GetColorByName("red",red);

  Bool_t good_color=kFALSE;
  TString usercolor = fConfig->GetGuiColor();
  if(!usercolor.IsNull()) {
    good_color = gClient->GetColorByName(usercolor,mainguicolor);
  }

  if(!good_color) {
    if(!usercolor.IsNull()) {
      cout << "Bad guicolor (" << usercolor << ").. using default." << endl;
    }
    if(fConfig->IsMonitor()) {
      // Default background color for Online Monitor
      mainguicolor = lightgreen;
    } else {
      // Default background color for Normal Online Display
      mainguicolor = lightblue;
    }
  }

  fMain->SetBackgroundColor(mainguicolor);

  // Top frame, to hold page buttons and canvas
  fTopframe = new TGHorizontalFrame(fMain,w,UInt_t(h*0.9));
  fTopframe->SetBackgroundColor(mainguicolor);
  fMain->AddFrame(fTopframe, new TGLayoutHints(kLHintsExpandX
					       | kLHintsExpandY,10,10,10,1));

  // Create a verticle frame widget with radio buttons
  //  This will hold the page buttons
  vframe = new TGVerticalFrame(fTopframe,UInt_t(w*0.3),UInt_t(h*0.9));
  vframe->SetBackgroundColor(mainguicolor);
  TString buff;
  for(UInt_t i=0; i<fConfig->GetPageCount(); i++) {
    buff = fConfig->GetPageTitle(i);
    fRadioPage[i] = new TGRadioButton(vframe,buff,i);
    fRadioPage[i]->SetBackgroundColor(mainguicolor);
  }

  fRadioPage[0]->SetState(kButtonDown);
  current_page = 0;

  for (UInt_t i=0; i<fConfig->GetPageCount(); i++) {
    vframe->AddFrame(fRadioPage[i], new TGLayoutHints(kLHintsLeft |
						      kLHintsCenterY,5,5,3,4));
    fRadioPage[i]->Connect("Pressed()", "OnlineGUI", this, "DoRadio()");
  }

  // heartbeat below the picture, watchfile only
  if(fConfig->IsMonitor()){
    fRootFileLastUpdated = new TGTextButton(vframe, "File updated at: XX:XX:XX");
    // fRootFileLastUpdated->SetWidth(156);
    vframe->AddFrame(fRootFileLastUpdated, new TGLayoutHints(kLHintsBottom | kLHintsRight, 5, 5, 3, 4));

    fLastUpdated = new TGTextButton(vframe, "Plots updated at: XX:XX:XX");
    // fLastUpdated->SetWidth(156);
    vframe->AddFrame(fLastUpdated, new TGLayoutHints(kLHintsBottom | kLHintsRight, 5, 5, 3, 4));

    fNow = new TGTextButton(vframe, "Current time: XX:XX:XX");
    // fNow->SetWidth(156);
    vframe->AddFrame(fNow, new TGLayoutHints(kLHintsBottom | kLHintsRight, 5, 5, 3, 4));
  }

  if(!fConfig->IsMonitor()) {
    wile =
      new TGPictureButton(vframe,gClient->GetPicture(guiDirectory+"/genius.xpm"));
    wile->Connect("Pressed()","OnlineGUI", this,"DoDraw()");
  } else {
    wile =
      new TGPictureButton(vframe,gClient->GetPicture(guiDirectory+"/panguin.xpm"));
    wile->Connect("Pressed()","OnlineGUI", this,"DoDrawClear()");
  }
  wile->SetBackgroundColor(mainguicolor);

  vframe->AddFrame(wile,new TGLayoutHints(kLHintsBottom|kLHintsLeft,5,10,4,2));


  fTopframe->AddFrame(vframe,new TGLayoutHints(kLHintsLeft|
                                               kLHintsCenterY,2,2,2,2));

  // Create canvas widget
  fEcanvas = new TRootEmbeddedCanvas("Ecanvas", fTopframe, UInt_t(w*0.7), UInt_t(h*0.9));
  fEcanvas->SetBackgroundColor(mainguicolor);
  fTopframe->AddFrame(fEcanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY
						  ,10,10,10,1));
  fCanvas = fEcanvas->GetCanvas();

  // Create the bottom frame.  Contains control buttons
  fBottomFrame = new TGHorizontalFrame(fMain,w,UInt_t(h*0.1));
  fBottomFrame->SetBackgroundColor(mainguicolor);
  fMain->AddFrame(fBottomFrame, new TGLayoutHints(kLHintsExpandX,10,10,10,10));

  // Create a horizontal frame widget with buttons
  hframe = new TGHorizontalFrame(fBottomFrame,1200,40);
  hframe->SetBackgroundColor(mainguicolor);
  //fBottomFrame->AddFrame(hframe,new TGLayoutHints(kLHintsExpandX,200,20,2,2));
  fBottomFrame->AddFrame(hframe,new TGLayoutHints(kLHintsExpandX,2,2,2,2));

  fPrev = new TGTextButton(hframe,"Prev");
  fPrev->SetBackgroundColor(mainguicolor);
  fPrev->Connect("Clicked()","OnlineGUI",this,"DrawPrev()");
  hframe->AddFrame(fPrev, new TGLayoutHints(kLHintsCenterX,5,5,1,1));

  fNext = new TGTextButton(hframe,"Next");
  fNext->SetBackgroundColor(mainguicolor);
  fNext->Connect("Clicked()","OnlineGUI",this,"DrawNext()");
  hframe->AddFrame(fNext, new TGLayoutHints(kLHintsCenterX,5,5,1,1));

  fExit = new TGTextButton(hframe,"Exit GUI");
  fExit->SetBackgroundColor(red);
  fExit->Connect("Clicked()","OnlineGUI",this,"CloseGUI()");

  hframe->AddFrame(fExit, new TGLayoutHints(kLHintsCenterX,5,5,1,1));

  TString Buff;
  if(runNumber==0) {
    Buff = "";
  } else {
    Buff = "Run #";
    Buff += runNumber;
  }
  TGString labelBuff(Buff);

  fRunNumber = new TGLabel(hframe,Buff);
  fRunNumber->SetBackgroundColor(mainguicolor);
  hframe->AddFrame(fRunNumber,new TGLayoutHints(kLHintsCenterX,5,5,1,1));

  fPrint = new TGTextButton(hframe,"Print To &File");
  fPrint->SetBackgroundColor(mainguicolor);
  fPrint->Connect("Clicked()","OnlineGUI",this,"PrintToFile()");
  hframe->AddFrame(fPrint, new TGLayoutHints(kLHintsCenterX,5,5,1,1));


  // Set a name to the main frame
  if(fConfig->IsMonitor()) {
    fMain->SetWindowName("Parity ANalysis GUI moNitor");
  } else {
    fMain->SetWindowName("Online Analysis GUI");
  }

  // Map all sub windows to main frame
  fMain->MapSubwindows();

  // Initialize the layout algorithm
  fMain->Resize(fMain->GetDefaultSize());

  // Map main frame
  fMain->MapWindow();

  if(fVerbosity>=1)
    fMain->Print();

  if(fFileAlive) DoDraw();

  if(fConfig->IsMonitor()) {
    timerNow = new TTimer();
    timerNow->Connect(timerNow, "Timeout()", "OnlineGUI", this, "UpdateCurrentTime()");
    timerNow->Start(1000);  // update every second
  }

  if(fConfig->IsMonitor()) {
    timer = new TTimer();
    if(fFileAlive) {
      timer->Connect(timer,"Timeout()","OnlineGUI",this,"TimerUpdate()");
    } else {
      timer->Connect(timer,"Timeout()","OnlineGUI",this,"CheckRootFile()");
    }
    timer->Start(UPDATETIME);
  }

}

void OnlineGUI::DoDraw()
{
  // The main Drawing Routine.

  gStyle->SetOptStat(1110);
  gStyle->SetStatFontSize(0.1);
  if (fConfig->IsLogy(current_page)) {
    gStyle->SetOptLogy(1);
  } else {
    gStyle->SetOptLogy(0);
  }
  //   gStyle->SetTitleH(0.10);
  //   gStyle->SetTitleW(0.40);
  gStyle->SetTitleH(0.10);
  gStyle->SetTitleW(0.60);
  gStyle->SetStatH(0.70);
  gStyle->SetStatW(0.35);
  //   gStyle->SetLabelSize(0.10,"X");
  //   gStyle->SetLabelSize(0.10,"Y");
  gStyle->SetLabelSize(0.05,"X");
  gStyle->SetLabelSize(0.05,"Y");
  gStyle->SetPadLeftMargin(0.14);
  gStyle->SetNdivisions(505,"X");
  gStyle->SetNdivisions(404,"Y");
  gROOT->ForceStyle();

  // Determine the dimensions of the canvas..
  UInt_t draw_count = fConfig->GetDrawCount(current_page);
  if(draw_count>=8) {
    gStyle->SetLabelSize(0.08,"X");
    gStyle->SetLabelSize(0.08,"Y");
  }
  //   Int_t dim = Int_t(round(sqrt(double(draw_count))));
  pair <UInt_t,UInt_t> dim = fConfig->GetPageDim(current_page);

  if(fVerbosity>=1)
    cout << "Dimensions: " << dim.first << "X"
	 << dim.second << endl;

  // Create a nice clean canvas.
  fCanvas->Clear();
  fCanvas->Divide(dim.first,dim.second);

  vector <TString> drawcommand(5);
  // Draw the histograms.
  for(UInt_t i=0; i<draw_count; i++) {
    drawcommand = fConfig->GetDrawCommand(current_page,i);
    fCanvas->cd(i+1);
    if (drawcommand[0] == "macro") {
      MacroDraw(drawcommand);
    } else if (drawcommand[0] == "loadmacro") {
      LoadDraw(drawcommand);
    } else if (drawcommand[0] == "loadlib") {
      LoadLib(drawcommand);
    } else if (IsHistogram(drawcommand[0])) {
      HistDraw(drawcommand);
    } else {
      // Check if variable is in Trees or RNTuples
      UInt_t treeIndex = GetTreeIndex(drawcommand[0]);
#ifdef HAS_RNTUPLE_SUPPORT
      UInt_t ntupleIndex = FindNTupleForExpression(drawcommand[0], drawcommand[4]);
      // A file with RNTuples and no trees: let the RNTuple path report what is missing
      const Bool_t ntupleOnly = fRootTree.empty() && !fRootNTuple.empty();
#endif

      if (treeIndex < fRootTree.size()) {
        TreeDraw(drawcommand);
#ifdef HAS_RNTUPLE_SUPPORT
      } else if (ntupleIndex < fRootNTuple.size() || ntupleOnly) {
#ifdef QW_ENABLE_MAPFILE
        if(fIsMapFile) {
          if(fVerbosity>=2)
            cout << "Using NTupleDraw for mapfile RNTuple variable: "
                 << drawcommand[0] << endl;
          NTupleDraw(drawcommand);
        } else
#endif
        {
          // Use DataFrame for file-backed RNTuple variables.
          if(fVerbosity>=2)
            cout << "Using DataFrame for RNTuple variable: " << drawcommand[0] << endl;
          DataFrameDraw(drawcommand);
        }
#endif // HAS_RNTUPLE_SUPPORT
      } else {
        TreeDraw(drawcommand); // Fallback to TreeDraw for backwards compatibility
      }
    }
  }

  fCanvas->cd();
  fCanvas->Update();

  if(fConfig->IsMonitor()) {
    char buffer[9]; // HH:MM:SS
    time_t t = time(0);
    TString sLastUpdated("Plots updated at: ");
    strftime(buffer, 9, "%T", localtime(&t));
    sLastUpdated += buffer;
    fLastUpdated->SetText(sLastUpdated);

    struct stat result;
    stat(fConfig->GetRootFile().Data(), &result);
    time_t tf = result.st_mtime;
    strftime(buffer, 9, "%T", localtime(&tf));

    TString sRootFileLastUpdated("File updated at: ");
    sRootFileLastUpdated += buffer;
    fRootFileLastUpdated->SetText(sRootFileLastUpdated);
    ULong_t backgroundColor = fLastUpdated->GetBackground();
    if(fVerbosity>=4)
      cout<<"Updating plots (current, file, diff[s]):\t"<<t<<"\t"<<tf<<"\t"<<t - tf<<endl;
    if( t - tf > 60 ){
      ULong_t red;
      gClient->GetColorByName("red",red);
      fRootFileLastUpdated->SetBackgroundColor(red);
    }
    else{
      fRootFileLastUpdated->SetBackgroundColor(backgroundColor);
    }
  }

  if(!fPrintOnly) {
    CheckPageButtons();
  }

}

void OnlineGUI::DrawNext()
{
  // Handler for the "Next" button.
  fRadioPage[current_page]->SetState(kButtonUp);
  // The following line triggers DoRadio(), or at least.. used to
  fRadioPage[current_page+1]->SetState(kButtonDown);
  current_page++;
  DoDraw();
}

void OnlineGUI::DrawPrev()
{
  // Handler for the "Prev" button.
  fRadioPage[current_page]->SetState(kButtonUp);
  // The following line triggers DoRadio(), or at least.. used to
  fRadioPage[current_page-1]->SetState(kButtonDown);
  current_page--;
  DoDraw();
}

void OnlineGUI::DoRadio()
{
  // Handle the radio buttons
  //  Find out which button has been pressed..
  //   turn off the previous button...
  //   then draw the appropriate page.
  // This routine also handles the Draw from the Prev/Next buttons
  //   - makes a call to DoDraw()

  UInt_t pagecount = fConfig->GetPageCount();
  TGButton *btn = (TGButton *) gTQSender;
  UInt_t id = btn->WidgetId();

  if (id <= pagecount) {
    fRadioPage[current_page]->SetState(kButtonUp);
  }

  current_page = id;
  DoDraw();
}

void OnlineGUI::CheckPageButtons()
{
  // Checks the current page to see if it's the first or last page.
  //  If so... turn off the appropriate button.
  //  If not.. turn on both buttons.

  if(current_page==0) {
    fPrev->SetState(kButtonDisabled);
    if(fConfig->GetPageCount()!=1)
      fNext->SetState(kButtonUp);
  } else if(current_page==fConfig->GetPageCount()-1) {
    fNext->SetState(kButtonDisabled);
    if(fConfig->GetPageCount()!=1)
      fPrev->SetState(kButtonUp);
  } else {
    fPrev->SetState(kButtonUp);
    fNext->SetState(kButtonUp);
  }
}

Bool_t OnlineGUI::IsHistogram(TString objectname)
{
  // Utility to determine if the objectname provided is a histogram

  for(UInt_t i=0; i<fileObjects.size(); i++) {
    if (fileObjects[i].first.Contains(objectname)) {
      if(fVerbosity>=2)
	cout << fileObjects[i].first << "      "
	     << fileObjects[i].second << endl;

      // TProfile/TProfile2D derive from TH1D/TH2D but their class names do
      // not contain "TH", so match them explicitly.
      if(fileObjects[i].second.Contains("TH") ||
	 fileObjects[i].second.Contains("TProfile"))
	return kTRUE;
    }
  }

  return kFALSE;

}

#ifdef QW_ENABLE_MAPFILE
Bool_t OnlineGUI::LooksLikeMapFile(const TString& path)
{
  // A path is interpreted as a TMapFile when it ends in ".map" or
  // lives under /dev/shm/.  This keeps the .cfg syntax unchanged
  // ('rootfile <path>') and just routes the open by extension.
  if(path.EndsWith(".map")) return kTRUE;
  if(path.BeginsWith("/dev/shm/")) return kTRUE;
  return kFALSE;
}

TObject* OnlineGUI::GetObjectFromFile(const TString& name)
{
  // Fetch a named object from whichever input is currently attached.
  // For a TMapFile the returned object is a fresh clone owned by the
  // caller; for a TFile we delegate to gDirectory and the file owns it.
  if(fIsMapFile) {
    if(fMapFile == nullptr) return nullptr;
    return fMapFile->Get(name.Data());
  }
  return gDirectory->Get(name.Data());
}
#endif // QW_ENABLE_MAPFILE

void OnlineGUI::GetFileObjects()
{
  // Utility to find all of the objects within a File (TTree, TH1F, etc).
  //  The pair stored in the vector is <ObjName, ObjType>
  //  If there's no good keys.. do nothing.
#ifdef QW_ENABLE_MAPFILE
  if(fIsMapFile) {
    // A TMapFile is not a TDirectory and has no key dictionary; instead
    // it exposes a singly-linked list of TMapRec entries (name + class).
    fileObjects.clear();
    if(fMapFile == nullptr) {
      fUpdate = kFALSE;
      return;
    }
    TMapRec *rec = fMapFile->GetFirst();
    UInt_t nrec = 0;
    while(rec) {
      TString objname  = rec->GetName();
      TString objtype  = rec->GetClassName();
      // TMapRec::GetClassName() can return an empty string when the
      // producer hasn't registered the class with the consumer's
      // dictionary; in that case resolve it through the live object so
      // downstream type filtering (TH1, TH2, ...) keeps working.
      if(objtype.IsNull()) {
        TObject *obj = fMapFile->Get(objname.Data());
        if(obj) objtype = obj->ClassName();
      }
      if(fVerbosity>=1)
        cout << "MapRec = " << objname << " (" << objtype << ")" << endl;
      fileObjects.push_back(make_pair(objname, objtype));
      rec = rec->GetNext();
      ++nrec;
    }
    fUpdate = (nrec > 0);
    return;
  }
#endif // QW_ENABLE_MAPFILE
  if(fVerbosity>=1)
    cout << "Keys = " << fRootFile->ReadKeys() << endl;

  if(fRootFile->ReadKeys()==0) {
    fUpdate = kFALSE;
    //     delete fRootFile;
    //     fRootFile = 0;
    //     CheckRootFile();
    return;
  }
  fileObjects.clear();
  TIter next(fRootFile->GetListOfKeys());
  TKey *key = new TKey();

  // Do the search
  while((key=(TKey*)next())!=0) {
    if(fVerbosity>=1)
      cout << "Key = " << key << endl;

    TString objname = key->GetName();
    TString objtype = key->GetClassName();

    if(fVerbosity>=1)
      cout << objname << " " << objtype << endl;

    fileObjects.push_back(make_pair(objname,objtype));
  }
  fUpdate = kTRUE;
  delete key;
}

void OnlineGUI::GetTreeVars()
{
  // Utility to find all of the variables (leaf's/branches) within a
  // Specified TTree and put them within the treeVars vector.
  treeVars.clear();
  TObjArray *branchList;
  vector <TString> currentTree;

  for(UInt_t i=0; i<fRootTree.size(); i++) {
    currentTree.clear();
    branchList = fRootTree[i]->GetListOfBranches();
    TIter next(branchList);
    TBranch *brc;

    while((brc=(TBranch*)next())!=0) {
      TString found = brc->GetName();
      // Not sure if the line below is so smart...
      currentTree.push_back(found);
    }
    treeVars.push_back(currentTree);
  }

  if(fVerbosity>=5){
    for(UInt_t iTree=0; iTree<treeVars.size(); iTree++) {
      cout << "In Tree " << iTree << ": " << endl;
      for(UInt_t i=0; i<treeVars[iTree].size(); i++) {
	cout << treeVars[iTree][i] << endl;
      }
    }
  }
}


void OnlineGUI::GetRootTree() {
  // Utility to search a ROOT File for ROOT Trees
  // Fills the fRootTree vector
#ifdef QW_ENABLE_MAPFILE
  // In mapfile mode TMapFile::Get returns caller-owned clones; release the
  // previous batch before fetching a fresh snapshot, otherwise every refresh
  // would leak whole trees.
  if(fIsMapFile) {
    for(auto *t : fRootTree) delete t;
  }
#endif
  fRootTree.clear();

  list <TString> found;
  for(UInt_t i=0; i<fileObjects.size(); i++) {

    if(fVerbosity>=2)
      cout << "Object = " << fileObjects[i].second <<
	"     Name = " << fileObjects[i].first << endl;

    if(fileObjects[i].second.Contains("TTree"))
      found.push_back(fileObjects[i].first);
  }

  // Remove duplicates, then insert into fRootTree
  found.unique();
  UInt_t nTrees = found.size();

  for(UInt_t i=0; i<nTrees; i++) {
#ifdef QW_ENABLE_MAPFILE
    if(fIsMapFile) {
      TObject *o = fMapFile ? fMapFile->Get(found.front().Data()) : nullptr;
      fRootTree.push_back(dynamic_cast<TTree*>(o));
    } else
#endif
    fRootTree.push_back((TTree*)fRootFile->Get(found.front()));
    found.pop_front();
  }
  // Initialize the fTreeEntries vector
  fTreeEntries.clear();
  for(UInt_t i=0;i<fRootTree.size();i++) {
    fTreeEntries.push_back(0);
  }

}

UInt_t OnlineGUI::GetTreeIndex(TString var) {
  // Utility to find out which Tree (in fRootTree) has the specified
  // variable "var".  If the variable is a collection of Tree
  // variables (e.g. bcm1:lumi1), will only check the first
  // (e.g. bcm1).
  // Returns the correct index.  if not found returns an index 1
  // larger than fRootTree.size()

  //  This is for 2d draws... look for the first only
  if(var.Contains(":")) {
    TString first_var = fConfig->SplitString(var,":")[0];
    var = first_var;
  }
  if(var.Contains("-")) {
    TString first_var = fConfig->SplitString(var,"-")[0];
    var = first_var;
  }
  if(var.Contains("/")) {
    TString first_var = fConfig->SplitString(var,"/")[0];
    var = first_var;
  }
  if(var.Contains("*")) {
    TString first_var = fConfig->SplitString(var,"*")[0];
    var = first_var;
  }
  if(var.Contains("+")) {
    TString first_var = fConfig->SplitString(var,"+")[0];
    var = first_var;
  }
  if(var.Contains("(")) {
    TString first_var = fConfig->SplitString(var,"(")[0];
    var = first_var;
  }
  //  This is for variables with multiple dimensions.
  if(var.Contains("[")) {
    TString first_var = fConfig->SplitString(var,"[")[0];
    var = first_var;
  }

  if(fVerbosity>=3)
    cout<<__PRETTY_FUNCTION__<<"\t"<<__LINE__<<endl
	<<"\t looking for variable: "<<var<<endl;
  for(UInt_t iTree=0; iTree<treeVars.size(); iTree++) {
    for(UInt_t ivar=0; ivar<treeVars[iTree].size(); ivar++) {
      if(fVerbosity>=4)
	cout<<"Checking tree "<<iTree<<" name:"<<fRootTree[iTree]->GetName()
	    <<" \t var "<<ivar<<" >> "<<treeVars[iTree][ivar]<<endl;
      if(var == treeVars[iTree][ivar]) return iTree;
    }
  }

  return fRootTree.size()+1;
}

UInt_t OnlineGUI::GetTreeIndexFromName(TString name) {
  // Called by TreeDraw().  Tries to find the Tree index provided the
  //  name.  If it doesn't match up, return a number that's one larger
  //  than the number of found trees.
  for(UInt_t iTree=0; iTree<fRootTree.size(); iTree++) {
    TString treename = fRootTree[iTree]->GetName();
    if(name == treename) {
      return iTree;
    }
  }

  return fRootTree.size()+1;
}

#ifdef HAS_RNTUPLE_SUPPORT
void OnlineGUI::GetRootNTuple() {
  // Utility to search a ROOT File for RNTuples
  // Fills the fRootNTuple vector
  fRootNTuple.clear();
  fRootNTupleNames.clear();
  fNTupleFilters.clear();
  fNTupleDataFrames.clear();

  std::list<TString> found;
#ifdef QW_ENABLE_MAPFILE
  if(fIsMapFile && fRNTupleMemFile) {
    TIter nextKey(fRNTupleMemFile->GetListOfKeys());
    TKey *key = nullptr;
    while((key = static_cast<TKey*>(nextKey())) != nullptr) {
      TString keyName = key->GetName();
      auto *anchor = fRNTupleMemFile->Get<ROOT::RNTuple>(keyName.Data());
      if(anchor != nullptr) {
        found.push_back(keyName);
      }
    }
  } else
#endif
  {
  for(UInt_t i=0; i<fileObjects.size(); i++) {
    if(fVerbosity>=2)
      cout << "Object = " << fileObjects[i].second <<
        "     Name = " << fileObjects[i].first << endl;

    if(fileObjects[i].second.Contains("RNTuple")) {
      found.push_back(fileObjects[i].first);
    }
  }
  }

  found.sort();
  found.unique();
  // Search order for variables that exist in several RNTuples: like the
  // TTree order in JAPAN files (evt, mul, pr first; running sums last)
  {
    auto priority = [](const TString& n) {
      if (n == "evt") return 0;
      if (n == "mul") return 1;
      if (n == "pr" || n == "pr_yield") return 2;
      if (n == "pr_asym") return 3;
      if (n == "evts" || n == "muls" || n == "bursts") return 9;
      return 5;
    };
    std::vector<TString> ordered(found.begin(), found.end());
    std::stable_sort(ordered.begin(), ordered.end(),
                     [&](const TString& a, const TString& b) { return priority(a) < priority(b); });
    found.assign(ordered.begin(), ordered.end());
  }
  while(!found.empty()) {
    TString ntupleName = found.front();
    found.pop_front();

    try {
      std::unique_ptr<ROOT::RNTupleReader> ntuple;
#ifdef QW_ENABLE_MAPFILE
      if(fIsMapFile && fRNTupleMemFile) {
        auto *anchor = fRNTupleMemFile->Get<ROOT::RNTuple>(ntupleName.Data());
        if(anchor != nullptr) {
          ntuple = ROOT::RNTupleReader::Open(*anchor);
        }
      } else
#endif
      {
        TString fileName = fRootFile ? fRootFile->GetName() : "";
        if(!fileName.IsNull()) {
          ntuple = ROOT::RNTupleReader::Open(ntupleName.Data(), fileName.Data());
        }
      }

      if(ntuple != nullptr) {
        fRootNTuple.push_back(std::move(ntuple));
        fRootNTupleNames.push_back(ntupleName);
      } else if(fVerbosity>=1) {
        cout << "Failed to open RNTuple " << ntupleName << ": null reader" << endl;
      }
    } catch (std::exception& e) {
      if(fVerbosity>=1)
        cout << "Failed to open RNTuple " << ntupleName << ": " << e.what() << endl;
    }
  }

  // Initialize the fNTupleEntries vector
  fNTupleEntries.clear();
  for(UInt_t i=0;i<fRootNTuple.size();i++) {
    fNTupleEntries.push_back(0);
  }
}

void OnlineGUI::GetNTupleVars() {
  // Utility to find all of the variables (fields) within RNTuples
  // and put them within the ntupleVars vector.
  ntupleVars.clear();

  for(UInt_t i=0; i<fRootNTuple.size(); i++) {
    std::vector<TString> currentNTuple;
    currentNTuple.clear();

    try {
      const auto& descriptor = fRootNTuple[i]->GetDescriptor();

      // Get the top-level field (root field) and iterate through its sub-fields
      for (const auto& fieldDesc : descriptor.GetTopLevelFields()) {
        TString fieldName = fieldDesc.GetFieldName();
        // Add all top-level fields
        if (fieldName != "") {
          currentNTuple.push_back(fieldName);
        }
      }

    } catch (std::exception& e) {
      if(fVerbosity>=1)
        cout << "Error getting RNTuple variables: " << e.what() << endl;
    }

    ntupleVars.push_back(currentNTuple);
  }

  BuildNTupleAliases();

  if(fVerbosity>=5){
    for(UInt_t iNTuple=0; iNTuple<ntupleVars.size(); iNTuple++) {
      cout << "In RNTuple " << iNTuple << ": " << endl;
      for(UInt_t i=0; i<ntupleVars[iNTuple].size(); i++) {
        cout << ntupleVars[iNTuple][i] << endl;
      }
    }
  }
}

UInt_t OnlineGUI::GetNTupleIndex(TString var) {
  // Utility to find out which RNTuple (in fRootNTuple) holds the variables
  // of the draw expression "var" (TTree-style names; see
  // TranslateNTupleExpression).  Returns an index 1 larger than
  // fRootNTuple.size() if no RNTuple has all of them.
  return FindNTupleForExpression(var, "");
}

void OnlineGUI::BuildNTupleAliases() {
  // For every RNTuple, map the names a panguin configuration may use to the
  // RDataFrame expression that reads the value:
  //  - one field per value (default JAPAN layout): name -> name
  //  - one std::array<double,N> field per channel (--rntuple-channel-arrays),
  //    with the element names in the field description ("hw_sum:block0:..."):
  //    channel_element -> channel[i], and channel -> its main value
  //  - std::vector<double> fields (correlator matrices and vectors): name -> name
  fNTupleAliases.assign(fRootNTuple.size(), std::map<TString,TString>());
  fNTupleVectorFields.assign(fRootNTuple.size(), std::set<TString>());
  fNTupleNEntries.assign(fRootNTuple.size(), 0);

  for(UInt_t i=0; i<fRootNTuple.size(); i++) {
    if(fRootNTuple[i] == nullptr) continue;
    auto& aliases = fNTupleAliases[i];
    try {
      fNTupleNEntries[i] = fRootNTuple[i]->GetNEntries();
      const auto& descriptor = fRootNTuple[i]->GetDescriptor();
      for (const auto& fieldDesc : descriptor.GetTopLevelFields()) {
        const TString field = fieldDesc.GetFieldName();
        const std::string type = fieldDesc.GetTypeName();
        if (type.rfind("std::array<", 0) == 0) {
          // Number of elements from the type name, element names from the description
          const size_t comma = type.rfind(',');
          const UInt_t n = (comma == std::string::npos) ? 0 : std::stoul(type.substr(comma + 1));
          std::vector<TString> names;
          TString description = fieldDesc.GetFieldDescription();
          if (!description.IsNull()) {
            TObjArray* tokens = description.Tokenize(":");
            for (Int_t t = 0; t < tokens->GetEntries(); t++)
              names.push_back(((TObjString*)tokens->At(t))->GetString());
            delete tokens;
          }
          Int_t main = -1;
          for (UInt_t j = 0; j < n; j++) {
            const TString element = (names.size() == n) ? names[j] : TString::Format("%u", j);
            const TString key = (element == "value") ? field : field + "_" + element;
            aliases[key] = TString::Format("%s[%u]", field.Data(), j);
            if (main < 0 && (element == "hw_sum" || element == "value")) main = j;
          }
          // The channel name alone means its main value, like the first leaf
          // of the channel's TTree branch
          if (aliases.find(field) == aliases.end())
            aliases[field] = TString::Format("%s[%d]", field.Data(), main < 0 ? 0 : main);
        } else {
          if (type.rfind("std::vector<", 0) == 0) fNTupleVectorFields[i].insert(field);
          aliases[field] = field;
        }
      }
    } catch (std::exception& e) {
      if(fVerbosity>=1)
        cout << "Error reading RNTuple fields: " << e.what() << endl;
    }
  }
}

Bool_t OnlineGUI::ResolveNTupleName(UInt_t iNTuple, const TString& name, const TString& leaf, TString& expr) {
  // Resolve a TTree-style variable of RNTuple iNTuple: "name", or "name.leaf"
  // for one leaf of a channel's branch.  Returns kFALSE if it does not exist.
  const auto& aliases = fNTupleAliases[iNTuple];
  auto look = [&](const TString& key, TString& out) {
    auto it = aliases.find(key);
    if (it == aliases.end()) return kFALSE;
    out = it->second;
    return kTRUE;
  };

  if (leaf.IsNull()) {
    if (look(name, expr)) return kTRUE;
    // One field per value: the channel name means its main value (hw_sum),
    // like the first leaf of the channel's TTree branch
    if (look(name + "_hw_sum", expr)) return kTRUE;
    // TTree also accepts a unique leaf name alone, e.g. ppm for units.ppm
    return look("units_" + name, expr);
  }

  if (leaf == "value" && look(name, expr)) return kTRUE;
  if (look(name + "_" + leaf, expr)) return kTRUE;
  // ADC18 and scaler leaves value_m2/value_err are stored as name_m2/name_err
  if (leaf.BeginsWith("value_") && look(name + "_" + leaf(6, leaf.Length()), expr)) return kTRUE;
  // 64-bit sums of squares are stored as two exact 32-bit halves
  if (leaf.BeginsWith("SumSq_")) {
    const TString block = leaf(6, leaf.Length());
    TString low, high;
    if (look(name + "_sumsq" + block + "_low", low) && look(name + "_sumsq" + block + "_high", high)) {
      expr = "(" + high + "*4294967296.0+" + low + ")";
      return kTRUE;
    }
  }
  if (leaf.BeginsWith("RawMin_") && look(name + "_min" + leaf(7, leaf.Length()), expr)) return kTRUE;
  if (leaf.BeginsWith("RawMax_") && look(name + "_max" + leaf(7, leaf.Length()), expr)) return kTRUE;
  return kFALSE;
}

Bool_t OnlineGUI::TranslateNTupleExpression(UInt_t iNTuple, const TString& in, TString& out, TString& missing) {
  // Translate a TTree::Draw-style expression (variable or cut) into an
  // RDataFrame expression for RNTuple iNTuple.  Variable names are resolved
  // with ResolveNTupleName; Entry$ and Entries$ become the entry number and
  // the number of entries; function and namespace names (abs, TMath::Abs)
  // and numbers are kept.  On failure, missing holds the first unknown name.
  out = "";
  missing = "";
  if (iNTuple >= fNTupleAliases.size()) return kFALSE;

  const std::string s = in.Data();
  const size_t n = s.size();
  auto isIdStart = [](char c) { return std::isalpha((unsigned char)c) || c == '_'; };
  auto isIdChar = [](char c) { return std::isalnum((unsigned char)c) || c == '_'; };
  auto nextNonSpace = [&](size_t p) { while (p < n && std::isspace((unsigned char)s[p])) p++; return p; };

  size_t i = 0;
  while (i < n) {
    const char c = s[i];

    // Numbers, including exponents (1e6, 1.0e-4) and hexadecimal literals
    if (std::isdigit((unsigned char)c) || (c == '.' && i + 1 < n && std::isdigit((unsigned char)s[i+1]))) {
      const size_t start = i;
      const bool hex = (c == '0' && i + 1 < n && (s[i+1] == 'x' || s[i+1] == 'X'));
      i++;
      while (i < n && (isIdChar(s[i]) || s[i] == '.' ||
                       (!hex && (s[i] == '+' || s[i] == '-') && (s[i-1] == 'e' || s[i-1] == 'E'))))
        i++;
      out += TString(s.substr(start, i - start));
      continue;
    }

    if (!isIdStart(c)) { out += c; i++; continue; }

    // Identifier, possibly with a TTreeFormula '$' suffix
    const size_t start = i;
    while (i < n && isIdChar(s[i])) i++;
    if (i < n && s[i] == '$') i++;
    TString name(s.substr(start, i - start));

    // Namespace qualifiers and members (TMath::Abs) and function calls (abs())
    size_t before = start;
    while (before > 0 && std::isspace((unsigned char)s[before-1])) before--;
    const bool afterScope = (before >= 2 && s[before-1] == ':' && s[before-2] == ':');
    const size_t after = nextNonSpace(i);
    const bool beforeScope = (after + 1 < n && s[after] == ':' && s[after+1] == ':');
    const bool isCall = (after < n && s[after] == '(');
    if (name.EndsWith("$")) {
      if (name == "Entry$") { out += "static_cast<Long64_t>(rdfentry_)"; continue; }
      if (name == "Entries$") { out += TString::Format("static_cast<Long64_t>(%llu)", fNTupleNEntries[iNTuple]); continue; }
      missing = name;   // other TTreeFormula specials are not supported
      return kFALSE;
    }
    if (afterScope || beforeScope || isCall || name == "true" || name == "false") {
      out += name;
      continue;
    }

    // TTree leaf syntax: channel.leaf
    TString leaf;
    if (i + 1 < n && s[i] == '.' && isIdStart(s[i+1])) {
      size_t j = i + 1;
      while (j < n && isIdChar(s[j])) j++;
      leaf = TString(s.substr(i + 1, j - i - 1));
      i = j;
    }

    TString expr;
    if (!ResolveNTupleName(iNTuple, name, leaf, expr)) {
      missing = leaf.IsNull() ? name : name + "." + leaf;
      return kFALSE;
    }

    // Matrices of the correlator RNTuples are stored row-major with their
    // dimensions in <name>_ncols: name[i][j] -> name[(i)*name_ncols+(j)]
    if (fNTupleVectorFields[iNTuple].count(expr)) {
      const size_t open1 = nextNonSpace(i);
      if (open1 < n && s[open1] == '[') {
        const size_t close1 = s.find(']', open1);
        const size_t open2 = (close1 == std::string::npos) ? n : nextNonSpace(close1 + 1);
        TString ncols;
        if (open2 < n && s[open2] == '[' && ResolveNTupleName(iNTuple, expr + "_ncols", "", ncols)) {
          const size_t close2 = s.find(']', open2);
          if (close2 != std::string::npos) {
            out += expr + "[(" + TString(s.substr(open1 + 1, close1 - open1 - 1)) + ")*" + ncols +
                   "+(" + TString(s.substr(open2 + 1, close2 - open2 - 1)) + ")]";
            i = close2 + 1;
            continue;
          }
        }
      }
    }
    out += expr;
  }
  return kTRUE;
}

UInt_t OnlineGUI::FindNTupleForExpression(TString expr, const TString& ntupleName) {
  // Index of the RNTuple to draw expr from: the one named ntupleName (from
  // the -tree option), or else the first RNTuple that has every variable of
  // expr.  Files from older JAPAN versions split the TTree "pr" into the
  // RNTuples pr_yield and pr_asym; -tree pr then picks the one that has the
  // variables.  Returns fRootNTuple.size()+1 if there is none.
  const Ssiz_t projection = expr.Index(">>");
  if (projection != kNPOS) expr.Remove(projection);

  TString translated, missing;
  if (!ntupleName.IsNull()) {
    for (UInt_t i = 0; i < fRootNTupleNames.size(); i++)
      if (fRootNTupleNames[i] == ntupleName) return i;
    std::vector<UInt_t> parts;
    for (const char* suffix : {"_yield", "_asym"})
      for (UInt_t i = 0; i < fRootNTupleNames.size(); i++)
        if (fRootNTupleNames[i] == ntupleName + suffix) parts.push_back(i);
    for (UInt_t i : parts)
      if (TranslateNTupleExpression(i, expr, translated, missing)) return i;
    return parts.empty() ? fRootNTuple.size() + 1 : parts.front();
  }

  for (UInt_t i = 0; i < fRootNTuple.size(); i++)
    if (TranslateNTupleExpression(i, expr, translated, missing)) return i;
  return fRootNTuple.size() + 1;
}

#ifdef QW_ENABLE_MAPFILE
Bool_t OnlineGUI::LoadRNTupleSnapshot() {
  if(fMapFile == nullptr) {
    return kFALSE;
  }
  TObject *blob = fMapFile->Get("rntuple_snap");
  if(blob == nullptr) {
    fRNTupleMemFile.reset();
    return kFALSE;
  }
  auto *payload = dynamic_cast<TObjString*>(blob);
  if(payload == nullptr) {
    delete blob;
    fRNTupleMemFile.reset();
    return kFALSE;
  }
  const TString &data = payload->String();
  fRNTupleMemFile = std::make_unique<TMemFile>("rntuple_snap.root",
      const_cast<char*>(data.Data()), data.Length(), "READ");
  delete blob;
  if(fRNTupleMemFile == nullptr || fRNTupleMemFile->IsZombie()) {
    fRNTupleMemFile.reset();
    return kFALSE;
  }
  return kTRUE;
}
#endif

#endif // HAS_RNTUPLE_SUPPORT

void OnlineGUI::MacroDraw(vector <TString> command) {
  // Called by DoDraw(), this will make a call to the defined macro, and
  //  plot it in it's own pad.  One plot per macro, please.

  if(command[1].IsNull()) {
    cout << "macro command doesn't contain a macro to execute" << endl;
    return;
  }

  if(doGolden) fRootFile->cd();
  gROOT->Macro(command[1]);


}

void OnlineGUI::LoadDraw(vector <TString> command) {
  // Called by DoDraw(), this will load a shared object library
  // and then make a call to the defined macro, and
  // plot it in it's own pad.  One plot per macro, please.

  if(command[1].IsNull()) {
    cout << "load command doesn't contain a command to execute" << endl;
    return;
  }

  if(doGolden) fRootFile->cd();
  gSystem->Load(command[1]);
  gROOT->Macro(command[2]);


}

void OnlineGUI::LoadLib(vector <TString> command) {
  // Called by DoDraw(), this will load a shared object library

  if(command[1].IsNull()) {
    cout << "load command doesn't contain a shared object library path" << endl;
    return;
  }

  if(doGolden) fRootFile->cd();
  gSystem->Load(command[1]);


}

void OnlineGUI::DoDrawClear() {
  // Utility to grab the number of entries in each tree.  This info is
  // then used, if watching a file, to "clear" the TreeDraw
  // histograms, and begin looking at new data.
  for(UInt_t i=0; i<fTreeEntries.size(); i++) {
    fTreeEntries[i] = (Int_t) fRootTree[i]->GetEntries();
  }


}

void OnlineGUI::TimerUpdate() {
  // Called periodically by the timer, if "watchfile" is indicated
  // in the config.  Reloads the ROOT file, and updates the current page.
  if(fVerbosity>=1)
    cout<<__PRETTY_FUNCTION__<<"\t"<<__LINE__<<endl;

#ifdef QW_ENABLE_MAPFILE
  if(fIsMapFile) {
    // TMapFile points at a live, producer-updated shared-memory region.
    // No close/reopen needed: subsequent Get() calls in HistDraw fetch
    // the latest version of each histogram from shm.  Just refresh the
    // object listing in case the producer added new entries, and redraw.
    if(fMapFile == nullptr) {
      // Producer dropped away; arm CheckRootFile to wait for it to come back.
      timer->Reset();
      timer->Disconnect();
      timer->Connect(timer,"Timeout()","OnlineGUI",this,"CheckRootFile()");
      return;
    }
    GetFileObjects();
    // Re-fetch trees on every cycle: each TMapFile::Get returns a fresh
    // Streamer-decoded snapshot, so the previous fRootTree entries are
    // stale and must be replaced.
    GetRootTree();
    GetTreeVars();
    if(fUpdate) DoDraw();
    timer->Reset();
    return;
  }
#endif // QW_ENABLE_MAPFILE

#ifdef OLDTIMERUPDATE
  if(fVerbosity>=2)
    cout<<"\t rtFile: "<<fRootFile<<"\t"<<fConfig->GetRootFile()<<endl;
  if(fRootFile){
    fRootFile->Close();
    fRootFile->Delete();
    delete fRootFile;
    fRootFile=0;
  }
  fRootFile = new TFile(fConfig->GetRootFile(),"READ");
  if(fRootFile->IsZombie()) {
    cout << "New run not yet available.  Waiting..." << endl;
    fRootFile->Close();
    delete fRootFile;
    fRootFile = 0;
    timer->Reset();
    timer->Disconnect();
    timer->Connect(timer,"Timeout()","OnlineGUI",this,"CheckRootFile()");
    return;
  }

  // Update the runnumber
  runNumber = fConfig -> GetRunNumber();
  if(runNumber != 0) {
    TString rnBuff = "Run #";
    rnBuff += runNumber;
    fRunNumber->SetText(rnBuff.Data());
    hframe->Layout();
  }

  // Open the Root Trees.  Give a warning if it's not there..
  GetFileObjects();
  if (fUpdate) { // Only do this stuff if their are valid keys
    GetRootTree();
    GetTreeVars();
    for(UInt_t i=0; i<fRootTree.size(); i++) {
      if(fRootTree[i]==0) {
	fRootTree.erase(fRootTree.begin() + i);
      }
    }
    DoDraw();
  }
  timer->Reset();

#else

  if(fRootFile->IsZombie() || (fRootFile->GetSize() == -1)
     || (fRootFile->ReadKeys()==0)) {
    cout << "New run not yet available.  Waiting..." << endl;
    fRootFile->Close();
    delete fRootFile;
    fRootFile = 0;
    timer->Reset();
    timer->Disconnect();
    timer->Connect(timer,"Timeout()","OnlineGUI",this,"CheckRootFile()");
    return;
  }
  for(UInt_t i=0; i<fRootTree.size(); i++) {
    fRootTree[i]->Refresh();
  }
  DoDraw();
  timer->Reset();

#endif

}

void OnlineGUI::UpdateCurrentTime() {
  char buffer[9];
  time_t t = time(0);
  strftime(buffer, 9, "%T", localtime(&t));
  TString sNow("Current time: ");
  sNow += buffer;
  fNow->SetText(sNow);
  timerNow->Reset();
}

void OnlineGUI::BadDraw(TString errMessage) {
  // Routine to display (in Pad) why a particular draw method has
  // failed.
  TPaveText *pt = new TPaveText(0.1,0.1,0.9,0.9,"brNDC");
  pt->SetBorderSize(3);
  pt->SetFillColor(10);
  pt->SetTextAlign(22);
  pt->SetTextFont(72);
  pt->SetTextColor(2);
  pt->AddText(errMessage.Data());
  pt->Draw();
  if(fPrintOnly || fVerbosity>=1)
    cout << "Cannot draw: " << errMessage << endl;

}


void OnlineGUI::CheckRootFile() {
  // Check the path to the rootfile (should follow symbolic links)
  // ... If found:
  //   Reopen new root file,
  //   Reconnect the timer to TimerUpdate()

  if(gSystem->AccessPathName(fConfig->GetRootFile())==0) {
    cout << "Found the new run" << endl;
#ifdef QW_ENABLE_MAPFILE
    if(fIsMapFile && fMapFile == nullptr) {
      // Producer file appeared again; reattach.
      fMapFile = TMapFile::Create(fConfig->GetRootFile(), "READ");
      if(fMapFile == nullptr) return;
      fFileAlive = kTRUE;
      GetFileObjects();
    }
#endif
#ifndef OLDTIMERUPDATE
    if(OpenRootFile()==0) {
#endif
      timer->Reset();
      timer->Disconnect();
      timer->Connect(timer,"Timeout()","OnlineGUI",this,"TimerUpdate()");
#ifndef OLDTIMERUPDATE
    }
#endif
  } else {
    TString rnBuff = "Waiting for run";
    fRunNumber->SetText(rnBuff.Data());
    hframe->Layout();
  }

}

Int_t OnlineGUI::OpenRootFile() {

#ifdef QW_ENABLE_MAPFILE
  if(fIsMapFile) {
    // TMapFile is producer-managed; we just (re)attach and refresh.
    if(fMapFile == nullptr) {
      fMapFile = TMapFile::Create(fConfig->GetRootFile(), "READ");
    }
    if(fMapFile == nullptr) {
      cout << "Producer mapfile not yet available.  Waiting..." << endl;
      timer->Reset();
      timer->Disconnect();
      timer->Connect(timer,"Timeout()","OnlineGUI",this,"CheckRootFile()");
      return -1;
    }
    runNumber = fConfig->GetRunNumber();
    if(runNumber != 0) {
      TString rnBuff = "Run #";
      rnBuff += runNumber;
      fRunNumber->SetText(rnBuff.Data());
      hframe->Layout();
    }
    GetFileObjects();
    if(!fUpdate) return -1;
    GetRootTree();
    GetTreeVars();
    DoDraw();
    return 0;
  }
#endif // QW_ENABLE_MAPFILE

  fRootFile = new TFile(fConfig->GetRootFile(),"READ");
  if(fRootFile->IsZombie() || (fRootFile->GetSize() == -1)
     || (fRootFile->ReadKeys()==0)) {
    cout << "New run not yet available.  Waiting..." << endl;
    fRootFile->Close();
    delete fRootFile;
    fRootFile = 0;
    timer->Reset();
    timer->Disconnect();
    timer->Connect(timer,"Timeout()","OnlineGUI",this,"CheckRootFile()");
    return -1;
  }

  // Update the runnumber
  runNumber = fConfig->GetRunNumber();
  if(runNumber != 0) {
    TString rnBuff = "Run #";
    rnBuff += runNumber;
    fRunNumber->SetText(rnBuff.Data());
    hframe->Layout();
  }

  // Open the Root Trees.  Give a warning if it's not there..
  GetFileObjects();
  if (fUpdate) { // Only do this stuff if their are valid keys
    GetRootTree();
    GetTreeVars();
#ifdef HAS_RNTUPLE_SUPPORT
    // Initialize RNTuples
    GetRootNTuple();
    GetNTupleVars();
#endif // HAS_RNTUPLE_SUPPORT
    // Initialize DataFrame for large dataset support
    InitializeDataFrame();
    for(UInt_t i=0; i<fRootTree.size(); i++) {
      if(fRootTree[i]==0) {
	fRootTree.erase(fRootTree.begin() + i);
      }
    }
    DoDraw();
  } else {
    return -1;
  }
  return 0;

}

TH1* OnlineGUI::MakeDelta(TH1* current, const TString& key, Int_t nIter)
{
  // Build the "what's new" histogram for the -delta option.
  //
  // Returns a private clone holding (current - snapshot from nIter updates
  // ago), and pushes the current state onto the rolling per-key history so
  // the next update again shows only the addition over the last nIter
  // refreshes.  Only the last nIter snapshots are kept, so the oldest stored
  // snapshot is exactly the one from nIter updates ago once the history has
  // filled.  Until then (and on the first call for a key) the oldest
  // available snapshot -- or nothing, giving the full histogram -- is used.
  //
  // The returned histogram and the stored snapshots are detached from any
  // directory (SetDirectory(0)) and owned by us; the drawn clone is
  // intentionally long-lived so it stays alive for the pad, mirroring the
  // existing mapfile draw path.
  static Long64_t counter = 0;

  if (nIter < 1) nIter = 1;

  std::deque<TH1*>& history = fPrevHist[key];

  // Baseline = oldest snapshot we keep, which is the one from nIter updates
  // ago once the history has filled to nIter entries.
  TH1* prev = history.empty() ? 0 : history.front();

  TH1* delta = static_cast<TH1*>(current->Clone(Form("__panguin_delta_%lld", counter++)));
  delta->SetDirectory(0);
  if (prev) {
    // Same binning is guaranteed because both come from the same object.
    delta->Add(prev, -1.0);
  }
  if (nIter == 1) {
    delta->SetTitle(TString(current->GetTitle()) + " (new since last update)");
  } else {
    delta->SetTitle(TString(current->GetTitle()) +
                    Form(" (new over last %d updates)", nIter));
  }

  // Record the current state as the newest snapshot, then keep only the last
  // nIter snapshots so history.front() stays nIter updates behind.
  TH1* snapshot = static_cast<TH1*>(current->Clone(Form("__panguin_snap_%lld", counter++)));
  snapshot->SetDirectory(0);
  history.push_back(snapshot);
  while (history.size() > (size_t)nIter) {
    delete history.front();
    history.pop_front();
  }

  return delta;
}

void OnlineGUI::HistDraw(vector <TString> command) {
  // Called by DoDraw(), this will plot a histogram.

  Bool_t showGolden=kFALSE;
  if(doGolden) showGolden=kTRUE;

  if(command.size()==2)
    if(command[1]=="noshowgolden") {
      showGolden = kFALSE;
    }

  // -delta[ N]: draw what is new over the last N updates (default N=1, i.e.
  // since the previous update).  This is incompatible with the golden
  // overlay, so the golden reference is suppressed when a delta is requested.
  Bool_t doDelta = (command.size()>6 && command[6]=="delta");
  Int_t deltaN = 1;
  if(doDelta && command.size()>7 && command[7].IsDigit() && command[7].Atoi()>0)
    deltaN = command[7].Atoi();
  if(doDelta) showGolden = kFALSE;
  TString deltaKey = Form("%u_%s", current_page, command[0].Data());

  cout<<"showGolden= "<<showGolden<<endl;

  // Determine dimensionality of histogram.  TProfile derives from TH1D and
  // TProfile2D from TH2D, so a stored profile is drawn through the matching
  // 1D/2D branch (the (TH1D*)/(TH2D*) casts are valid upcasts and Draw() is
  // virtual, so it renders as a profile).
  for(UInt_t i=0; i<fileObjects.size(); i++) {
    if (fileObjects[i].first.Contains(command[0])) {
      if(fileObjects[i].second.Contains("TH1") ||
	 (fileObjects[i].second.Contains("TProfile") &&
	  !fileObjects[i].second.Contains("TProfile2D"))) {
#ifdef QW_ENABLE_MAPFILE
	if(fIsMapFile) {
	  mytemp1d = (TH1D*)GetObjectFromFile(command[0]);
	} else {
	  if(showGolden) fRootFile->cd();
	  mytemp1d = (TH1D*)gDirectory->Get(command[0]);
	}
#else
	if(showGolden) fRootFile->cd();
	mytemp1d = (TH1D*)gDirectory->Get(command[0]);
#endif
	if(mytemp1d==NULL || mytemp1d->GetEntries()==0) {
	  BadDraw("Empty Histogram");
	} else if(doDelta) {
	  MakeDelta(mytemp1d, deltaKey, deltaN)->Draw();
	} else {
	  if(showGolden) {
	    fGoldenFile->cd();
	    mytemp1d_golden = (TH1D*)gDirectory->Get(command[0]);
	    mytemp1d_golden->SetLineColor(30);
	    mytemp1d_golden->SetFillColor(30);
	    Int_t fillstyle=3027;
	    if(fPrintOnly) fillstyle=3010;
	    mytemp1d_golden->SetFillStyle(fillstyle);
	    mytemp1d_golden->Draw();
	    cout<<"one golden histo drawn"<<endl;
	    mytemp1d->Draw("sames");
	  } else {
	    mytemp1d->Draw();
	  }
	}
	break;
      }
      if(fileObjects[i].second.Contains("TH2") ||
	 fileObjects[i].second.Contains("TProfile2D")) {
#ifdef QW_ENABLE_MAPFILE
	if(fIsMapFile) {
	  mytemp2d = (TH2D*)GetObjectFromFile(command[0]);
	} else {
	  if(showGolden) fRootFile->cd();
	  mytemp2d = (TH2D*)gDirectory->Get(command[0]);
	}
#else
	if(showGolden) fRootFile->cd();
	mytemp2d = (TH2D*)gDirectory->Get(command[0]);
#endif
	if(mytemp2d==NULL || mytemp2d->GetEntries()==0) {
	  BadDraw("Empty Histogram");
	} else if(doDelta) {
	  MakeDelta(mytemp2d, deltaKey, deltaN)->Draw();
	} else {
	  // These are commented out for some reason (specific to DVCS?)
	  // 	  if(showGolden) {
	  // 	    fGoldenFile->cd();
	  // 	    mytemp2d_golden = (TH2D*)gDirectory->Get(command[0]);
	  // 	    mytemp2d_golden->SetMarkerColor(2);
	  // 	    mytemp2d_golden->Draw();
	  //mytemp2d->Draw("sames");
	  // 	  } else {
	  mytemp2d->Draw();
	  // 	  }
	}
	break;
      }
      if(fileObjects[i].second.Contains("TH3")) {
#ifdef QW_ENABLE_MAPFILE
	if(fIsMapFile) {
	  mytemp3d = (TH3D*)GetObjectFromFile(command[0]);
	} else {
	  if(showGolden) fRootFile->cd();
	  mytemp3d = (TH3D*)gDirectory->Get(command[0]);
	}
#else
	if(showGolden) fRootFile->cd();
	mytemp3d = (TH3D*)gDirectory->Get(command[0]);
#endif
	if(mytemp3d==NULL || mytemp3d->GetEntries()==0) {
	  BadDraw("Empty Histogram");
	} else if(doDelta) {
	  MakeDelta(mytemp3d, deltaKey, deltaN)->Draw();
	} else {
	  mytemp3d->Draw();
	  if(showGolden) {
	    fGoldenFile->cd();
	    mytemp3d_golden = (TH3D*)gDirectory->Get(command[0]);
	    mytemp3d_golden->SetMarkerColor(2);
	    mytemp3d_golden->Draw();
	    mytemp3d->Draw("sames");
	  } else {
	    mytemp3d->Draw();
	  }
	}
	break;
      }
    }
  }

}

void OnlineGUI::TreeDraw(vector <TString> command) {
  // Called by DoDraw(), this will plot a Tree Variable

  TString var = command[0];

  //  Check to see if we're projecting to a specific histogram
  TString histoname = command[0](TRegexp(">>.+(?"));
  if (histoname.Length()>0){
    histoname.Remove(0,2);
    Int_t bracketindex = histoname.First("(");
    if (bracketindex>0) histoname.Remove(bracketindex);
    if(fVerbosity>=3)
      std::cout << histoname << " "<< command[0](TRegexp(">>.+(?")) <<std::endl;
  } else {
    histoname = "htemp";
  }

  // Combine the cuts (definecuts and specific cuts)
  TCut cut = "";
  TString tempCut;
  if(command.size()>1) {
    tempCut = command[1];
    vector <TString> cutIdents = fConfig->GetCutIdent();
    for(UInt_t i=0; i<cutIdents.size(); i++) {
      if(tempCut.Contains(cutIdents[i])) {
	TString cut_found = (TString)fConfig->GetDefinedCut(cutIdents[i]);
	tempCut.ReplaceAll(cutIdents[i],cut_found);
      }
    }
    cut = (TCut)tempCut;
  }

  // Determine which Tree the variable comes from, then draw it.
  UInt_t iTree;
  if(command[4].IsNull()) {
    iTree = GetTreeIndex(var);
    if(fVerbosity>=2)
      cout<<"got index from variable "<<iTree<<endl;
  } else {
    iTree = GetTreeIndexFromName(command[4]);
    if(fVerbosity>=2)
      cout<<"got index from command "<<iTree<<endl;
  }
  TString drawopt = command[2];

  if(fVerbosity>=3)
    cout<<"\tDraw option:"<<drawopt<<" and histo name "<<histoname<<endl;
  Int_t errcode=0;
  if (iTree <= fRootTree.size() ) {
    if(fVerbosity>=1){
      cout<<__PRETTY_FUNCTION__<<"\t"<<__LINE__<<endl;
      cout<<command[0]<<"\t"<<command[1]<<"\t"<<command[2]<<"\t"<<command[3]
	  <<"\t"<<command[4]<<endl;
      if(fVerbosity>=2)
	cout<<"\tProcessing from tree: "<<iTree<<"\t"<<fRootTree[iTree]->GetTitle()<<"\t"
	    <<fRootTree[iTree]->GetName()<<endl;
    }
    errcode = fRootTree[iTree]->Draw(var,cut,drawopt);
    if (command[5].EqualTo("grid")){
      gPad->SetGrid();
    }

    TObject *hobj = (TObject*)gROOT->FindObject(histoname);
    if(fVerbosity>=3)
      cout<<"Finished drawing with error code "<<errcode<<endl;

    if(errcode==-1) {
      BadDraw(var+" not found");
    } else if (errcode!=0) {
      if(!command[3].IsNull()) {
        //  Generate a "unique" histogram name based on the MD5 of the drawn variable, cut, drawopt,
        //  and plot title.
        //  Makes it less likely to cause a name collision if two plot titles are the same.
        //  If you draw the exact same plot twice, the histograms will have the same name, but
        //  since they are exactly the same, you likely won't notice (or it will complain at you).
        TString tmpstring(var);
        tmpstring += cut.GetTitle();
        tmpstring += drawopt;
        tmpstring += command[3];
        TString myMD5 = tmpstring.MD5();
	TH1* thathist = (TH1*)hobj;
	thathist->SetNameTitle(myMD5,command[3]);
      }
    } else {
      BadDraw("Empty Histogram");
    }
  } else {
    BadDraw(var+" not found");
    if (fConfig->IsMonitor()){
      // Maybe we missed it... look again.  I dont like the code
      // below... maybe I can come up with something better
      GetFileObjects();
      GetRootTree();
      GetTreeVars();
    }
  }
}

#ifdef HAS_RNTUPLE_SUPPORT
void OnlineGUI::NTupleDraw(vector <TString> command) {
  // Called by DoDraw(), this will plot an RNTuple Variable

  TString var = command[0];

  //  Check to see if we're projecting to a specific histogram
  TString histoname = command[0](TRegexp(">>.+(?"));
  if (histoname.Length()>0){
    histoname.Remove(0,2);
    Int_t bracketindex = histoname.First("(");
    if (bracketindex>0) histoname.Remove(bracketindex);
    if(fVerbosity>=3)
      std::cout << histoname << " "<< command[0](TRegexp(">>.+(?")) <<std::endl;
  } else {
    histoname = "htemp";
  }

  // Combine the cuts (definecuts and specific cuts)
  TString cutExpression = "";
  if(command.size()>1) {
    cutExpression = command[1];
    vector <TString> cutIdents = fConfig->GetCutIdent();
    for(UInt_t i=0; i<cutIdents.size(); i++) {
      if(cutExpression.Contains(cutIdents[i])) {
	TString cut_found = (TString)fConfig->GetDefinedCut(cutIdents[i]);
	cutExpression.ReplaceAll(cutIdents[i],cut_found);
      }
    }
  }

  // Determine which RNTuple the variable comes from
  UInt_t iNTuple;
  if(command[4].IsNull()) {
    iNTuple = GetNTupleIndex(var);
    if(fVerbosity>=2)
      cout<<"got NTuple index from variable "<<iNTuple<<endl;
  } else {
    // Find RNTuple by name specified in -tree option
    TString requestedNTuple = command[4];
    iNTuple = fRootNTuple.size() + 1; // Default to not found
    for(UInt_t i=0; i<fRootNTupleNames.size(); i++) {
      if(fRootNTupleNames[i] == requestedNTuple) {
        iNTuple = i;
        break;
      }
    }
    if(fVerbosity>=2)
      cout<<"got NTuple index from command (-tree "<<requestedNTuple<<"): "<<iNTuple<<endl;
  }

  TString drawopt = command[2];

  if(fVerbosity>=3)
    cout<<"\tDraw option:"<<drawopt<<" and histo name "<<histoname<<endl;

  Int_t errcode=0;
  if (iNTuple < fRootNTuple.size() && fRootNTuple[iNTuple] != nullptr) {
    if(fVerbosity>=1){
      cout<<__PRETTY_FUNCTION__<<"\t"<<__LINE__<<endl;
      cout<<command[0]<<"\t"<<command[1]<<"\t"<<command[2]<<"\t"<<command[3]
	  <<"\t"<<command[4]<<endl;
      if(fVerbosity>=2)
	cout<<"\tProcessing from RNTuple: "<<iNTuple<<endl;
    }

    try {
      // Create RNTuple view for the variable - need to access raw pointer
      auto* ntuple = fRootNTuple[iNTuple].get();

      // Accept TTree-style names when they map to a single field
      {
        TString translated, missing;
        if (TranslateNTupleExpression(iNTuple, var, translated, missing) &&
            translated.Contains(TRegexp("^[A-Za-z_][A-Za-z0-9_]*$")))
          var = translated;
      }

      // Get the descriptor to find field information
      const auto& descriptor = ntuple->GetDescriptor();
      auto fieldId = descriptor.FindFieldId(var.Data());
      if (fieldId == ROOT::kInvalidDescriptorId) {
        BadDraw(TString("Field not found: ") + var);
        return;
      }

      // RNTuple views are strongly typed, so create a view matching the
      // field's actual on-disk type and expose the values as Double_t for
      // histogramming.  Without this, non-double fields (e.g. int) fail to
      // draw because GetView<Double_t> rejects them.
      const std::string typeName = descriptor.GetFieldDescriptor(fieldId).GetTypeName();

      std::function<Double_t(std::uint64_t)> getValue;
      auto makeAccessor = [&](auto typeTag) {
        using FieldT = decltype(typeTag);
        auto v = std::make_shared<decltype(ntuple->GetView<FieldT>(var.Data()))>(
            ntuple->GetView<FieldT>(var.Data()));
        getValue = [v](std::uint64_t i) {
          return static_cast<Double_t>((*v)(i));
        };
      };

      if (typeName == "double" || typeName == "Double_t") {
        makeAccessor(double{});
      } else if (typeName == "float" || typeName == "Float_t") {
        makeAccessor(float{});
      } else if (typeName == "std::int32_t" || typeName == "int" || typeName == "Int_t") {
        makeAccessor(std::int32_t{});
      } else if (typeName == "std::uint32_t" || typeName == "unsigned int" || typeName == "UInt_t") {
        makeAccessor(std::uint32_t{});
      } else if (typeName == "std::int64_t" || typeName == "long" || typeName == "long long" || typeName == "Long64_t") {
        makeAccessor(std::int64_t{});
      } else if (typeName == "std::uint64_t" || typeName == "unsigned long" || typeName == "unsigned long long" || typeName == "ULong64_t") {
        makeAccessor(std::uint64_t{});
      } else if (typeName == "std::int16_t" || typeName == "short" || typeName == "Short_t") {
        makeAccessor(std::int16_t{});
      } else if (typeName == "std::uint16_t" || typeName == "unsigned short" || typeName == "UShort_t") {
        makeAccessor(std::uint16_t{});
      } else if (typeName == "std::int8_t" || typeName == "char" || typeName == "signed char" || typeName == "Char_t") {
        makeAccessor(std::int8_t{});
      } else if (typeName == "std::uint8_t" || typeName == "unsigned char" || typeName == "UChar_t") {
        makeAccessor(std::uint8_t{});
      } else if (typeName == "bool" || typeName == "Bool_t") {
        makeAccessor(bool{});
      } else {
        // Fall back to a double view; if the field really is not convertible
        // the exception is caught below and reported via BadDraw.
        makeAccessor(double{});
      }

      // Determine histogram bounds from data
      auto nEntries = ntuple->GetNEntries();
      if (nEntries == 0) {
        BadDraw("Empty RNTuple");
        return;
      }

      // Sample some data to determine range
      auto sampleSize = std::min(static_cast<decltype(nEntries)>(1000), nEntries);
      Double_t minVal = 1e30, maxVal = -1e30;

      // For large datasets, sample more strategically (beginning, middle, end)
      if (nEntries > 10000) {
        decltype(nEntries) step = nEntries / sampleSize;
        for (decltype(nEntries) i = 0; i < sampleSize; i++) {
          decltype(nEntries) sampleIndex = (i * step) % nEntries;
          Double_t val = getValue(sampleIndex);
          if (val < minVal) minVal = val;
          if (val > maxVal) maxVal = val;
        }
      } else {
        // For smaller datasets, sample sequentially from the beginning
        for (decltype(nEntries) i = 0; i < sampleSize; i++) {
          Double_t val = getValue(i);
          if (val < minVal) minVal = val;
          if (val > maxVal) maxVal = val;
        }
      }

      // Create histogram
      TH1F* hist = new TH1F(histoname, command[3].IsNull() ? var : command[3],
                           100, minVal - 0.1*(maxVal-minVal), maxVal + 0.1*(maxVal-minVal));

      // Fill histogram
      decltype(nEntries) entriesFilled = 0;
      for (decltype(nEntries) i = 0; i < nEntries; i++) {
        Double_t val = getValue(i);

        // Apply cuts if specified (simplified implementation)
        Bool_t passCut = kTRUE;
        if (!cutExpression.IsNull()) {
          // For now, accept all entries - cuts would need proper parsing
          passCut = kTRUE;
        }

        if (passCut) {
          hist->Fill(val);
          entriesFilled++;
        }
      }

      if (entriesFilled > 0) {
        // Set histogram title
        if(!command[3].IsNull()) {
          TString tmpstring(var);
          tmpstring += cutExpression;
          tmpstring += drawopt;
          tmpstring += command[3];
          TString myMD5 = tmpstring.MD5();
          hist->SetNameTitle(myMD5, command[3]);
        }

        // Draw the histogram
        hist->Draw(drawopt);
        errcode = 1; // Success

        if (command[5].EqualTo("grid")){
          gPad->SetGrid();
        }

        if(fVerbosity>=3)
          cout<<"Finished drawing RNTuple with "<<entriesFilled<<" entries"<<endl;

      } else {
        BadDraw("No entries passed cuts");
        errcode = 0;
      }

    } catch (std::exception& e) {
      BadDraw(TString("RNTuple error: ") + e.what());
      errcode = -1;
    }

  } else {
    BadDraw(var+" not found in RNTuple");
    if (fConfig->IsMonitor()){
      // Maybe we missed it... look again
      GetFileObjects();
      GetRootNTuple();
      GetNTupleVars();
    }
  }
}
#endif // HAS_RNTUPLE_SUPPORT

void OnlineGUI::PrintToFile()
{
  // Routine to print the current page to a File.
  //  A file dialog pops up to request the file name.
  fCanvas = fEcanvas->GetCanvas();
  gStyle->SetPaperSize(20,24);
  static TString dir("printouts");
  TGFileInfo fi;
  const char *myfiletypes[] =
    { "All files","*",
      "Portable Document Format","*.pdf",
      "PostScript files","*.ps",
      "Encapsulated PostScript files","*.eps",
      "GIF files","*.gif",
      "JPG files","*.jpg",
      0,               0 };
  fi.fFileTypes = myfiletypes;
  fi.fIniDir    = StrDup(dir.Data());

  new TGFileDialog(gClient->GetRoot(), fMain, kFDSave, &fi);
  if(fi.fFilename!=NULL) fCanvas->Print(fi.fFilename);
}

void OnlineGUI::PrintPages() {
  // Routine to go through each defined page, and print the output to
  // a postscript file. (good for making sample histograms).

  // Open the RootFile
  //  unless we're watching a file.
#ifdef QW_ENABLE_MAPFILE
  fIsMapFile = LooksLikeMapFile(fConfig->GetRootFile());
  if(fIsMapFile) {
    fRootFile = nullptr;
    fMapFile = TMapFile::Create(fConfig->GetRootFile(), "READ");
    if(fMapFile == nullptr) {
      cout << "ERROR:  mapfile: " << fConfig->GetRootFile()
           << " could not be attached" << endl;
      gApplication->Terminate();
    } else {
      fFileAlive = kTRUE;
      GetFileObjects();
      GetRootTree();
      GetTreeVars();
#ifdef HAS_RNTUPLE_SUPPORT
      if(LoadRNTupleSnapshot()) {
        GetRootNTuple();
        GetNTupleVars();
      }
#endif // HAS_RNTUPLE_SUPPORT
      for(UInt_t i=0; i<fRootTree.size(); i++) {
        if(fRootTree[i]==0) {
          fRootTree.erase(fRootTree.begin() + i);
        }
      }
    }
  } else {
#endif
  fRootFile = new TFile(fConfig->GetRootFile(),"READ");
  if(!fRootFile->IsOpen()) {
    cout << "ERROR:  rootfile: " << fConfig->GetRootFile()
	 << " does not exist"
	 << endl;
    gApplication->Terminate();
  } else {
    fFileAlive = kTRUE;
    GetFileObjects();
    GetRootTree();
    GetTreeVars();
#ifdef HAS_RNTUPLE_SUPPORT
    // Initialize RNTuples
    GetRootNTuple();
    GetNTupleVars();
#endif // HAS_RNTUPLE_SUPPORT
    // Initialize DataFrame for large dataset support
    InitializeDataFrame();
    for(UInt_t i=0; i<fRootTree.size(); i++) {
      if(fRootTree[i]==0) {
	fRootTree.erase(fRootTree.begin() + i);
      }
    }

  }
#ifdef QW_ENABLE_MAPFILE
  }
#endif
  TString goldenfilename=fConfig->GetGoldenFile();
#ifdef QW_ENABLE_MAPFILE
  if(fIsMapFile && !goldenfilename.IsNull()) {
    cout << "NOTE:  goldenrootfile comparison is not supported in TMapFile "
            "mode; ignoring '" << goldenfilename << "'." << endl;
    goldenfilename = "";
  }
#endif
  if(!goldenfilename.IsNull()) {
    fGoldenFile = new TFile(goldenfilename,"READ");
    if(!fGoldenFile->IsOpen()) {
      cout << "ERROR: goldenrootfile: " << goldenfilename
	   << " does not exist.  Oh well, no comparison plots."
	   << endl;
      doGolden = kFALSE;
      fGoldenFile=NULL;
    } else {
      doGolden = kTRUE;
    }
  } else {
    doGolden=kFALSE;
    fGoldenFile=NULL;
  }

  fCanvas = new TCanvas("fCanvas","trythis",1000,800);
  TLatex *lt = new TLatex();

  TString plotsdir = fConfig->GetPlotsDir();
  if(plotsdir.IsNull()) plotsdir=".";

  Bool_t pagePrint = kFALSE;
  TString printFormat = fConfig->GetPlotFormat();
  if(printFormat.IsNull()) printFormat="pdf";
  if(printFormat!="pdf") pagePrint = kTRUE;

  TString filename = "summaryPlots";
  runNumber = fConfig->GetRunNumber();
  if(runNumber!=0) {
    filename += "_";
    filename += runNumber;
  } else {
    printf(" Warning for pretty plots: runNumber = %i\n",runNumber);
  }

  filename.Prepend(plotsdir+"/");
  if(pagePrint)
    filename += "_pageXXXX";
  TString fConfName = fConfig->GetConfFileName();
  TString fCfgNm = fConfName(fConfName.Last('/')+1,fConfName.Length());
  filename += "_" + fCfgNm(0,fCfgNm.Last('.'));
  filename += "."+printFormat;

  TString pagehead = "Summary Plots";
  if(runNumber!=0) {
    pagehead += "(Run #";
    pagehead += runNumber;
    pagehead += ")";
  }
  //  pagehead += ": ";

  gStyle->SetPalette(1);
  gStyle->SetTitleX(0.15);
  gStyle->SetTitleY(0.9);
  gStyle->SetPadBorderMode(0);
  gStyle->SetHistLineColor(1);
  gStyle->SetHistFillColor(1);
  if(!pagePrint) fCanvas->Print(filename+"[");
  TString origFilename = filename;
  for(UInt_t i=0; i<fConfig->GetPageCount(); i++) {
    current_page=i;
    DoDraw();
    if(fVerbosity>=1) {
      // One line per plot, to compare outputs (e.g. TTree vs RNTuple files)
      for(Int_t ipad=1; fCanvas->GetPad(ipad)!=nullptr; ipad++) {
        TIter next(fCanvas->GetPad(ipad)->GetListOfPrimitives());
        TH1* h = nullptr;
        TGraph* g = nullptr;
        while(TObject* obj = next()) {
          if(!g && obj->InheritsFrom(TGraph::Class())) g = (TGraph*)obj;
          if(!h && obj->InheritsFrom(TH1::Class())) h = (TH1*)obj;
        }
        if(g)
          printf("PLOT page %u pad %d: graph points=%d meanx=%.10g meany=%.10g%s\n",
                 i, ipad, g->GetN(), g->GetMean(1), g->GetMean(2),
                 h ? TString::Format(" frame x=[%.10g,%.10g] y=[%.10g,%.10g]",
                       h->GetXaxis()->GetXmin(), h->GetXaxis()->GetXmax(),
                       h->GetYaxis()->GetXmin(), h->GetYaxis()->GetXmax()).Data() : "");
        else if(h)
          printf("PLOT page %u pad %d: %s entries=%.0f mean=%.10g rms=%.10g x=[%.10g,%.10g]%s\n",
                 i, ipad, h->ClassName(), h->GetEntries(), h->GetMean(), h->GetRMS(),
                 h->GetXaxis()->GetXmin(), h->GetXaxis()->GetXmax(),
                 h->GetDimension() > 1 ? TString::Format(" meany=%.10g y=[%.10g,%.10g]", h->GetMean(2),
                   h->GetYaxis()->GetXmin(), h->GetYaxis()->GetXmax()).Data() : "");
        else
          printf("PLOT page %u pad %d: nothing drawn\n", i, ipad);
      }
    }
    TString pagename = pagehead;
    pagename += " ";
    pagename += i;
    pagename += ": ";
    pagename += fConfig->GetPageTitle(current_page);
    lt->SetTextSize(0.025);
    lt->DrawLatex(0.05,0.98,pagename);
    if(pagePrint) {
      filename = origFilename;
      filename.ReplaceAll("XXXX",Form("%02d",current_page));
      cout << "Printing page " << current_page
	   << " to file = " << filename << endl;
    }
    fCanvas->Print(filename);
  }
  if(!pagePrint) fCanvas->Print(filename+"]");

  gApplication->Terminate();
}

void OnlineGUI::MyCloseWindow()
{
  fMain->SendCloseMessage();
  cout << "OnlineGUI Closed." << endl;
  if(timer!=NULL) {
    timer->Stop();
    delete timer;
  }
  delete fPrint;
  delete fExit;
  delete fRunNumber;
  delete fPrev;
  delete fNext;
  delete wile;
  for(UInt_t i=0; i<fConfig->GetPageCount(); i++)
    delete fRadioPage[i];
  delete hframe;
  delete fEcanvas;
  delete fBottomFrame;
  delete vframe;
  delete fTopframe;
  delete fMain;
  if(fGoldenFile!=NULL) delete fGoldenFile;
  if(fRootFile!=NULL) delete fRootFile;
#ifdef QW_ENABLE_MAPFILE
  fRNTupleMemFile.reset();
  if(fMapFile!=nullptr) { fMapFile->Close(); fMapFile = nullptr; }
#endif
  delete fConfig;

  gApplication->Terminate();
}

void OnlineGUI::CloseGUI()
{
  // Routine to take care of the Exit GUI button
  fMain->SendCloseMessage();
}

void OnlineGUI::InitializeDataFrame() {
  // Initialize RDataFrame for large dataset processing
  // This will be created on-demand when needed
  fDataFrame.reset();

  if(fVerbosity>=2)
    cout << "DataFrame initialization deferred until needed" << endl;
}

void OnlineGUI::DataFrameDraw(vector <TString> command) {
  // Called by DoDraw(), this plots from an RNTuple using RDataFrame.  It
  // accepts the same draw commands as TreeDraw: "x", "y:x" or "z:y:x" with
  // TTree-style variable names (see TranslateNTupleExpression), cuts, and the
  // TTree::Draw options (prof, box, colz, scat, graph-style 2D plots), with
  // the same default binning and axis ranges as TTree::Draw.

  TString var = command[0];

  //  Check to see if we're projecting to a specific histogram:
  //  var>>name or var>>name(nbins,low,high[,nbins,low,high[,...]])
  TString histoname = "htemp";
  std::vector<Double_t> binning;
  const Ssiz_t projection = var.Index(">>");
  if (projection != kNPOS) {
    TString target = var(projection + 2, var.Length());
    var.Remove(projection);
    target = target.Strip(TString::kBoth);
    const Ssiz_t open = target.First('(');
    if (open != kNPOS) {
      TString args = target(open + 1, target.Length());
      const Ssiz_t close = args.Last(')');
      if (close != kNPOS) args.Remove(close);
      TObjArray* tokens = args.Tokenize(",");
      for (Int_t t = 0; t < tokens->GetEntries(); t++)
        binning.push_back(((TObjString*)tokens->At(t))->GetString().Atof());
      delete tokens;
      target.Remove(open);
    }
    if (!target.IsNull()) histoname = target;
  }

  // Combine the cuts (definecuts and specific cuts)
  TString cutExpression = "";
  if(command.size()>1) {
    cutExpression = command[1];
    vector <TString> cutIdents = fConfig->GetCutIdent();
    for(UInt_t i=0; i<cutIdents.size(); i++) {
      if(cutExpression.Contains(cutIdents[i])) {
	TString cut_found = (TString)fConfig->GetDefinedCut(cutIdents[i]);
	cutExpression.ReplaceAll(cutIdents[i],cut_found);
      }
    }
  }

  // Determine which RNTuple the variables come from
  const UInt_t iNTuple = FindNTupleForExpression(var, command[4]);
  if (iNTuple >= fRootNTuple.size() || fRootNTuple[iNTuple] == nullptr) {
    TString message;
    if (!command[4].IsNull()) {
      message = "RNTuple " + command[4] + " not found";
    } else {
      TString translated, missing;
      for (UInt_t i = 0; i < fRootNTuple.size() && missing.IsNull(); i++)
        TranslateNTupleExpression(i, var, translated, missing);
      message = (missing.IsNull() ? var : missing) + " not found";
    }
    BadDraw(message);
    if (fConfig->IsMonitor()){
      // Maybe we missed it... look again
      GetFileObjects();
      GetRootNTuple();
      GetNTupleVars();
    }
    return;
  }
  const TString ntupleName = fRootNTupleNames[iNTuple];

  // Split "z:y:x" into its dimensions (TTree order: the last one is x),
  // ignoring "::" and colons inside parentheses or brackets
  std::vector<TString> dims;
  {
    Int_t depth = 0;
    TString current;
    for (Ssiz_t k = 0; k < var.Length(); k++) {
      const char c = var[k];
      if (c == '(' || c == '[') depth++;
      if (c == ')' || c == ']') depth--;
      if (c == ':' && depth == 0) {
        const bool scope = (k + 1 < var.Length() && var[k+1] == ':') || (k > 0 && var[k-1] == ':');
        if (!scope) { dims.push_back(current); current = ""; continue; }
      }
      current += c;
    }
    dims.push_back(current);
  }
  if (dims.size() > 3) {
    BadDraw("Only 1D, 2D and 3D draws are supported for RNTuples");
    return;
  }

  // Translate the variables and the cut into RDataFrame expressions
  std::vector<TString> exprs;
  TString cut, missing;
  for (auto& d : dims) {
    TString expr;
    if (!TranslateNTupleExpression(iNTuple, d, expr, missing)) {
      BadDraw(missing + " not found in RNTuple " + ntupleName);
      return;
    }
    exprs.push_back(expr);
  }
  if (!cutExpression.IsNull() &&
      !TranslateNTupleExpression(iNTuple, cutExpression, cut, missing)) {
    BadDraw(missing + " (cut) not found in RNTuple " + ntupleName);
    return;
  }

  TString drawopt = command[2];
  TString opt = drawopt;
  opt.ToLower();

  if(fVerbosity>=1){
    cout<<__PRETTY_FUNCTION__<<"\t"<<__LINE__<<endl;
    cout<<command[0]<<"\t"<<command[1]<<"\t"<<command[2]<<"\t"<<command[3]
        <<"\t"<<command[4]<<endl;
    if(fVerbosity>=2) {
      cout<<"\tProcessing from RNTuple "<<ntupleName<<":";
      for (auto& e : exprs) cout<<" ["<<e<<"]";
      if (!cut.IsNull()) cout<<" cut ["<<cut<<"]";
      cout<<endl;
    }
  }

  // Read the selected values
  std::vector< std::vector<Double_t> > values;
  try {
    if (fNTupleDataFrames.size() < fRootNTuple.size()) {
      fNTupleDataFrames.resize(fRootNTuple.size());
      fNTupleFilters.resize(fRootNTuple.size());
    }
    if (!fNTupleDataFrames[iNTuple])
      fNTupleDataFrames[iNTuple] = std::make_unique<ROOT::RDataFrame>(ntupleName.Data(), fRootFile->GetName());
    ROOT::RDF::RNode node = *fNTupleDataFrames[iNTuple];
    if (!cut.IsNull()) {
      // Each cut is compiled once and its filter is shared by later plots
      auto filter = fNTupleFilters[iNTuple].find(cut);
      if (filter == fNTupleFilters[iNTuple].end())
        filter = fNTupleFilters[iNTuple].emplace(cut,
                   node.Filter(("static_cast<bool>(" + cut + ")").Data())).first;
      node = filter->second;
    }

    // A whole correlator matrix or vector is drawn element by element, like
    // an array leaf of a TTree
    const bool wholeVector = (exprs.size() == 1 && fNTupleVectorFields[iNTuple].count(exprs[0]) > 0);
    if (wholeVector) {
      auto rows = node.Take<std::vector<Double_t>>(exprs[0].Data());
      values.emplace_back();
      for (const auto& row : *rows) values[0].insert(values[0].end(), row.begin(), row.end());
    } else {
      const auto& descriptor = fRootNTuple[iNTuple]->GetDescriptor();
      std::vector< ROOT::RDF::RResultPtr< std::vector<Double_t> > > results;
      for (size_t j = 0; j < exprs.size(); j++) {
        // A double field is read directly; anything else needs a
        // just-in-time compiled Define
        const auto fieldId = descriptor.FindFieldId(exprs[j].Data());
        if (fieldId != ROOT::kInvalidDescriptorId &&
            descriptor.GetFieldDescriptor(fieldId).GetTypeName() == "double") {
          results.push_back(node.Take<Double_t>(exprs[j].Data()));
          continue;
        }
        const std::string column = "panguin_v" + std::to_string(j);
        node = node.Define(column, ("static_cast<double>(" + exprs[j] + ")").Data());
        results.push_back(node.Take<Double_t>(column));
      }
      for (auto& r : results) values.push_back(*r);
    }
  } catch (std::exception& e) {
    // A failed just-in-time compilation leaves the RDataFrame unusable
    if (iNTuple < fNTupleDataFrames.size()) {
      fNTupleFilters[iNTuple].clear();
      fNTupleDataFrames[iNTuple].reset();
    }
    BadDraw(TString("RNTuple draw error: ") + e.what());
    return;
  }

  const size_t nValues = values[0].size();
  if (nValues == 0) {
    BadDraw("Empty Histogram");
    return;
  }

  // Like TTree::Draw, an axis is an integer axis (bins aligned to whole
  // numbers) when it shows a single variable with integer values; RNTuple
  // fields of JAPAN store all channel values as double, so check the values
  std::vector<bool> isInteger(dims.size(), false);
  for (size_t j = 0; j < dims.size() && j < values.size(); j++) {
    TString d = dims[j].Strip(TString::kBoth);
    while (d.BeginsWith("(") && d.EndsWith(")")) d = TString(d(1, d.Length() - 2)).Strip(TString::kBoth);
    if (!d.Contains(TRegexp("^[A-Za-z_][A-Za-z0-9_]*\\$?$")) &&
        !d.Contains(TRegexp("^[A-Za-z_][A-Za-z0-9_]*\\.[A-Za-z_][A-Za-z0-9_]*$")))
      continue;
    bool integer = true;
    for (Double_t x : values[j])
      if (!std::isfinite(x) || x != std::floor(x) || std::fabs(x) > 9007199254740992.) { integer = false; break; }
    isInteger[j] = integer;
  }
  auto markInteger = [&](TAxis* axis, size_t j) { if (isInteger[j]) axis->SetBit(TAxis::kIsInteger); };

  // Data ranges for the axes, ignoring non-finite values
  auto dataRange = [](const std::vector<Double_t>& v, Double_t& lo, Double_t& hi) {
    lo = 0; hi = 0;
    bool first = true;
    for (Double_t x : v) {
      if (!std::isfinite(x)) continue;
      if (first) { lo = hi = x; first = false; }
      else { if (x < lo) lo = x; if (x > hi) hi = x; }
    }
  };
  // Binning from ">>name(...)" (axis a), or the TTree::Draw default
  auto nbins = [&](size_t a, const char* envName, Int_t def) {
    return (binning.size() > 3*a) ? (Int_t)binning[3*a] : gEnv->GetValue(envName, def);
  };
  auto explicitRange = [&](size_t a, Double_t& lo, Double_t& hi) {
    if (binning.size() < 3*a + 3 || binning[3*a + 1] >= binning[3*a + 2]) return false;
    lo = binning[3*a + 1]; hi = binning[3*a + 2];
    return true;
  };

  TString title = var;
  if (!cutExpression.IsNull()) title += " {" + cutExpression + "}";
  TH1* hist = nullptr;

  // TTree::Draw takes line, fill and marker attributes from the TTree, which
  // has those of ROOT's default style when JAPAN wrote it; do the same, so
  // that plots look alike whichever format the file holds
  auto applyTreeStyle = [&](auto* obj) {
    fTreeLineAtt.Copy(*obj);
    fTreeFillAtt.Copy(*obj);
    fTreeMarkerAtt.Copy(*obj);
  };

  if (dims.size() == 1) {
    TH1D* h = new TH1D(histoname, title, nbins(0, "Hist.Binning.1D.x", 100), 0, 1);
    h->SetDirectory(nullptr);
    markInteger(h->GetXaxis(), 0);
    Double_t lo, hi;
    if (explicitRange(0, lo, hi)) h->SetBins(h->GetNbinsX(), lo, hi);
    else { dataRange(values[0], lo, hi); THLimitsFinder::GetLimitsFinder()->FindGoodLimits(h, lo, hi); }
    for (Double_t x : values[0]) h->Fill(x);
    h->GetXaxis()->SetTitle(dims[0]);
    hist = h;
  } else if (dims.size() == 2) {
    const std::vector<Double_t>& y = values[0];
    const std::vector<Double_t>& x = values[1];
    Double_t xlo, xhi, ylo, yhi;
    if (opt.Contains("prof")) {
      TString profopt = "";
      if (opt.Contains("profs")) profopt = "s";
      else if (opt.Contains("profi")) profopt = "i";
      else if (opt.Contains("profg")) profopt = "g";
      TProfile* p = new TProfile(histoname, title, nbins(0, "Hist.Binning.2D.Prof", 100), 0, 1, profopt);
      p->SetDirectory(nullptr);
      markInteger(p->GetXaxis(), 1);
      if (explicitRange(0, xlo, xhi)) p->SetBins(p->GetNbinsX(), xlo, xhi);
      else { dataRange(x, xlo, xhi); THLimitsFinder::GetLimitsFinder()->FindGoodLimits(p, xlo, xhi); }
      for (size_t k = 0; k < nValues; k++) p->Fill(x[k], y[k]);
      hist = p;
    } else {
      TH2D* h2 = new TH2D(histoname, title,
                          nbins(0, "Hist.Binning.2D.x", 40), 0, 1,
                          nbins(1, "Hist.Binning.2D.y", 40), 0, 1);
      h2->SetDirectory(nullptr);
      markInteger(h2->GetXaxis(), 1);
      markInteger(h2->GetYaxis(), 0);
      const bool xFixed = explicitRange(0, xlo, xhi), yFixed = explicitRange(1, ylo, yhi);
      if (!xFixed) dataRange(x, xlo, xhi);
      if (!yFixed) dataRange(y, ylo, yhi);
      if (xFixed && yFixed) h2->SetBins(h2->GetNbinsX(), xlo, xhi, h2->GetNbinsY(), ylo, yhi);
      else THLimitsFinder::GetLimitsFinder()->FindGoodLimits(h2, xlo, xhi, ylo, yhi);
      h2->GetXaxis()->SetTitle(dims[1]);
      h2->GetYaxis()->SetTitle(dims[0]);
      h2->SetStats(kFALSE);
      // Like TTree::Draw: without a histogram option, or with p, * or l,
      // draw the points as a graph on an empty frame
      bool graph = opt.IsNull() || opt.Contains("p") || opt.Contains("*") || opt.Contains("l");
      if (opt.Contains("surf") || opt.Contains("lego") || opt.Contains("cont") ||
          opt.Contains("col") || opt.Contains("hist") || opt.Contains("scat") || opt.Contains("box"))
        graph = false;
      if (graph) {
        // As TTree::Draw: a lower axis limit of 0 is moved to the data minimum
        // when that is inside the first bin, so log scales do not cut data
        if (!(xFixed && yFixed)) {
          TAxis* aX = h2->GetXaxis();
          TAxis* aY = h2->GetYaxis();
          Double_t xmin = aX->GetXmin(), ymin = aY->GetXmin();
          if (xmin == 0 || ymin == 0) {
            if (aX->GetBinUpEdge(aX->FindFixBin(0.01*aX->GetBinWidth(aX->GetFirst()))) > xlo) xmin = xlo;
            if (aY->GetBinUpEdge(aY->FindFixBin(0.01*aY->GetBinWidth(aY->GetFirst()))) > ylo) ymin = ylo;
            h2->SetBins(aX->GetNbins(), xmin, aX->GetXmax(), aY->GetNbins(), ymin, aY->GetXmax());
          }
        }
        h2->SetBit(kCanDelete);
        h2->Draw();
        TGraph* g = new TGraph((Int_t)nValues, x.data(), y.data());
        g->SetEditable(kFALSE);
        g->SetBit(kCanDelete);
        applyTreeStyle(g);
        applyTreeStyle(h2);
        TString gopt = drawopt;
        gopt.ReplaceAll("same", "");
        gopt.ReplaceAll("a", "");
        gopt.ReplaceAll("A", "");
        g->Draw(gopt.IsNull() ? "p" : gopt.Data());
        if (!command[3].IsNull()) h2->SetTitle(command[3]);
        if (command[5].EqualTo("grid")) gPad->SetGrid();
        if(fVerbosity>=3)
          cout<<"Finished RNTuple drawing with "<<nValues<<" points"<<endl;
        return;
      }
      for (size_t k = 0; k < nValues; k++) h2->Fill(x[k], y[k]);
      hist = h2;
    }
  } else {
    const std::vector<Double_t>& z = values[0];
    const std::vector<Double_t>& y = values[1];
    const std::vector<Double_t>& x = values[2];
    TH3D* h3 = new TH3D(histoname, title,
                        nbins(0, "Hist.Binning.3D.x", 20), 0, 1,
                        nbins(1, "Hist.Binning.3D.y", 20), 0, 1,
                        nbins(2, "Hist.Binning.3D.z", 20), 0, 1);
    h3->SetDirectory(nullptr);
    markInteger(h3->GetXaxis(), 2);
    markInteger(h3->GetYaxis(), 1);
    markInteger(h3->GetZaxis(), 0);
    Double_t xlo, xhi, ylo, yhi, zlo, zhi;
    dataRange(x, xlo, xhi); dataRange(y, ylo, yhi); dataRange(z, zlo, zhi);
    THLimitsFinder::GetLimitsFinder()->FindGoodLimits(h3, xlo, xhi, ylo, yhi, zlo, zhi);
    for (size_t k = 0; k < nValues; k++) h3->Fill(x[k], y[k], z[k]);
    h3->GetXaxis()->SetTitle(dims[2]);
    h3->GetYaxis()->SetTitle(dims[1]);
    h3->GetZaxis()->SetTitle(dims[0]);
    hist = h3;
  }

  // Like TreeDraw: a unique name when a plot title is given
  if(!command[3].IsNull()) {
    TString tmpstring(command[0]);
    tmpstring += cutExpression;
    tmpstring += drawopt;
    tmpstring += command[3];
    hist->SetNameTitle(tmpstring.MD5(), command[3]);
  }
  hist->SetBit(kCanDelete);
  applyTreeStyle(hist);
  hist->Draw(drawopt);
  if (command[5].EqualTo("grid")){
    gPad->SetGrid();
  }
  if(fVerbosity>=3)
    cout<<"Finished RNTuple drawing with "<<hist->GetEntries()<<" entries"<<endl;
}
OnlineGUI::~OnlineGUI()
{
  //  fMain->SendCloseMessage();
  if(timer!=NULL) {
    timer->Stop();
    delete timer;
  }
  delete fPrint;
  delete fExit;
  delete fRunNumber;
  delete fPrev;
  delete fNext;
  delete wile;
  for(UInt_t i=0; i<fConfig->GetPageCount(); i++)
    delete fRadioPage[i];
  delete hframe;
  delete fEcanvas;
  delete vframe;
  delete fBottomFrame;
  delete fTopframe;
  delete fMain;
  if(fGoldenFile!=NULL) delete fGoldenFile;
  if(fRootFile!=NULL) delete fRootFile;
#ifdef QW_ENABLE_MAPFILE
  // Mapfile-resident TTree clones are caller-owned; release them before
  // detaching from the shared-memory region.
  if(fIsMapFile) {
    for(auto *t : fRootTree) delete t;
    fRootTree.clear();
    fRNTupleMemFile.reset();
  }
  if(fMapFile!=nullptr) { fMapFile->Close(); fMapFile = nullptr; }
#endif
  delete fConfig;
}
