
//plot_fft.C
//Sofia Hoffman
//07/2/24

//This program is used to take the beam data from the MOLLER experiment and turn it into FFT magnitude graphs and plots.
//To change the channel name, load root and use the command:set_chan(name).
//To change the frequency limit, you need to change the event period using the command: set_period(period).
//You can also change the CntHouse file run number by using the command:open_myfile(), and in the parenthesis you will insert the run number. For example: open_myfile(16664).
//The parameters of the plot can also be changed when running the program.
//"open_myfile" can also be used with the full path to open any other root file. 
#include "TFile.h"
#include "TTree.h"

#include "TH1D.h"
#include "TVirtualFFT.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TMath.h"
#include "TString.h"

TFile *myfile = NULL;
TTree *mytree = NULL;

TString chan_name("bcm_an_ds");

Double_t frequency_lim=-1;

Int_t plot_counter=0;

Double_t evt_period=520.85e-6;

Int_t run_number;

void set_frequency_lim(Double_t input)
{
  if (input < 0)
    std::cout << "The frequency limit needs to be greater than 0" << endl;

  else  if (input > .5/evt_period)
    std::cout << "The frequency limit needs to be less than half the event frequency." << endl;

  else frequency_lim = input;

}

  

//This is having the user choose the event period for frequencies of either 920Hz or 240Hz and setting it to a variable called "evt_period".

void set_period()
{

std::cout << "What is the event period? For runs with 1920Hz, the event period is 520.85e-6. For runs with 240Hz, the event period is 4066.65e-6. Input your period: " << endl;

std::cin >> evt_period;

}

void set_period(Double_t input)
{
  evt_period=input;
}

void set_chan(TString input)
{
  chan_name=input;
}

void open_myfile(Int_t run_num=16664, 
		 TString path="/volatile/halla/moller12gev/pking/rootfiles/",
		 TString name="sbs_CntHouse_")
{
  run_number=run_num;
  TString filename(Form("%s/%s%d.root",path.Data(),name.Data(),run_number));
  std::cout << filename << endl;
  myfile = new TFile(filename);
  myfile->Print();
  mytree = (TTree*)myfile->Get("evt");
}
void open_myfile(TString fullname)
{
  myfile = new TFile(fullname);
  myfile->Print();
  mytree = (TTree*)myfile->Get("evt");
}

void plot_fft(Int_t first_event=0, Int_t num_events=1000)
{
  //TFile *f = new TFile("/chafs2/work1/parity/japanOutput/sbs_CntHouse_16664.root");
  // TTree *t1 = (TTree*)f->Get("evt");

  if (myfile == NULL) open_myfile();

  if (frequency_lim < 0)
    frequency_lim = 0.5/evt_period;

  //  The data structure in the tree for each branch has 13 Double_t values.  We will
  //  just want to take the first one "[0]".
   Double_t chan_data[13];
   mytree->SetBranchAddress(chan_name,&chan_data);

   Long64_t nentries = mytree->GetEntries();
   Long64_t maxevent = first_event + num_events;
   if (maxevent>nentries){
     maxevent   = nentries;
     num_events = maxevent - first_event;
   }
   //Allocate an array big enough to hold the transform output
   //Transform output in 1d contains, for a transform of size N,
   //N/2+1 complex numbers, i.e. 2*(N/2+1) real numbers
   //our transform is of size n+1, because the histogram has n+1 bins
   Int_t n=num_events;
   Double_t x;
   Double_t *in = new Double_t[2*((n+1)/2+1)];
   Double_t re_2,im_2;

   for (Int_t i=0;i<2*((n+1)/2+1);i++){
     in[i]=0;
   }
   
   Int_t ii=0;
   Double_t meanval=0.0;
   for (Long64_t i=first_event;i<maxevent;i++) {
     mytree->GetEntry(i);
     //  At this point, the variable "chan_data[0]" will have the value for the event "i".
     //  You could then assign that to the data array that we'd use for the FFT.
     //  if (i%100==0) std::cout << "i=="<<i << " chan_data=="<< chan_data[0] <<std::endl;
     in[ii++]=chan_data[0];
     meanval += chan_data[0];
   }
   meanval /= ii;
   for (Int_t i=0; i<ii; i++){
     in[i] -= meanval;
   }

   // Histograms
   // =========
   //prepare the canvas for drawing
   TF1 *fsin = new TF1("fsin", "sin(x)+sin(2*x)+sin(0.5*x)+1", 0, 4*TMath::Pi());

// Data array - same transform
// ===========================
 
   
   //Make our own TVirtualFFT object (using option "K")
   //Third parameter (option) consists of 3 parts:
   //- transform type:
   // real input/complex output in our case
   //- transform flag:
   // the amount of time spent in planning
   // the transform (see TVirtualFFT class description)
   //- to create a new TVirtualFFT object (option "K") or use the global (default)
   Int_t n_size = n+1;
   TVirtualFFT *fft_own = TVirtualFFT::FFT(1, &n_size, "R2C ES K");
   if (!fft_own) return;
   fft_own->SetPoints(in);
   fft_own->Transform();
 
   //Copy all the output points:
   fft_own->GetPoints(in);
   //Draw the real part of the output
   TH1 *hr = nullptr;
   hr = TH1::TransformHisto(fft_own, hr, "MAG");
   hr->SetTitle("Magnitude of the 3rd (array) transform");
   hr->Draw();
   hr->SetStats(kFALSE);
   hr->GetXaxis()->SetLabelSize(0.05);
   hr->GetYaxis()->SetLabelSize(0.05);
   //   hr->GetXaxis()->SetRangeUser(0,300);

   std::cout << "Bins in Magnitude Histogram" << hr->GetNbinsX()
 << endl; 

   TString histname(Form("FFTmag%d",plot_counter++));
   TH1D *FFTmag = new TH1D(histname, histname, hr->GetNbinsX(), 0, 1.0/evt_period);

   for (Int_t j=0;j<hr->GetNbinsX();j++) {
     FFTmag->SetBinContent(j,hr->GetBinContent(j)/sqrt(n));
   }
   FFTmag->SetTitle(Form("FFT Magnitude for %s in range %d-%lld",chan_name.Data(),first_event,maxevent));
   // if (frequency_lim > .5/evt_period)
   //  frequency_lim = .5/evt_period;
   FFTmag->GetXaxis()->SetRangeUser(0,frequency_lim);
   FFTmag->Draw();

  
   
   delete fft_own;
   delete [] in;
   // delete [] re_full;
   // delete [] im_full;
}



