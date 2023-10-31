
double MyFit(TH2 * h2BaseLpos, TH1 ** h1Base_return, char *ModTitle, int i = -1)
{
  TCanvas * c2 = new TCanvas("dummy", "dummy");
  c2->cd();

  int NumberLposBins = h2BaseLpos->GetYaxis()->GetNbins();
  int NumberRotationPoints = 6;
  int NumberRotationBins = NumberLposBins / NumberRotationPoints;

  char H1Name[40];
  char F1Name[40];

  TH1 * h1Base;
  if (i >= 0 && i < NumberRotationPoints) {
    sprintf(H1Name, "%s_rot%d", ModTitle, i);
    sprintf(F1Name, "%s_fit_rot%d", ModTitle, i);
    int minbin = i * NumberRotationBins;
    int maxbin = minbin + NumberRotationBins;
    h1Base = h2BaseLpos->ProjectionX(H1Name, minbin, maxbin);
  } else {
    sprintf(H1Name, "%s_shift", ModTitle);
    sprintf(F1Name, "%s_fit", ModTitle);
    h1Base = h2BaseLpos->ProjectionX(H1Name);
  }
  h1Base->Scale(1.0/NumberLposBins);
  h1Base->SetName(H1Name);
  h1Base->SetTitle(H1Name);
  h1Base->GetYaxis()->SetTitle("Energy (MeV)");

  TF1 * f1 = new TF1(F1Name, TriangleFunction, -17.0, +17.0, 2);
  f1->SetParameter(0, 3.5);
  f1->SetParameter(1, 0.0);
  h1Base->Fit(F1Name, "Q", "Q");
  double shift = f1->GetParameter(1);
  h1Base->Fit(F1Name, "Q", "Q", shift - 10.0, shift + 10.0);
  shift = f1->GetParameter(1);

  if (h1Base_return != 0) *h1Base_return = h1Base;

  return shift;
}