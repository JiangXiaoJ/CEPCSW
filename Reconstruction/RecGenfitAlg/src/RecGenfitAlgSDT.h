//////////////////////////////////////////////////////////////////////
///
/// This is an algorithm for track fitting for CEPC track with genfit.
///
/// In this file, including:
///   An algorithm for combined silicon and drift chamber track fitting
///   with genfit for 5 particle hypothesis
///
///   Units are following DD4hepUnits
///
/// Authors:
///   Yao ZHANG(zhangyao@ihep.ac.cn)
///
/////////////////////////////////////////////////////////////////////

#ifndef RECGENFITALG_RECGENFITALGSDT_H
#define RECGENFITALG_RECGENFITALGSDT_H

#include "GaudiAlg/GaudiAlgorithm.h"
#include "GaudiKernel/NTuple.h"
#include "k4FWCore/DataHandle.h"
#include "DD4hep/Fields.h"
#include <string>

class GenfitFitter;
class GenfitField;
class GenfitTrack;
class IGeomSvc;
class time;
namespace genfit{
    class EventDisplay;
}
namespace dd4hep {
    class Detector;
    //class rec::CellIDPositionConverter;
    namespace DDSegmentation{
        class GridDriftChamber;
        class BitFieldCoder;
    }
}
namespace edm4hep{
    class EventHeaderCollection;
    class MCParticleCollection;
    class SimTrackerHitCollection;
    class TrackCollection;
    class TrackerHitCollection;
    class MCRecoTrackerAssociationCollection;
    class ReconstructedParticle;
    class ReconstructedParticleCollection;
}

/////////////////////////////////////////////////////////////////////////////

class RecGenfitAlgSDT:public GaudiAlgorithm {
    public:
        RecGenfitAlgSDT (const std::string& name, ISvcLocator* pSvcLocator);
        StatusCode initialize() override; StatusCode execute() override; StatusCode finalize() override;

    private:
        GenfitFitter* m_genfitFitter;//The pointer to a GenfitFitter
        const GenfitField* m_genfitField;//The pointer to a GenfitField

        void debugTrack(int pidType,const GenfitTrack* genfitTrack);
        void debugEvent(const edm4hep::TrackCollection* sdtTrackCol,
                double eventStartTime);

        DataHandle<edm4hep::EventHeaderCollection> m_headerCol{
            "EventHeaderCol", Gaudi::DataHandle::Reader, this};
        //Drift chamber rec hit and trac
        DataHandle<edm4hep::TrackerHitCollection> m_DCDigiCol{
            "DigiDCHitCollection", Gaudi::DataHandle::Reader, this};
        //Mc truth
        DataHandle<edm4hep::MCParticleCollection> m_mcParticleCol{
            "MCParticle", Gaudi::DataHandle::Reader, this};
        DataHandle<edm4hep::SimTrackerHitCollection> m_simDCHitCol{
            "DriftChamberHitsCollection" , Gaudi::DataHandle::Reader, this};
        DataHandle<edm4hep::MCRecoTrackerAssociationCollection>
            m_DCHitAssociationCol{"DCHitAssociationCollection",
                Gaudi::DataHandle::Reader, this};

        //Track from silicon detectors
        DataHandle<edm4hep::TrackCollection> m_SDTTrackCol{"SDTTrackCollection",
            Gaudi::DataHandle::Reader, this};

        //Output hits and particles
        DataHandle<edm4hep::ReconstructedParticleCollection> m_SDTRecParticleCol{
            "SDTRecParticleCollection", Gaudi::DataHandle::Writer, this};

        const unsigned int m_nPDG;//5:e,mu,pi,K,proton
        SmartIF<IGeomSvc> m_geomSvc;
        dd4hep::OverlayedField m_dd4hepField;
        dd4hep::Detector* m_dd4hep;
        dd4hep::DDSegmentation::GridDriftChamber* m_gridDriftChamber;
        dd4hep::DDSegmentation::BitFieldCoder* m_decoder;
        Gaudi::Property<std::string> m_readout_name{this,
            "readout", "DriftChamberHitsCollection"};
        Gaudi::Property<int> m_debug{this,"debug",0};
        Gaudi::Property<float> m_sigmaHit{this,"sigmaHit",0.11};//mm
        Gaudi::Property<float> m_nSigmaHit{this,"nSigmaHit",5};
        Gaudi::Property<double> m_initCovResPos{this,"initCovResPos",1};
        Gaudi::Property<double> m_initCovResMom{this,"initCovResMom",0.1};
        //Fitter type default is DAFRef.
        //Candidates are DAF,DAFRef,KalmanFitter and KalmanFitterRefTrack.
        Gaudi::Property<std::string> m_fitterType{this,"fitterTyep","DAFRef"};
        Gaudi::Property<bool> m_correctBremsstrahlung{this,
            "correctBremsstrahlung",false};
        Gaudi::Property<bool> m_noMaterialEffects{this,
            "noMaterialEffects",false};
        Gaudi::Property<int> m_maxIteration{this,"maxIteration",20};
        Gaudi::Property<int> m_resortHits{this,"resortHits",true};
        Gaudi::Property<double> m_bStart{this,"bStart",100};
        Gaudi::Property<double> m_bFinal{this,"bFinal",0.01};
        Gaudi::Property<double> m_DCCornerCuts{this,"dcCornerCuts",-999};
        Gaudi::Property<double> m_ndfCut{this,"ndfCut",1e9};
        Gaudi::Property<double> m_chi2Cut{this,"chi2Cut",1e9};
        //-1,chargedGeantino;0,1,2,3,4:e,mu,pi,K,proton
        Gaudi::Property<int> m_debugPid{this,"debugPid",-99};
        Gaudi::Property<bool> m_useTruthTrack{this,"useTruthTrack",true};
        Gaudi::Property<bool> m_useTruthHit{this,"useTruthHit",true};
        Gaudi::Property<std::string> m_genfitHistRootName{this,
            "genfitHistRootName",""};
        Gaudi::Property<bool> m_showDisplay{this,"showDisplay",false};
        int m_fitSuccess[5];
        int m_nRecTrack;
        //bool m_useRecLRAmbig;