void plot_block_fft(Int_t first_event=0, Int_t num_events=1000)
{
  //TFile *f = new TFile("/chafs2/work1/parity/japanOutput/sbs_CntHouse_16664.root");
  // TTree *t1 = (TTree*)f->Get("evt");

  if (myfile == NULL) open_myfile();

  Double_t block_period=evt_period/4;
  
  if (frequency_lim < 0)
    frequency_lim = 0.5/block_period;

  //  The data structure in the tree for each branch has 13 Double_t values.  We will
  //  just want to take the first one "[0]".
   Double_t chan_data[13];
   mytree->SetBranchAddress(chan_name,&chan_data);

   Long64_t nentries = mytree->GetEntries();
   Long64_t maxevent = first_event + num_events;
   if (maxevent>nentries){
     maxevent   = nentries;
     num_events = maxevent - first_event;
   }
   //Allocate an array big enough to hold the transform output
   //Transform output in 1d contains, for a transform of size N,
   //N/2+1 complex numbers, i.e. 2*(N/2+1) real numbers
   //our transform is of size n+1, because the histogram has n+1 bins
   Int_t n=num_events*4;
   Double_t x;
   Double_t *in = new Double_t[2*((n+1)/2+1)];
   Double_t re_2,im_2;

   for (Int_t i=0;i<2*((n+1)/2+1);i++){
     in[i]=0;
   }
   
   Int_t ii=0;
   Double_t meanval=0.0;
   for (Long64_t i=first_event;i<maxevent;i++) {
     mytree->GetEntry(i);
     //  At this point, the variable "chan_data[0]" will have the value for the event "i".
     //  You could then assign that to the data array that we'd use for the FFT.
     //  if (i%100==0) std::cout << "i=="<<i << " chan_data=="<< chan_data[0] <<std::endl;
     in[ii++]=chan_data[1];
     meanval += chan_data[1];
     in[ii++]=chan_data[2];
     meanval += chan_data[2];
     in[ii++]=chan_data[3];
     meanval += chan_data[3];
     in[ii++]=chan_data[4];
     meanval += chan_data[4];
   }
   meanval /= ii;
   for (Int_t i=0; i<ii; i++){
     in[i] -= meanval;
   }

   // Histograms
   // =========
   //prepare the canvas for drawing
   TF1 *fsin = new TF1("fsin", "sin(x)+sin(2*x)+sin(0.5*x)+1", 0, 4*TMath::Pi());

// Data array - same transform
// ===========================
 
   
   //Make our own TVirtualFFT object (using option "K")
   //Third parameter (option) consists of 3 parts:
   //- transform type:
   // real input/complex output in our case
   //- transform flag:
   // the amount of time spent in planning
   // the transform (see TVirtualFFT class description)
   //- to create a new TVirtualFFT object (option "K") or use the global (default)
   Int_t n_size = n+1;
   TVirtualFFT *fft_own = TVirtualFFT::FFT(1, &n_size, "R2C ES K");
   if (!fft_own) return;
   fft_own->SetPoints(in);
   fft_own->Transform();
 
   //Copy all the output points:
   fft_own->GetPoints(in);
   //Draw the real part of the output
   TH1 *hr = nullptr;
   hr = TH1::TransformHisto(fft_own, hr, "MAG");
   hr->SetTitle("Magnitude of the 3rd (array) transform");
   hr->Draw();
   hr->SetStats(kFALSE);
   hr->GetXaxis()->SetLabelSize(0.05);
   hr->GetYaxis()->SetLabelSize(0.05);
   //   hr->GetXaxis()->SetRangeUser(0,300);

   std::cout << "Bins in Magnitude Histogram" << hr->GetNbinsX()
 << endl; 

   TString histname(Form("FFTmag%d",plot_counter++));
   TH1D *FFTmag = new TH1D(histname, histname, hr->GetNbinsX(), 0, 1.0/block_period);

   for (Int_t j=0;j<hr->GetNbinsX();j++) {
     FFTmag->SetBinContent(j,hr->GetBinContent(j)/sqrt(n));
   }
   FFTmag->SetTitle(Form("FFT Magnitude for Sub Blocks of %s in range %d-%lld of run %d",chan_name.Data(),first_event,maxevent,run_number));
   // if (frequency_lim > .5/block_period)
   //  frequency_lim = .5/block_period;
   FFTmag->GetXaxis()->SetRangeUser(0,frequency_lim);
   FFTmag->Draw();

  
   
   delete fft_own;
   delete [] in;
   // delete [] re_full;
   // delete [] im_full;
}
  
  

