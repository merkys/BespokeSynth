/*
  ==============================================================================

    RadioSequencer.cpp
    Created: 10 Jun 2017 4:53:15pm
    Author:  Ryan Challinor

  ==============================================================================
*/

#include "RadioSequencer.h"
#include "ModularSynth.h"
#include "PatchCableSource.h"

RadioSequencer::RadioSequencer()
: mGrid(NULL)
, mInterval(kInterval_1n)
, mIntervalSelector(NULL)
, mLength(4)
, mLengthSelector(NULL)
{
   TheTransport->AddListener(this, mInterval);
}

RadioSequencer::~RadioSequencer()
{
   TheTransport->RemoveListener(this);
}

void RadioSequencer::CreateUIControls()
{
   IDrawableModule::CreateUIControls();
   mGrid = new Grid(5,23,200,170,mLength,8);
   mIntervalSelector = new DropdownList(this,"interval",5,3,(int*)(&mInterval));
   mLengthSelector = new DropdownList(this,"length",45,3,(int*)(&mLength));
   
   mGrid->SetHighlightCol(-1);
   mGrid->SetSingleColumnMode(true);
   mGrid->SetMajorColSize(4);
   mGrid->SetListener(this);
   
   /*mIntervalSelector->AddLabel("8", kInterval_8);
    mIntervalSelector->AddLabel("4", kInterval_4);
    mIntervalSelector->AddLabel("2", kInterval_2);*/
   mIntervalSelector->AddLabel("1n", kInterval_1n);
   mIntervalSelector->AddLabel("2n", kInterval_2n);
   mIntervalSelector->AddLabel("4n", kInterval_4n);
   mIntervalSelector->AddLabel("4nt", kInterval_4nt);
   mIntervalSelector->AddLabel("8n", kInterval_8n);
   mIntervalSelector->AddLabel("8nt", kInterval_8nt);
   mIntervalSelector->AddLabel("16n", kInterval_16n);
   mIntervalSelector->AddLabel("16nt", kInterval_16nt);
   mIntervalSelector->AddLabel("32n", kInterval_32n);
   mIntervalSelector->AddLabel("64n", kInterval_64n);
   
   mLengthSelector->AddLabel("4", 4);
   mLengthSelector->AddLabel("6", 6);
   mLengthSelector->AddLabel("8", 8);
   mLengthSelector->AddLabel("16", 16);
   mLengthSelector->AddLabel("32", 32);
   mLengthSelector->AddLabel("64", 64);
   mLengthSelector->AddLabel("128", 128);
   
   SyncControlCablesToGrid();
}

void RadioSequencer::Init()
{
   IDrawableModule::Init();
}

void RadioSequencer::Poll()
{
}

void RadioSequencer::OnTimeEvent(int samplesTo)
{
   int stepsPerMeasure = TheTransport->CountInStandardMeasure(mInterval) * TheTransport->GetTimeSigTop()/TheTransport->GetTimeSigBottom();
   int numMeasures = MAX(1,ceil(float(mGrid->GetCols()) / stepsPerMeasure));
   int measure = TheTransport->GetMeasure() % numMeasures;
   int step = (TheTransport->GetQuantized(0, mInterval) + measure * stepsPerMeasure) % mGrid->GetCols();
   
   mGrid->SetHighlightCol(step);
   
   IUIControl* controlToEnable = NULL;
   for (int i=0; i<mControlCables.size(); ++i)
   {
      IUIControl* uicontrol = NULL;
      if (mControlCables[i]->GetTarget())
         uicontrol = dynamic_cast<IUIControl*>(mControlCables[i]->GetTarget());
      if (uicontrol)
      {
         if (mGrid->GetVal(step, i) > 0)
            controlToEnable = uicontrol;
         else
            uicontrol->SetValue(0);
      }
   }
   
   if (controlToEnable)
      controlToEnable->SetValue(1);
}

void RadioSequencer::DrawModule()
{
   if (Minimized() || IsVisible() == false)
      return;
   
   mGrid->Draw();
   mIntervalSelector->Draw();
   mLengthSelector->Draw();
   
   for (int i=0; i<mControlCables.size(); ++i)
   {
      mControlCables[i]->SetManualPosition(GetRect(true).width, mGrid->GetPosition().y+(mGrid->GetHeight()/mGrid->GetRows())*(i+.5f));
   }
}

void RadioSequencer::OnClicked(int x, int y, bool right)
{
   IDrawableModule::OnClicked(x,y,right);
   
   mGrid->TestClick(x, y, right);
}

void RadioSequencer::MouseReleased()
{
   IDrawableModule::MouseReleased();
   mGrid->MouseReleased();
}

bool RadioSequencer::MouseMoved(float x, float y)
{
   IDrawableModule::MouseMoved(x,y);
   mGrid->NotifyMouseMoved(x,y);
   return false;
}