        genfit::EventDisplay* m_genfitDisplay;
        clock_t m_timer;

        /// tuples
        NTuple::Tuple*  m_tuple;
        NTuple::Item<int> m_run;
        NTuple::Item<int> m_evt;
        NTuple::Item<int> m_tkId;
        NTuple::Item<int> m_mcIndex;//number of navigated mcParicle
        NTuple::Matrix<double> m_truthPocaMc;//2 dim matched particle and 3 pos.
        NTuple::Item<double> m_seedMomP;//for single track
        NTuple::Item<double> m_seedMomPt;
        NTuple::Array<double> m_seedMom;
        NTuple::Array<double> m_seedPos;
        NTuple::Matrix<double> m_pocaPosMc;//2 dim matched particle and 3 pos.
        NTuple::Matrix<double> m_pocaMomMc;//2 dim matched particle and 3 mom.
        NTuple::Array<double> m_pocaMomMcP;//2 dim matched particle and p
        NTuple::Array<double> m_pocaMomMcPt;//2 dim matched particle and pt
        NTuple::Array<double> m_pocaPosMdc;//pos 0:x,1:y,2:z
        NTuple::Array<double> m_pocaMomMdc;//mom. 0:px,1:py,2:pz
        NTuple::Item<int> m_pidIndex;
        NTuple::Matrix<double> m_firstPosKal;//5 hyposis and pos. at first
        NTuple::Array<double> m_firstMomKalP;//5 hyposis and mom. at first
        NTuple::Array<double> m_firstMomKalPt;//5 hyposis and mom. at first
        NTuple::Matrix<double> m_pocaPosKal;//5 hyposis and 3 mom.
        NTuple::Matrix<double> m_pocaMomKal;//5 hyposis and 3 mom.
        NTuple::Array<double> m_pocaMomKalP;//5 hyposis and p
        NTuple::Array<double> m_pocaMomKalPt;//5 hyposis and pt
        NTuple::Array<int> m_chargeKal;
        NTuple::Array<double> m_chi2Kal;
        NTuple::Array<double> m_nDofKal;
        NTuple::Array<int> m_isFitConverged;
        NTuple::Array<int> m_isFitConvergedFully;
        NTuple::Array<int> m_isFitted;
        NTuple::Item<int> m_nDigi;
        NTuple::Item<int> m_nHitMc;
        NTuple::Item<int> m_nSimDCHit;
        NTuple::Array<int> m_nHitWithFitInfo;
        NTuple::Item<int> m_nHitKalInput;
        NTuple::Array<double> m_mdcHitDriftT;
        NTuple::Array<double> m_mdcHitDriftDl;
        NTuple::Array<double> m_mdcHitDriftDr;
        NTuple::Array<int> m_mdcHitLr;
        NTuple::Array<int> m_mdcHitLayer;
        NTuple::Array<int> m_mdcHitWire;
        NTuple::Array<double> m_mdcHitExpDoca;
        NTuple::Array<double> m_mdcHitExpMcDoca;
        NTuple::Array<double> m_mdcHitErr;
        NTuple::Array<int> m_nHitFailedKal;
        NTuple::Array<int> m_nHitFitted;
        NTuple::Array<double> m_time;
        //truth
        NTuple::Array<int> m_mdcHitMcLr;
        NTuple::Array<int> m_mdcHitMcTkId;
        NTuple::Array<double> m_mdcHitMcDrift;
        NTuple::Array<double> m_mdcHitMcX;
        NTuple::Array<double> m_mdcHitMcY;
        NTuple::Array<double> m_mdcHitMcZ;
        NTuple::Array<double> m_mdcHitExpMcPocaX;
        NTuple::Array<double> m_mdcHitExpMcPocaY;
        NTuple::Array<double> m_mdcHitExpMcPocaZ;
        NTuple::Array<double> m_mdcHitExpMcPocaWireX;
        NTuple::Array<double> m_mdcHitExpMcPocaWireY;
        NTuple::Array<double> m_mdcHitExpMcPocaWireZ;

};
#endif
