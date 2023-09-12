#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TProfile3D.h>
#include <TProfile2D.h>
#include <TProfile.h>
#include <TTree.h>
#include <TChain.h>
#include <string>
#include <map>
#include <math.h>
#include "S2SUtils.cxx"

//==================================================================================================
// ReadNT.C
// input: playlist.txt, s2s ntuple files
// output: ReadNT.root
// Determines iterative truncated mean energy in each channel
// Output histograms are used by MakeSummaryTuple.C to get dE/dX and identify error strips
//==================================================================================================

//==================================================================================================
// Declare global variables and histograms
//==================================================================================================
const int NENTRIES = 1000;
const int n_iterations = 8;
double TM[120][2][127] = {{{0.0}}}; // store the running truncated mean for [module+5][plane-1][strip-1]
int min_run;
int max_run;

TFile * fout; // Output ROOT file

// Histograms used for alignment only -- not needed for s2s
TProfile3D * p_moduleLposBase;
TProfile2D * p_shifts;

// Histograms for sanity checking -- not needed for anything
TH2 * h_energy_vs_path;
TProfile * p_energy_vs_path;
TH2 * h_energy_over_path;
TProfile * p_energy_over_path;

// Histograms used in s2s
TH2 * h_TM_energy_channel; // this is where the iterative TM is stored
TProfile2D * p_energy_channel_all; // used for RMS, includes 0 entries in full mean
TH2 * h_zero_fraction; // fraction of path lengths with zero energy
TH1 * chanEnergy[120][2][127]; // used for the iteration

// For Howard, also not used for anything
TH2 * nHits[2][8];
TProfile2D * e_avgMeV[2][8];
TProfile2D * e_avgPE[2][8];
TProfile2D * e_avgQ[2][8];

//==================================================================================================
// Book histograms
//==================================================================================================
void BookHistos()
{
  fout = new TFile("ReadNT.root", "RECREATE");

  p_moduleLposBase = new TProfile3D("moduleLposBase", "Energy/Path;Module;LPos;Base", 240, -5, 115, 24, -1200, 1200, 68, -17, 17);
  p_shifts = new TProfile2D("shifts", "Peak base position;Module;Strip", 240, -5, 115, 127, 1, 128);
  h_energy_vs_path = new TH2I("h_energy_path", ";Path length (cm);Energy (MeV)", 90, 0.2, 2, 100, 0, 10);
  p_energy_vs_path = new TProfile("p_energy_path", ";Path length (cm);Energy (MeV)", 90, 0.2, 2);
  h_energy_over_path = new TH2I("h_energy_over_path", ";Path length (cm);Energy per path length (MeV/cm)", 90, 0.2, 2, 100, 0, 10);
  p_energy_over_path = new TProfile("p_energy_over_path", ";Path length (cm);Energy per path length (MeV/cm)", 90, 0.2, 2);
  h_TM_energy_channel = new TH2D("TM_energy_channel", "Truncated mean energy/path (MeV/cm);Module;Strip", 240, -5, 115, 127, 1, 128);
  p_energy_channel_all = new TProfile2D("p_energy_channel_all", "Full mean energy/path (MeV/cm);Module;Strip", 240, -5, 115, 127, 1, 128);
  h_zero_fraction = new TH2D("h_zero_fraction", "Fraction of tracks with zero energy;Module;Strip", 240, -5, 115, 127, 1, 128);
  for( int i = -5; i < 115; i++ ) {
    for( int j = 1; j <= 2; j++ ) {
      for( int k = 1; k <= 127; k++ ) {
        int modidx = i+5;
        int plidx = j-1;
        int stridx = k-1;
        chanEnergy[modidx][plidx][stridx] = new TH1D(Form("mod%03d_pl%01d_str%03d",i,j,k),Form("mod%03d_pl%01d_str%03d;Energy (pseudo-MeV)",i,j,k),20000,0.0,20.0);
      }
    }
  }

  for( int crate = 0; crate < 2; ++crate ) {
    for( int croc = 0; croc < 8; ++croc ) {
      nHits[crate][croc] = new TH2D( Form("nh_crate%d_croc%d",crate,croc+1), Form("N Hits Crate %d Croc %d;Board;Chain",crate,croc+1), 80, 1.0, 11.0, 32, 0.0, 4.0 );
      e_avgMeV[crate][croc] = new TProfile2D( Form("mev_crate%d_croc%d",crate,croc+1), Form("Avg. MeV Crate %d Croc %d;Board;Chain",crate,croc+1), 80, 1.0, 11.0, 32, 0.0, 4.0 );
      e_avgPE[crate][croc] = new TProfile2D( Form("pe_crate%d_croc%d",crate,croc+1), Form("Avg. PE Crate %d Croc %d;Board;Chain",crate,croc+1), 80, 1.0, 11.0, 32, 0.0, 4.0 );
      e_avgQ[crate][croc] = new TProfile2D( Form("q_crate%d_croc%d",crate,croc+1), Form("Avg. Q Crate %d Croc %d;Board;Chain",crate,croc+1), 80, 1.0, 11.0, 32, 0.0, 4.0 );
    }
  }

}