void RadioSequencer::SetNumSteps(int numSteps, bool stretch)
{
   int oldNumSteps = mGrid->GetCols();
   assert(numSteps != 0);
   assert(oldNumSteps != 0);
   if (stretch)   //updated interval, stretch old pattern out to make identical pattern at higher res
   {              // abcd becomes aabbccdd
      vector<float> pattern;
      pattern.resize(oldNumSteps);
      for (int i=0; i<oldNumSteps; ++i)
         pattern[i] = mGrid->GetVal(i,0);
      float ratio = float(numSteps)/oldNumSteps;
      for (int i=0; i<numSteps; ++i)
         mGrid->SetVal(i, 0, pattern[int(i/ratio)]);
   }
   else           //updated length, copy old pattern out to make identical pattern over longer time
   {              // abcd becomes abcdabcd
      int numCopies = numSteps / oldNumSteps;
      for (int i=1; i<numCopies; ++i)
      {
         for (int j=0; j<oldNumSteps; ++j)
            mGrid->SetVal(i*oldNumSteps + j, 0, mGrid->GetVal(j, 0));
      }
   }
   mGrid->SetGrid(numSteps,mGrid->GetRows());
}

void RadioSequencer::GridUpdated(Grid* grid, int col, int row, float value, float oldValue)
{
   if (grid == mGrid)
   {
   }
}

void RadioSequencer::PostRepatch(PatchCableSource* cableSource)
{
}

void RadioSequencer::SyncControlCablesToGrid()
{
   if (mGrid->GetRows() == mControlCables.size())
      return;  //nothing to do
   
   if (mGrid->GetRows() > mControlCables.size())
   {
      int oldSize = mControlCables.size();
      mControlCables.resize(mGrid->GetRows());
      for (int i=oldSize; i<mControlCables.size(); ++i)
      {
         mControlCables[i] = new PatchCableSource(this, kConnectionType_UIControl);
         //mControlCables[i]->SetColor(GetRowColor(i));
         AddPatchCableSource(mControlCables[i]);
      }
   }
   else
   {
      for (int i=mGrid->GetRows(); i<mControlCables.size(); ++i)
         RemovePatchCableSource(mControlCables[i]);
      mControlCables.resize(mGrid->GetRows());
   }
}

void RadioSequencer::DropdownUpdated(DropdownList* list, int oldVal)
{
   int newSteps = int(mLength * TheTransport->CountInStandardMeasure(mInterval));
   if (list == mIntervalSelector)
   {
      if (newSteps > 0)
      {
         TheTransport->UpdateListener(this, mInterval);
         SetNumSteps(newSteps, true);
      }
      else
      {
         mInterval = (NoteInterval)oldVal;
      }
   }
   if (list == mLengthSelector)
   {
      if (newSteps > 0)
         SetNumSteps(newSteps, false);
      else
         mLength = oldVal;
   }
}

namespace
{
   const float extraW = 10;
   const float extraH = 30;
}

void RadioSequencer::GetModuleDimensions(int &width, int &height)
{
   width = mGrid->GetWidth() + extraW;
   height = mGrid->GetHeight() + extraH;
}

void RadioSequencer::Resize(float w, float h)
{
   w = MAX(w - extraW, 200);
   h = MAX(h - extraH, 170);
   SetGridSize(w,h);
}

void RadioSequencer::SetGridSize(float w, float h)
{
   mGrid->SetDimensions(w, h);
}

void RadioSequencer::SaveLayout(ofxJSONElement& moduleInfo)
{
   IDrawableModule::SaveLayout(moduleInfo);
}

void RadioSequencer::LoadLayout(const ofxJSONElement& moduleInfo)
{
   mModuleSaveData.LoadFloat("gridwidth", moduleInfo, 200, 200, 1000);
   mModuleSaveData.LoadFloat("gridheight", moduleInfo, 170, 170, 1000);
   
   SetUpFromSaveData();
}

void RadioSequencer::SetUpFromSaveData()
{
   SetGridSize(mModuleSaveData.GetFloat("gridwidth"), mModuleSaveData.GetFloat("gridheight"));
}

namespace
{
   const int kSaveStateRev = 0;
}

void RadioSequencer::SaveState(FileStreamOut& out)
{
   IDrawableModule::SaveState(out);
   
   out << kSaveStateRev;
   
   out << (int)mControlCables.size();
   for (auto cable : mControlCables)
   {
      string path = "";
      if (cable->GetTarget())
         path = cable->GetTarget()->Path();
      out << path;
   }
   
   mGrid->SaveState(out);
}

void RadioSequencer::LoadState(FileStreamIn& in)
{
   IDrawableModule::LoadState(in);
   
   int rev;
   in >> rev;
   LoadStateValidate(rev == kSaveStateRev);
   
   int size;
   in >> size;
   mControlCables.resize(size);
   for (auto cable : mControlCables)
   {
      string path;
      in >> path;
      cable->SetTarget(TheSynth->FindUIControl(path));
   }
   
   mGrid->LoadState(in);
}