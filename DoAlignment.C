//
// DoAlignment.C
//
// input: ReadNT.root from ReadNT.C
//
// output: AlignmentConstants.txt
//
// To put alignment constants in geometry format:
//   python AlignmentXMLPrinter.py > Planes.xml
// 

double TriangleFunction(double * u, double * par)
{
  double x = u[0];
  double height = par[0];
  double shift = par[1];
  x -= shift;
  if (fabs(x) > 17.) return 0.;
  else return height * (17. - fabs(x));
}

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

void DoAlignment()
{
  FILE * fptext = fopen("AlignmentConstants.txt", "w");

  TFile * fin = new TFile("ReadNT.root");
  TProfile3D * moduleLposBase = (TProfile3D *)fin->Get("moduleLposBase");

  moduleLposBase->GetYaxis()->SetTitle("lpos");
  moduleLposBase->GetZaxis()->SetTitle("triangle base");

  TCanvas * c1 = new TCanvas("c1", "c1");
  c1->Divide(2,2);
  c1->Print("AlignmentBook.pdf[");

  for (int modbin = 1; modbin <= 240; ++modbin) // From 1 to 240
  {
    double module_number = moduleLposBase->GetXaxis()->GetBinLowEdge(modbin);
    int module = (int) (5 + module_number) - 5;
    int plane = (int(2 * (module_number + 5)) % 2) + 1;

    printf("Doing alignment for module %d plane %d\n",module,plane);

    if (plane == 1) c1->Clear("D");

    char ModTitle[40];
    if (module >= 0) sprintf(ModTitle, "mod%03dpl%1d", module, plane);
    if (module <  0) sprintf(ModTitle, "modm%1dpl%1d", module, plane);   
 
    moduleLposBase->GetXaxis()->SetRange(modbin, modbin);

    char H2Name[40];
    sprintf(H2Name, "%s_2D", ModTitle);
    TH2 * h2BaseLpos = moduleLposBase->Project3D("yz");
    h2BaseLpos->SetName(H2Name);
    h2BaseLpos->SetTitle(Form("Module %d Plane %d;Base Position (mm);Longitudinal Position (mm);Average Energy (MeV)",module,plane));

    if (h2BaseLpos->GetEntries() == 0) continue;

    TH1 * h1Base;
    double shift = -MyFit(h2BaseLpos, &h1Base, ModTitle);

    double lpos_point[6], shift_point[6];
    for (int i = 0; i < 6; ++i)
    {
      lpos_point[i] = -1200.0 + (400.0 * i) + 200.0;
      shift_point[i] = MyFit(h2BaseLpos, 0, ModTitle, i);
    }
    TGraph * tgrRotation = new TGraph(6, shift_point, lpos_point);
    TGraph * tgrFit = new TGraph(6, lpos_point, shift_point);
    tgrFit->Fit("pol1","Q","Q"); 
    double LowPoint = tgrFit->GetFunction("pol1")->Eval(-1000.0);
    double HighPoint = tgrFit->GetFunction("pol1")->Eval(+1000.0);
    double rotation = atan((HighPoint - LowPoint) / 2000.0);

    c1->cd(2*(plane-1) + 1);
    h2BaseLpos->SetMaximum(4);
    h2BaseLpos->Draw("colz");
    tgrRotation->SetMarkerStyle(kOpenCircle);
    tgrRotation->Draw("P same");
    TLine * tline = new TLine(LowPoint, -1000.0, HighPoint, +1000.0);
    tline->SetLineWidth(3);
    tline->Draw("same");
    c1->cd(2*(plane-1) + 2);
    h1Base->SetMaximum(4);
    h1Base->SetTitle(Form("Module %d Plane %d;Base Position (mm);Average Energy (MeV)",module, plane));
    h1Base->Draw();
    char ModString[40]; sprintf(ModString, "Module %d Plane %d", module, plane);
    char ShiftString[40]; sprintf(ShiftString, "Shift = %+5.3f mm", shift);
    char RotationString[40]; sprintf(RotationString, "Rotation = %+5.3f mrad\n", 1000*rotation);
    TPaveLabel * ModLabel      = new TPaveLabel(5.0, 4.00, 17.0, 3.75, ModString);
    TPaveLabel * ShiftLabel    = new TPaveLabel(5.0, 3.75, 17.0, 3.50, ShiftString);
    TPaveLabel * RotationLabel = new TPaveLabel(5.0, 3.50, 17.0, 3.25, RotationString);
    ModLabel->Draw();
    ShiftLabel->Draw();
    RotationLabel->Draw();
    if (plane == 2) c1->Print("AlignmentBook.pdf");

    TCanvas * c2 = new TCanvas("c2","c2", 800, 500);
    c2->Divide(2,1);
    c2->cd(1);
    h1Base->Draw();
    c2->cd(2);
    h2BaseLpos->Draw("colz");
    tgrRotation->Draw("P same");
    tline->Draw("same");
    c2->Print(Form("alignmentPlotDump/mod%03dpl%d.png",module,plane));
    delete c2;

    fprintf(fptext, "module: %d plane: %d shift: %f rotation: %f\n", module, plane, shift, rotation);
  }

  c1->Print("AlignmentBook.pdf]");
  fclose(fptext);
}