//==================================================================================================
// Loop over rock muon tracks and fill histograms
//==================================================================================================
void LoopOverTracks( TTree * nt )
{
  int NN = nt->GetEntries();

  int n_entries, run, subrun, nTracks;
  double extraE;
  int strip[NENTRIES], plane[NENTRIES], module[NENTRIES];
  int crate[NENTRIES], croc[NENTRIES], chain[NENTRIES], board[NENTRIES], pixel[NENTRIES];
  double lpos[NENTRIES], path[NENTRIES], base[NENTRIES], E[NENTRIES], pe[NENTRIES], q[NENTRIES];
  nt->SetBranchAddress("n_entries", &n_entries);
  nt->SetBranchAddress("ev_run", &run);
  nt->SetBranchAddress("ev_subrun", &subrun);
  nt->SetBranchAddress("ev_ntracks", &nTracks);
  nt->SetBranchAddress("ev_extraEnergy", &extraE);
  nt->SetBranchAddress("st_strip", strip);
  nt->SetBranchAddress("st_plane", plane);
  nt->SetBranchAddress("st_module", module);
  nt->SetBranchAddress("st_crate", crate);
  nt->SetBranchAddress("st_croc", croc);
  nt->SetBranchAddress("st_chain", chain);
  nt->SetBranchAddress("st_board", board);
  nt->SetBranchAddress("st_pixel", pixel);
  nt->SetBranchAddress("st_lpos", lpos);
  nt->SetBranchAddress("st_path", path);
  nt->SetBranchAddress("st_base", base);
  nt->SetBranchAddress("st_mev", E);
  nt->SetBranchAddress("st_pe", pe);
  nt->SetBranchAddress("st_q", q);

  // ntuple has a bunch of other branches used in monitoring plots; turn them off to save time
  SetBranches( nt );

  for (int ii = 0; ii < NN; ++ii) {
    nt->GetEntry(ii);
    if( ii == 0 ) printf("Processing %d tracks for %d/%d\n", NN, run, subrun);

    // Require clean events -- single track with very little extra energy
    if( nTracks > 1 || extraE > 100.0 ) continue;

    //----------------------------------------------------------------------------------------------
    // Fill number of rock muons for each run
    //----------------------------------------------------------------------------------------------
    for( int i = 0; i < n_entries; ++i ) {
      double path_cm = 0.1 * path[i];
      if( path_cm < 0.2 ) continue; // we divide by this, so make sure it's not near 0

      //--------------------------------------------------------------------------------------------
      // Fill alignment histograms with normal incidence correction factor C
      //--------------------------------------------------------------------------------------------
      double C = (17-fabs(base[i]))/path[i];
      p_moduleLposBase->Fill(module[i] + 0.5 * (plane[i] - 1), lpos[i], base[i], E[i]*C);

      //p3ModLposBase->Fill(module[i] + 0.5 * (plane[i] - 1), lpos[i], base[i], E[i]*C);
      
      p_shifts->Fill(module[i] + 0.5 * (plane[i] - 1), strip[i], base[i], E[i]*C);

      //--------------------------------------------------------------------------------------------
      // Sanity check histograms - not actually used for anything
      //--------------------------------------------------------------------------------------------
      if( E[i] > 0.0 ) {
        p_energy_vs_path->Fill(path_cm, E[i]);
        h_energy_vs_path->Fill(path_cm, E[i]);
        p_energy_over_path->Fill(path_cm, E[i]/path_cm);
        h_energy_over_path->Fill(path_cm, E[i]/path_cm);
      }

      //--------------------------------------------------------------------------------------------
      // Determine index to chanEnergy for this channel
      //--------------------------------------------------------------------------------------------
      int modidx = module[i]+5;
      int plidx = plane[i]-1;
      int stridx = strip[i]-1;

      //--------------------------------------------------------------------------------------------
      // Fill truncated mean histogram for non-zero energy
      //--------------------------------------------------------------------------------------------
      if( E[i] != 0.0 ) chanEnergy[modidx][plidx][stridx]->Fill(E[i]/path_cm);
      else h_zero_fraction->Fill(module[i] + 0.5 * (plane[i] - 1), strip[i]); // count number of events for now

      //--------------------------------------------------------------------------------------------
      // Fill histograms to get full mean and mean above truncation to ID error strips
      //--------------------------------------------------------------------------------------------
      p_energy_channel_all->Fill(module[i] + 0.5 * (plane[i] - 1), strip[i], E[i]/path_cm);

      int column = pixel[i] % 8;
      int row = pixel[i] / 8;

      double xx = board[i] + column/8.0;
      double yy = chain[i] + row/8.0;

      nHits[crate[i]][croc[i]-1]->Fill( xx, yy );
      e_avgMeV[crate[i]][croc[i]-1]->Fill( xx, yy, E[i] );
      e_avgPE[crate[i]][croc[i]-1]->Fill( xx, yy, pe[i] );
      e_avgQ[crate[i]][croc[i]-1]->Fill( xx, yy, q[i] );

    } // hits

  } // rock muons

}

