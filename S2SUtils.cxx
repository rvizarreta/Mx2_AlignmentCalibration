#include <TChain.h>
#include <fstream>
#include <sstream>
#include <string>

void GetFilenames( std::vector<std::string> &fnames, int &min_run, int &max_run )
{

  fnames.clear();
  max_run = 0;
  min_run = 99999999;

  std::ifstream ifs("playlist.txt");

  if( ifs.is_open() ){

    std::string s;
    while( getline(ifs,s) ){
      if(s[0]=='#') continue;

      int run, sub;
      std::string name;

      std::istringstream sin(s);
      sin >> run >> sub >> name;

      if( run < min_run ) min_run = run;
      if( run > max_run ) max_run = run;

      fnames.push_back( name );
    }
  }
}

void SetBranches( TTree * nt )
{
  nt->SetBranchStatus("*", 0);
  nt->SetBranchStatus("n_entries", 1);
  nt->SetBranchStatus("ev_run", 1);
  nt->SetBranchStatus("ev_subrun", 1);
  nt->SetBranchStatus("ev_gate", 1);
  nt->SetBranchStatus("ev_ntracks", 1);
  nt->SetBranchStatus("ev_extraEnergy", 1);
  nt->SetBranchStatus("st_strip", 1);
  nt->SetBranchStatus("st_plane", 1);
  nt->SetBranchStatus("st_module", 1);
  nt->SetBranchStatus("st_crate", 1);
  nt->SetBranchStatus("st_croc", 1);
  nt->SetBranchStatus("st_chain", 1);
  nt->SetBranchStatus("st_board", 1);
  nt->SetBranchStatus("st_pixel", 1);
  nt->SetBranchStatus("st_lpos", 1);
  nt->SetBranchStatus("st_path", 1);
  nt->SetBranchStatus("st_base", 1);
  nt->SetBranchStatus("st_mev", 1);
  nt->SetBranchStatus("st_q", 1);
  nt->SetBranchStatus("st_pe", 1);
}
    
