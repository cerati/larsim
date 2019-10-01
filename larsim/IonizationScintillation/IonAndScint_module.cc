////////////////////////////////////////////////////////////////////////
// Class:       IonAndScint
// Plugin Type: producer
// File:        IonAndScint_module.cc
// Description:
// - acts on sim::SimEnergyDeposit from LArG4Main, 
// - calculate the number of photons and electrons
// Input: 'sim::SimEnergyDeposit'
// Output: updated 'sim::SimEnergyDeposit' with numPhotons and numElectrons

//This module calculate the number of photons and electrons produced at each step where energy is deposited.
//The Separate algorithm is used.
//At the end of this module the numPhotons and numElectrons of sim:SimEnergyDeposit have been updated.

// Aug.18 by Mu Wei
////////////////////////////////////////////////////////////////////////

// LArSoft includes
#include "larsim/IonizationScintillation/ISCalc.h"
#include "larsim/IonizationScintillation/ISCalcSeparate.h"
#include "larsim/IonizationScintillation/ISCalcNESTLAr.h"
#include "lardataobj/Simulation/SimEnergyDeposit.h"

// Framework includes
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Core/EDProducer.h"

namespace larg4
{
    class IonAndScint : public art::EDProducer
    {
    public:
        explicit IonAndScint(fhicl::ParameterSet const& pset);
        void produce(art::Event& event)         override;
        void beginJob()                         override;
        void endJob()                           override;
        
    private:
        art::InputTag            simTag;
        art::InputTag            calcTag;             // name of calculator to use, NEST or Separate
        ISCalc*                  fISAlg;
        CLHEP::HepRandomEngine&  fEngine;             // random number engine for NEST algorithm
    };
    
    //......................................................................    
    IonAndScint::IonAndScint(fhicl::ParameterSet const& pset)
    : art::EDProducer{pset}
    , simTag{pset.get<art::InputTag>("SimulationLabel")}
    , calcTag{pset.get<art::InputTag>("ISCalcAlg")}
    , fEngine(art::ServiceHandle<rndm::NuRandomService>()->createEngine(*this, "HepJamesRandom", "NEST", pset, "SeedNEST"))
    {
        std::cout << "IonAndScint Module Construct" << std::endl;
        std::cout << "Using " << calcTag.label() << " algorithm to calculate IS." << std::endl;
        
        if(calcTag.label().compare("NEST") == 0)
        {
            fISAlg = new ISCalcNESTLAr(fEngine);
        }
        else if(calcTag.label().compare("Separate") == 0)
        {
            fISAlg = new ISCalcSeparate();
        }
        else
        {
            mf::LogWarning("ISCalcAna") << "No ISCalculation set, this can't be good.";
        }
        
        produces< std::vector<sim::SimEnergyDeposit> >(calcTag.label());        
    }
    
    //......................................................................    
    void IonAndScint::beginJob()
    {
        std::cout << "IonAndScint beginJob." << std::endl;
        
        fISAlg->Reset();
        fISAlg->Initialize();
        
        return;
    }
    //......................................................................    
    void IonAndScint::endJob()
    {
        std::cout << "ISCalcAna endJob." << std::endl;
        
        if(fISAlg)
        {
            delete fISAlg;
        }
        
        return;
    }
    //......................................................................    
    void IonAndScint::produce(art::Event& event)
    {
        std::cout << "IonAndScint Module Producer" << std::endl;
        
        std::unique_ptr< std::vector<sim::SimEnergyDeposit> >  simedep (new std::vector<sim::SimEnergyDeposit>);        
        art::Handle< std::vector<sim::SimEnergyDeposit> > edepHandle;
        
        if (!event.getByLabel(simTag, edepHandle))
        {
            std::cout << "IonAndScint Module Cannot getByLabel: " << simTag << std::endl;
            return;
        }
        
        auto const& edeps = edepHandle;
        for (sim::SimEnergyDeposit const& edepi: *edeps)
        {
            fISAlg->CalcIonAndScint(edepi);
            
            int           ph_num        = fISAlg->NumOfPhotons();
            int           fph_num       = fISAlg->NumOfFastPhotons();
            int           sph_num       = fISAlg->NumOfSlowPhotons();
            int           ion_num       = fISAlg->NumOfElectrons();
            float         scintyield    = fISAlg->ScintillationYieldRatio();
            float         edep_tmp      = edepi.Energy();
            geo::Point_t  startPos_tmp  = edepi.Start();
            geo::Point_t  endPos_tmp    = edepi.End();
            double        startTime_tmp = edepi.StartT();
            double        endTime_tmp   = edepi.EndT();
            int           trackID_tmp   = edepi.TrackID();
            int           pdgCode_tmp   = edepi.PdgCode();
            
            simedep->emplace_back(ph_num, fph_num, sph_num, ion_num, scintyield, edep_tmp,
                                  startPos_tmp, endPos_tmp,
                                  startTime_tmp, endTime_tmp,
                                  trackID_tmp, pdgCode_tmp);
        }
        
        event.put(std::move(simedep), calcTag.label());
        
        return;
    }   
} // namespace
DEFINE_ART_MODULE(larg4::IonAndScint)