void FillHistograms()
{
  //------------------------------------------------------------------------------------------------
  // Iterate over energy histogram for each channel to find truncated mean
  //------------------------------------------------------------------------------------------------
  printf("Determining truncated mean with %d iterations...\n",n_iterations);
  for( int it = 0; it < n_iterations; it++ ) {
    for( int mod = 0; mod < 120; mod++ ) {
      for( int pl = 0; pl <= 1; pl++ ) {
        for( int str = 0; str < 127; str++ ) {
          double upper = 1.5 * TM[mod][pl][str];
          double lower = 0.5 * TM[mod][pl][str];
          if( it == 0 ) upper = 20.0;
          if( chanEnergy[mod][pl][str]->GetEntries() > 0 ) {
            chanEnergy[mod][pl][str]->GetXaxis()->SetRangeUser(lower,upper); // refine range
            TM[mod][pl][str] = chanEnergy[mod][pl][str]->GetMean(); // new mean for next iteration
          } else TM[mod][pl][str] = 0.0; // kills this channel forever and ever
        } // strip
      } // plane
    } // module
  } // iteration

  //------------------------------------------------------------------------------------------------
  // Fill "summary" histograms which are input to MakeSummaryTuple.C
  //------------------------------------------------------------------------------------------------
  for( int mod = -5; mod <= 114; mod++ ) {
    if( mod % 10 == 0 ) printf("Filling summary histograms for module %d...\n",mod);
    for( int pl = 1; pl <= 2; pl++ ) {
      for( int str = 1; str <= 127; str++ ) {

        int modidx = mod+5;
        int plidx = pl-1;
        int stridx = str-1;

        int bin = p_energy_channel_all->FindBin( mod+0.5*(pl-1), str ); // binning is the same for all three plots

        double mean = chanEnergy[modidx][plidx][stridx]->GetMean();
        h_TM_energy_channel->SetBinContent( bin, mean );
        chanEnergy[modidx][plidx][stridx]->GetXaxis()->SetRangeUser( 0.0, 20.0 );
        double stdev = chanEnergy[modidx][plidx][stridx]->GetRMS();
        h_TM_energy_channel->SetBinError( bin, stdev ); // store sigma for mean +/- 50%

        double nzero = h_zero_fraction->GetBinContent( bin );
        int nEnt = p_energy_channel_all->GetBinEntries( bin ); // includes zero entries
        if( nEnt != 0 ) h_zero_fraction->SetBinContent( bin, nzero/nEnt );
        // For example when we get to module 9 which is a passive target
        else h_zero_fraction->SetBinContent( bin, 0.0 );

        delete chanEnergy[modidx][plidx][stridx]; // maybe we actually have to be smart with memory?
      }
    }
  }

}

