//____________________________________________________________________________
/*!

\class   genie::GHepRecord

\brief   Generated Event Record: STDHEP-like record and Summary Information.

\author  Costas Andreopoulos <C.V.Andreopoulos@rl.ac.uk>
         CCLRC, Rutherford Appleton Laboratory

\created October 1, 2004

*/
//____________________________________________________________________________

#include <cassert>
#include <algorithm>
#include <vector>
#include <iomanip>

#include <TLorentzVector.h>

#include "GHEP/GHepParticle.h"
#include "GHEP/GHepRecord.h"
#include "GHEP/GHepStatus.h"
#include "Messenger/Messenger.h"
#include "PDG/PDGUtils.h"
#include "Utils/PrintUtils.h"

using std::vector;
using std::endl;
using std::setw;
using std::setprecision;
using std::setfill;
using std::ios;

using namespace genie;

ClassImp(GHepRecord)

//___________________________________________________________________________
namespace genie {
 ostream & operator << (ostream & stream, const GHepRecord & rec)
 {
   rec.Print(stream);

   return stream;
 }
}
//___________________________________________________________________________
GHepRecord::GHepRecord() :
TClonesArray("genie::GHepParticle")
{
  this->InitGHepRecord();
}
//___________________________________________________________________________
GHepRecord::GHepRecord(int size) :
TClonesArray("genie::GHepParticle", size)
{
  this->InitGHepRecord();
}
//___________________________________________________________________________
GHepRecord::GHepRecord(const GHepRecord & record) :
TClonesArray("genie::GHepParticle", record.GetEntries())
{
  this->InitGHepRecord();
  this->Copy(record);
}
//___________________________________________________________________________
GHepRecord::~GHepRecord()
{
  this->CleanUpGHepRecord();
}
//___________________________________________________________________________
Interaction * GHepRecord::GetInteraction(void) const
{
  if(!fInteraction)
  {
    LOG("GHEP", pWARN) << "Returning NULL interaction";
  }
  return fInteraction;
}
//___________________________________________________________________________
void GHepRecord::AttachInteraction(Interaction * interaction)
{
  fInteraction = interaction;
}
//___________________________________________________________________________
GHepParticle * GHepRecord::GetParticle(int position) const
{
// Returns the GHepParticle from the specified position of the event record.

  if( position >=0 && position < this->GetEntries() ) {

     GHepParticle * particle = (GHepParticle *) (*this)[position];
     if(particle) return particle;
  }
  LOG("GHEP", pWARN)
        << "No GHepParticle found with: (pos = "
                              << position << ") - Returning NULL";
  return 0;
}
//___________________________________________________________________________
GHepParticle * GHepRecord::FindParticle(int pdg, int status, int start) const
{
// Returns the first GHepParticle with the input pdg-code and status
// starting from the specified position of the event record.

  for(int i = start; i < this->GetEntries(); i++) {

     GHepParticle * p = (GHepParticle *) (*this)[i];
     if(p->Status() == status && p->PdgCode() == pdg) return p;
  }
  LOG("GHEP", pWARN)
        << "No GHepParticle found with: (pos >= " << start
        << ", pdg = " << pdg << ", ist = " << status << ") - Returning NULL";
  return 0;
}
//___________________________________________________________________________
int GHepRecord::ParticlePosition(int pdg, int status, int start) const
{
// Returns the position of the first GHepParticle with the input pdg-code
// and status starting from the specified position of the event record.

  for(int i = start; i < this->GetEntries(); i++) {

     GHepParticle * p = (GHepParticle *) (*this)[i];
     if(p->Status() == status && p->PdgCode() == pdg) return i;
  }
  LOG("GHEP", pWARN) << "Returning Invalid StdHep Record position";

  return -1;
}
//___________________________________________________________________________
int GHepRecord::ParticlePosition(GHepParticle * particle, int start) const
{
// Returns the position of the first match with the specified GHepParticle
// starting from the specified position of the event record.

  for(int i = start; i < this->GetEntries(); i++) {

     GHepParticle * p = (GHepParticle *) (*this)[i];
     if( p->Compare(particle) ) return i;
  }
  LOG("GHEP", pWARN) << "Returning Invalid StdHep Record position";

  return -1;
}
//___________________________________________________________________________
void GHepRecord::ShiftVertex(const TLorentzVector & vec4)
{
// Shifts the event record entries in space (used when events generated at
// the origin (0,0,0,0) are distributed within the volume described by an
// input ROOT geometry)

  LOG("GHEP", pNOTICE)
       << "Shifting vertex to: " << print_utils::X4AsString(&vec4);

  double x0 = vec4.X();
  double y0 = vec4.Y();
  double z0 = vec4.Z();
  double t0 = vec4.T();

  for(int i = 0; i < this->GetEntries(); i++) {

     GHepParticle * p = (GHepParticle *) (*this)[i];

     double vx = x0 + p->Vx();
     double vy = y0 + p->Vy();
     double vz = z0 + p->Vz();
     double vt = t0 + p->Vt();

     p->SetVertex(vx, vy, vz, vt);
  }
}
//___________________________________________________________________________
void GHepRecord::AddParticle(const GHepParticle & p)
{
// Provides a simplified method for inserting entries in the TClonesArray

  unsigned int pos = this->GetEntries();
  LOG("GHEP", pNOTICE)
    << "Adding particle with pdgc = " << p.PdgCode() << " at slot = " << pos;

  new ((*this)[pos]) GHepParticle(p);

  // Update the mother's daughter list. If the newly inserted particle broke
  // compactification, then run CompactifyDaughterLists()
  this->UpdateDaughterLists();
}
//___________________________________________________________________________
void GHepRecord::AddParticle(
  int pdg, int status, int mom1, int mom2, int dau1, int dau2,
                         const TLorentzVector & p, const TLorentzVector & v)
{
// Provides a 'simplified' method for inserting entries in the TClonesArray

  unsigned int pos = this->GetEntries();
  LOG("GHEP", pNOTICE)
           << "Adding particle with pdgc = " << pdg << " at slot = " << pos;

  new ((*this)[pos]) GHepParticle(pdg,status, mom1,mom2,dau1,dau2, p, v);

  // Update the mother's daughter list. If the newly inserted particle broke
  // compactification, then run CompactifyDaughterLists()
  this->UpdateDaughterLists();
}
//___________________________________________________________________________
void GHepRecord::AddParticle(
  int pdg, int status, int mom1, int mom2, int dau1, int dau2,
                         double px, double py, double pz, double E,
                                     double x, double y, double z, double t)
{
// Provides a 'simplified' method for inserting entries in the TClonesArray

  unsigned int pos = this->GetEntries();
  LOG("GHEP", pNOTICE)
           << "Adding particle with pdgc = " << pdg << " at slot = " << pos;

  new ( (*this)[pos] ) GHepParticle (
            pdg, status, mom1, mom2, dau1, dau2, px, py, pz, E, x, y, z, t);

  // Update the mother's daughter list. If the newly inserted particle broke
  // compactification, then run CompactifyDaughterLists()
  this->UpdateDaughterLists();
}
//___________________________________________________________________________
void GHepRecord::UpdateDaughterLists(void)
{
  int pos = this->GetEntries() - 1; // position of last entry

  LOG("GHEP", pINFO)
     << "Updating the daughter-list for the mother of particle at: " << pos;

  GHepParticle * p = this->GetParticle(pos);
  assert(p);

  int mom_pos = p->FirstMother();
  LOG("GHEP", pINFO) << "Mother particle is at slot: " << mom_pos;
  if(mom_pos==-1) return; // may not have mom (eg init state)  
  GHepParticle * mom = this->GetParticle(mom_pos);
  if(!mom) return; // may not have mom (eg init state)

  int dau1 = mom->FirstDaughter();
  int dau2 = mom->LastDaughter();

  // handles the case where the daughter list was initially empty
  if(dau1 == -1) {
     mom->SetFirstDaughter(pos);
     mom->SetLastDaughter(pos);
     LOG("GHEP", pINFO)
        << "Done! Daughter-list is compact: [" << pos << ", " << pos << "]";
     return;
  }
  // handles the case where the new daughter is added at the slot just before
  // an already compact daughter list
  if(pos == dau1-1) {
     mom->SetFirstDaughter(pos);
     LOG("GHEP", pINFO)
       << "Done! Daughter-list is compact: [" << pos << ", " << dau2 << "]";
     return;
  }
  // handles the case where the new daughter is added at the slot just after
  // an already compact daughter list
  if(pos == dau2+1) {
     mom->SetLastDaughter(pos);
     LOG("GHEP", pINFO)
       << "Done! Daughter-list is compact: [" << dau1 << ", " << pos << "]";
     return;
  }

  // If you are here, then the last particle insertion broke the daughter
  // list compactification - Run the compactifier
  LOG("GHEP", pNOTICE)
                   << "Daughter-list is not compact - Running compactifier";
  this->CompactifyDaughterLists();
}
//___________________________________________________________________________
void GHepRecord::CompactifyDaughterLists(void)
{
  int n     = this->GetEntries();
  int start = this->FirstNonInitStateEntry();

  for(int i=0; i<n; i++) {
     bool compact = this->HasCompactDaughterList(i);
     if(!compact) {
        int ndau = 0;     // number of daughters
        int dau1 = start; // 1st daughter position
        int k=start+1;
        for(; k<n; k++) {
            GHepParticle * p = this->GetParticle(k);
            if(p->FirstMother() == i) {
                 ndau++;
                 this->SwapParticles(start,k);
                 start++;
            }
        }
        if(ndau>0) {
          this->GetParticle(i)->SetFirstDaughter(dau1);
          this->GetParticle(i)->SetLastDaughter(dau1+ndau);
        } else {
          this->GetParticle(i)->SetFirstDaughter(-1);
          this->GetParticle(i)->SetLastDaughter(-1);
        }
     } //!compact
     LOG("GHEP", pNOTICE)
          << "Compactifying daughter-list for particle at slot: " 
                                                    << i << " - Done!";
  }
  this->FinalizeDaughterLists();
}
//___________________________________________________________________________
bool GHepRecord::HasCompactDaughterList(int pos)
{
  LOG("GHEP", pDEBUG) << "Examining daughter-list of particle at: " << pos;

  vector<int> daughters;
  GHepParticle * p = 0;
  TIter iter(this);
  int i=0;
  while( (p = (GHepParticle *)iter.Next()) ) {
    if(p->FirstMother() == pos) {
       LOG("GHEP", pDEBUG) << "Particle at: " << i << " is a daughter";
       daughters.push_back(i);
    }
    i++;
  }

  bool is_compact = true;
  if(daughters.size()>1) {
    sort(daughters.begin(), daughters.end());
    vector<int>::iterator diter = daughters.begin();
    int prev = *diter;
    for(; diter != daughters.end(); ++diter) {
       int curr = *diter;
       is_compact = is_compact && (TMath::Abs(prev-curr)<=1);
       prev = curr;
    }
  }
  LOG("GHEP", pINFO)
         << "Daughter-list of particle at: " << pos << " is "
                                << (is_compact ? "" : "not") << " compact";
  return is_compact;
}
//___________________________________________________________________________
int GHepRecord::FirstNonInitStateEntry(void)
{
  GHepParticle * p = 0;
  TIter iter(this);
  int pos = 0;
  while( (p = (GHepParticle *)iter.Next()) ) {
    int ist = p->Status();
    if(ist != kIStInitialState && ist != kIstNucleonTarget) return pos;
    pos++;
  }
  return pos;
}
//___________________________________________________________________________
void GHepRecord::SwapParticles(int i, int j)
{
  LOG("GHEP", pINFO) << "Swapping GHepParticles : " << i << " <--> " << j;

  int n = this->GetEntries();
  assert(i>=0 && j>=0 && i<n && j<n);

  if(i==j) return;

  GHepParticle * pi  = this->GetParticle(i);
  GHepParticle * pj  = this->GetParticle(j);
  GHepParticle * tmp = new GHepParticle(*pi);

  pi->Copy(*pj);
  pj->Copy(*tmp);

  delete tmp;

  // tell their daughters
  if(pi->HasDaughters()) {
    for(int k=pi->FirstDaughter(); k<=pi->LastDaughter(); k++)
      this->GetParticle(k)->SetFirstMother(j);
  }
  if(pj->HasDaughters()) {
    for(int k=pj->FirstDaughter(); k<=pj->LastDaughter(); k++)
      this->GetParticle(k)->SetFirstMother(i);
  }
}
//___________________________________________________________________________
void GHepRecord::FinalizeDaughterLists(void)
{
// Update all daughter-lists based on particle 'first mother' field.
// To work correctly, the daughter-lists must have been compactified first.

  GHepParticle * p1 = 0;
  TIter iter1(this);
  int i1=0;
  while( (p1 = (GHepParticle *)iter1.Next()) ) {
    int dau1 = -1;
    int dau2 = -1;
    GHepParticle * p2 = 0;
    TIter iter2(this);
    int i2=0;
    while( (p2 = (GHepParticle *)iter2.Next()) ) {

       if(p2->FirstMother() == i1) {
          dau1 = (dau1<0) ? i2 : TMath::Min(dau1,i2);
          dau2 = (dau2<0) ? i2 : TMath::Max(dau2,i2);
       }
       i2++;
    }
    i1++;
    p1 -> SetFirstDaughter (dau1);
    p1 -> SetLastDaughter  (dau2);
  }
}
//___________________________________________________________________________
void GHepRecord::SwitchIsPauliBlocked(bool on_off)
{
  LOG("GHEP", pNOTICE)
    << "Switching Pauli Block flag: " << print_utils::BoolAsIOString(on_off);

  fIsPauliBlocked = on_off;
}
//___________________________________________________________________________
void GHepRecord::SwitchIsBelowThrNRF(bool on_off)
{
  LOG("GHEP", pNOTICE)
      << "Switching Below Threshold in nucleon rest frame flag: "
                                     << print_utils::BoolAsIOString(on_off);
  fIsBelowThrNRF = on_off;
}
//___________________________________________________________________________
void GHepRecord::SwitchGenericErrFlag(bool on_off)
{
  LOG("GHEP", pNOTICE)
      << "Switching Generic Error Flag: "
                               << print_utils::BoolAsIOString(on_off);
  fGenericErrFlag = on_off;
}
//___________________________________________________________________________
bool GHepRecord::IsUnphysical(void) const
{
// Summarizes record flags

  return (fIsPauliBlocked || fIsBelowThrNRF || fGenericErrFlag);
}
//___________________________________________________________________________
void GHepRecord::InitGHepRecord(void)
{
  LOG("GHEP", pDEBUG) << "Initializing GHepRecord";

  fInteraction = 0;

  this -> SwitchIsPauliBlocked (false);
  this -> SwitchIsBelowThrNRF  (false);
  this -> SwitchGenericErrFlag (false);

  this->SetOwner(true);
}
//___________________________________________________________________________
void GHepRecord::CleanUpGHepRecord(void)
{
  LOG("GHEP", pDEBUG) << "Cleaning up GHepRecord";

  if (fInteraction) delete fInteraction;

  this->Clear("C");
}
//___________________________________________________________________________
void GHepRecord::ResetGHepRecord(void)
{
  LOG("GHEP", pDEBUG) << "Reseting GHepRecord";

  this->CleanUpGHepRecord();
  this->InitGHepRecord();
}
//___________________________________________________________________________
void GHepRecord::Copy(const GHepRecord & record)
{
  // clean up
  this->ResetGHepRecord();

  // copy event record entries
  unsigned int ientry = 0;
  GHepParticle * p = 0;
  TIter ghepiter(&record);
  while ( (p = (GHepParticle *) ghepiter.Next()) )
                         new ( (*this)[ientry++] ) GHepParticle(*p);

  // copy summary
  fInteraction = new Interaction( *record.fInteraction );

  // copy flags
  fIsPauliBlocked  = record.fIsPauliBlocked;
  fIsBelowThrNRF   = record.fIsBelowThrNRF;
  fGenericErrFlag  = record.fGenericErrFlag;
}
//___________________________________________________________________________
void GHepRecord::Print(ostream & stream) const
{
  stream << "\n\n |";
  stream << setfill('-') << setw(109) << "|";

  stream << "\n |";
  stream << setfill(' ') << setw(6)  << "Idx | "
         << setfill(' ') << setw(11) << "Name | "
         << setfill(' ') << setw(6)  << "Ist | "
         << setfill(' ') << setw(13) << "PDG | "
         << setfill(' ') << setw(12) << "Mother  | "
         << setfill(' ') << setw(12) << "Daughter  | "
         << setfill(' ') << setw(10) << "Px | "
         << setfill(' ') << setw(10) << "Py | "
         << setfill(' ') << setw(10) << "Pz | "
         << setfill(' ') << setw(10) << "E  | "
         << setfill(' ') << setw(10) << "m  | ";

  stream << "\n |";
  stream << setfill('-') << setw(109) << "|";

  GHepParticle * p = 0;

  TObjArrayIter piter(this);

  unsigned int idx = 0;

  double sum_E  = 0;
  double sum_px = 0;
  double sum_py = 0;
  double sum_pz = 0;

  while( (p = (GHepParticle *) piter.Next()) ) {

     stream << "\n |";
     stream << setfill(' ') << setw(3)  << idx++               << " | ";
     stream << setfill(' ') << setw(8)  << p->Name()           << " | ";
     stream << setfill(' ') << setw(3)  << p->Status()         << " | ";
     stream << setfill(' ') << setw(10) << p->PdgCode()        << " | ";
     stream << setfill(' ') << setw(3)  << p->FirstMother()    << " | ";
     stream << setfill(' ') << setw(3)  << p->LastMother()     << " | ";
     stream << setfill(' ') << setw(3)  << p->FirstDaughter()  << " | ";
     stream << setfill(' ') << setw(3)  << p->LastDaughter()   << " | ";
     stream << setiosflags(ios::fixed) << setprecision(3);
     stream << setfill(' ') << setw(7)  << p->Px()             << " | ";
     stream << setfill(' ') << setw(7)  << p->Py()             << " | ";
     stream << setfill(' ') << setw(7)  << p->Pz()             << " | ";
     stream << setfill(' ') << setw(7)  << p->E()              << " | ";

     if( p->IsOnMassShell() )
        stream << setfill(' ') << setw(7)  << p->Mass()        << " | ";
     else
        stream << setfill('*') << setw(7)  << p->Mass()        << " | " << p->GetP4()->M();

     // compute P4Final - P4Initial
     //
     // Take into account real particles and fake (generator-specific)
     // particles (rootino, bindino, ...) used to record non-fake physics.
     // Ignore initial & final state ions (if any).

     if( p->IsParticle() || p->IsFake() ) {

       if(p->Status() == kIStStableFinalState) {

          sum_E  += p->E();
          sum_px += p->Px();
          sum_py += p->Py();
          sum_pz += p->Pz();
       }
       else if(p->Status() == kIStInitialState || p->Status() == kIstNucleonTarget) {

          sum_E  -= p->E();
          sum_px -= p->Px();
          sum_py -= p->Py();
          sum_pz -= p->Pz();
       }
     }// !nucleus

  } // loop over particles

  stream << "\n |";
  stream << setfill('-') << setw(109) << "|";

  // Print SUMS
  stream << "\n |";
  stream << setfill(' ') << setw(17) << "Fin-Init:| "
         << setfill(' ') << setw(6)  << "    | "
         << setfill(' ') << setw(13) << "    | "
         << setfill(' ') << setw(12) << "        | "
         << setfill(' ') << setw(12) << "          | ";
  stream << setiosflags(ios::fixed) << setprecision(3);
  stream << setfill(' ') << setw(7)  << sum_px  << " | ";
  stream << setfill(' ') << setw(7)  << sum_py  << " | ";
  stream << setfill(' ') << setw(7)  << sum_pz  << " | ";
  stream << setfill(' ') << setw(7)  << sum_E   << " | ";
  stream << setfill(' ') << setw(10)  << "   | ";

  stream << "\n |";
  stream << setfill('-') << setw(109) << "|";

  // Print FLAGS
  stream << "\n |";
  stream << setfill(' ') << setw(17) << "FLAGS:   | "
         << setfill(' ') << setw(15) << "PauliBlock......"
         << print_utils::BoolAsIOString(this->IsPauliBlocked()) << " |"
         << setfill(' ') << setw(15) << " BelowThrNRF...."
         << print_utils::BoolAsIOString(this->IsBelowThrNRF())  << " |"
         << setfill(' ') << setw(15) << " GenericErr....."
         << print_utils::BoolAsIOString(this->GenericErrFlag()) << " |"
         << setfill(' ') << setw(15) << " UnPhysical....."
         << print_utils::BoolAsIOString(this->IsUnphysical())   << " |";

  stream << "\n |";
  stream << setfill('-') << setw(109) << "|";
  stream << "\n";

}
//___________________________________________________________________________
