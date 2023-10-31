# REQUIRED PACKAGES
import ROOT
#import dunestyle.root as dunestyle

path = '/minerva/data/users/rvizarr/minervacal/rawfiles/'
chain = ROOT.TChain("nt", "nt")
chain.Add(path+'*.root')

outputfile = ROOT.TFile("combined_files.root", "RECREATE")
chain.Write()
outputfile.Close()

#canvas = ROOT.TCanvas("canvas", "Plot")
#chain.Draw("st_mev:st_base","st_path>0 && st_mev<3 && st_mev>0", "colz")
#canvas.Draw()
#canvas.SaveAs("myCanvas.png")
#canvas.Draw()