void MakeTimeHeader( TChain * header )
{

  // Use the header tree to write out the start and end time in microseconds for the IOV to a text file
  // Later, we will convert this to human readable, but in python
  // note: tea time is good, but ROOT::TTime is evil

  int NFiles = header->GetEntries(); // number of subruns, to be used for bins
  printf( "There are %d files\n", NFiles );

  ULong64_t start_time, end_time;
  header->SetBranchAddress( "begin_gps_time", &start_time ); // in microseconds
  header->SetBranchAddress( "end_gps_time", &end_time ); // in microseconds

  long int begin_us = 9999999999999999;
  long int end_us = 0;

  // assume it's in order
  header->GetEntry(0);
  begin_us = start_time;
  header->GetEntry(NFiles-1);
  end_us = end_time;

  FILE * time = fopen( "time_header.txt", "w" );
  fprintf( time, "%ld %ld\n", begin_us, end_us );
  fclose( time );

}

//==================================================================================================
// Main execution: read ROOT files in from playlist and loop over ntuples
//==================================================================================================
void ReadNT()
{
  // Histograms
  BookHistos();

  // Get the ntuples
  TChain * header = new TChain("header","header");

  printf( "Getting ntuples...\n" );
  std::vector<std::string> fnames;
  int min_run, max_run;
  GetFilenames( fnames, min_run, max_run );
  
  for( unsigned int i = 0; i < fnames.size(); ++i ) {

    // See if the TFile is OK for the file name
    TFile * test_file = new TFile( fnames[i].c_str(), "OLD" );
    if( test_file == NULL ) {
      delete test_file;
      continue;
    }

    // See if the nt tree is OK
    TTree * tree = (TTree*)test_file->Get( "nt" );
    if( tree == NULL ) continue;

    LoopOverTracks( tree );
    header->Add( fnames[i].c_str() );

    test_file->Close();
    delete test_file;
  }

  printf( "Make time header...\n" );
  MakeTimeHeader( header );

  printf( "Filling summary histograms...\n" );
  FillHistograms();

  //------------------------------------------------------------------------------------------------
  // Save output to ReadNT.root
  //------------------------------------------------------------------------------------------------
  fout->cd();
  p_moduleLposBase->Write();
  p_shifts->Write();
  p_energy_vs_path->Write();
  h_energy_vs_path->Write();
  p_energy_over_path->Write();
  h_energy_over_path->Write();
  h_TM_energy_channel->Write();
  p_energy_channel_all->Write();
  h_zero_fraction->Write();

  for( int crate = 0; crate < 2; ++crate ) {
    for( int croc = 0; croc < 8; ++croc ) {
      nHits[crate][croc]->Write();
      e_avgMeV[crate][croc]->Write();
      e_avgPE[crate][croc]->Write();
      e_avgQ[crate][croc]->Write();
    }
  }  

}

int main()
{
  ReadNT();
  return 0;
}

